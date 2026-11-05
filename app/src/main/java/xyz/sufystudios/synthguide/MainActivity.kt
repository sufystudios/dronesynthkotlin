package xyz.sufystudios.synthguide

import android.graphics.Color
import android.os.Bundle
import android.util.Log
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.lifecycleScope
import com.google.android.play.core.ktx.launchReview
import com.google.android.play.core.ktx.requestReview
import com.google.android.play.core.review.ReviewManagerFactory
import kotlinx.coroutines.CancellationException
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.DroneSynthScreen

class MainActivity : ComponentActivity() {
    private val vm: SynthViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Always light system-bar icons: the synth is dark in both system themes.
        enableEdgeToEdge(SystemBarStyle.dark(Color.TRANSPARENT), SystemBarStyle.dark(Color.TRANSPARENT))
        // A drone can sustain for minutes with nobody touching the screen.
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        setContent { DroneSynthScreen(vm) }
        if (vm.reviews.wantsToAsk) askForReviewLater()
    }

    /**
     * Play's rating card, a minute or so into this session rather than as the app opens -
     * and not during a recording, or once the app has left the screen. The view model
     * has already decided this open is one to ask on.
     */
    private fun askForReviewLater() {
        lifecycleScope.launch {
            delay(60_000)
            // Wait out a recording, rather than interrupt it.
            while (vm.isCapturing) delay(10_000)
            if (!lifecycle.currentState.isAtLeast(Lifecycle.State.RESUMED)) return@launch
            try {
                val manager = ReviewManagerFactory.create(this@MainActivity)
                val info = manager.requestReview()
                vm.reviews.asked()
                manager.launchReview(this@MainActivity, info)
            } catch (e: CancellationException) {
                throw e
            } catch (e: Exception) {
                // No Play services or no network: a rating isn't worth bothering anyone over.
                Log.w("DroneSynth", "Couldn't ask for a rating", e)
            }
        }
    }

    override fun onStart() {
        super.onStart()
        vm.reviews.setActive(true)
        vm.onAppForeground()
    }

    override fun onResume() {
        super.onResume()
        // Picks up purchases made or refunded while the app was in the background.
        vm.refreshPurchases()
    }

    override fun onStop() {
        vm.reviews.setActive(false)
        vm.onAppBackground()
        super.onStop()
    }
}
