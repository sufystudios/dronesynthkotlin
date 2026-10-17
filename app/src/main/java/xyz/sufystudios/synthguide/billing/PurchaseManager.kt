package xyz.sufystudios.synthguide.billing

import android.app.Activity
import android.content.Context
import android.util.Log
import com.android.billingclient.api.AcknowledgePurchaseParams
import com.android.billingclient.api.BillingClient
import com.android.billingclient.api.BillingClientStateListener
import com.android.billingclient.api.BillingFlowParams
import com.android.billingclient.api.BillingResult
import com.android.billingclient.api.PendingPurchasesParams
import com.android.billingclient.api.ProductDetails
import com.android.billingclient.api.Purchase
import com.android.billingclient.api.QueryProductDetailsParams
import com.android.billingclient.api.QueryPurchasesParams
import com.android.billingclient.api.acknowledgePurchase
import com.android.billingclient.api.queryProductDetails
import com.android.billingclient.api.queryPurchasesAsync
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import kotlinx.coroutines.launch

/**
 * One-time purchases. The product IDs are the ones the app has always sold on
 * Google Play, so existing customers keep what they bought.
 */
enum class Product(val productId: String) {
    /** Sold as the second filter ("filt2") in earlier versions; it is the stereo filter pair. */
    StereoFilters("ai_drone_upgrade"),
    // "tube" and "spaceskin" were sold until 2.0, which retired both features.
}

class PurchaseManager(context: Context, private val scope: CoroutineScope, private val unlockAll: Boolean) {
    private val _owned = MutableStateFlow(if (unlockAll) Product.entries.toSet() else emptySet())

    /** Products this user owns. */
    val owned: StateFlow<Set<Product>> = _owned.asStateFlow()

    private val details = mutableMapOf<Product, ProductDetails>()

    private val client = BillingClient.newBuilder(context.applicationContext)
        .setListener { result, purchases -> onPurchasesUpdated(result, purchases) }
        .enablePendingPurchases(PendingPurchasesParams.newBuilder().enableOneTimeProducts().build())
        .enableAutoServiceReconnection()
        .build()

    fun connect() {
        if (client.isReady) {
            refresh()
            return
        }
        client.startConnection(object : BillingClientStateListener {
            override fun onBillingSetupFinished(result: BillingResult) {
                if (result.responseCode == BillingClient.BillingResponseCode.OK) refresh()
                else Log.w(TAG, "Billing setup failed: ${result.debugMessage}")
            }

            override fun onBillingServiceDisconnected() = Unit
        })
    }

    fun owns(product: Product): Boolean = product in _owned.value

    /** Starts Google Play's purchase sheet for [product]. */
    fun purchase(activity: Activity, product: Product) {
        val productDetails = details[product]
        if (productDetails == null) {
            Log.w(TAG, "No product details for ${product.productId} yet")
            refresh()
            return
        }
        val params = BillingFlowParams.newBuilder()
            .setProductDetailsParamsList(
                listOf(BillingFlowParams.ProductDetailsParams.newBuilder().setProductDetails(productDetails).build())
            )
            .build()
        client.launchBillingFlow(activity, params)
    }

    private fun refresh() {
        scope.launch {
            val owned = client.queryPurchasesAsync(
                QueryPurchasesParams.newBuilder().setProductType(BillingClient.ProductType.INAPP).build()
            )
            if (owned.billingResult.responseCode == BillingClient.BillingResponseCode.OK) handlePurchases(owned.purchasesList)

            val query = QueryProductDetailsParams.newBuilder()
                .setProductList(Product.entries.map {
                    QueryProductDetailsParams.Product.newBuilder()
                        .setProductId(it.productId)
                        .setProductType(BillingClient.ProductType.INAPP)
                        .build()
                })
                .build()
            val result = client.queryProductDetails(query)
            result.productDetailsList?.forEach { pd ->
                Product.entries.firstOrNull { it.productId == pd.productId }?.let { details[it] = pd }
            }
        }
    }

    private fun onPurchasesUpdated(result: BillingResult, purchases: List<Purchase>?) {
        if (result.responseCode == BillingClient.BillingResponseCode.OK && purchases != null) {
            handlePurchases(purchases)
        } else if (result.responseCode == BillingClient.BillingResponseCode.ITEM_ALREADY_OWNED) {
            refresh()
        }
    }

    private fun handlePurchases(purchases: List<Purchase>) {
        val purchased = purchases.filter { it.purchaseState == Purchase.PurchaseState.PURCHASED }
        val products = purchased.flatMap { p -> p.products.mapNotNull { id -> Product.entries.firstOrNull { it.productId == id } } }
        _owned.update { it + products }
        // Purchases must be acknowledged within three days or Google refunds them.
        purchased.filterNot { it.isAcknowledged }.forEach { purchase ->
            scope.launch {
                client.acknowledgePurchase(
                    AcknowledgePurchaseParams.newBuilder().setPurchaseToken(purchase.purchaseToken).build()
                )
            }
        }
    }

    private companion object {
        const val TAG = "PurchaseManager"
    }
}
