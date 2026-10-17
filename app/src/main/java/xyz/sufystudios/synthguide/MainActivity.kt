package xyz.sufystudios.synthguide

import android.Manifest
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.viewModels
import androidx.compose.runtime.snapshotFlow
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.flow.first
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
        askForNotificationsWhenFirstSustaining()
    }

    /**
     * Android 13+ needs permission to show the background-playback notification.
     * Ask once, the first time something keeps sounding - when the reason is obvious.
     */
    private fun askForNotificationsWhenFirstSustaining() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) return
        val prefs = getSharedPreferences("ui", MODE_PRIVATE)
        if (prefs.getBoolean("askedNotifications", false)) return
        val request = registerForActivityResult(ActivityResultContracts.RequestPermission()) {}
        // Create the view model here, not lazily inside snapshotFlow: that block runs
        // in a read-only snapshot, where the view model's initial state writes would throw.
        val model = vm
        lifecycleScope.launch {
            snapshotFlow { model.isSustaining }.first { it }
            if (checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
                prefs.edit().putBoolean("askedNotifications", true).apply()
                request.launch(Manifest.permission.POST_NOTIFICATIONS)
            }
        }
    }

    override fun onStart() {
        super.onStart()
        vm.onAppForeground()
    }

    override fun onResume() {
        super.onResume()
        // Picks up purchases made or refunded while the app was in the background.
        vm.refreshPurchases()
    }

    override fun onStop() {
        vm.onAppBackground()
        super.onStop()
    }
}
