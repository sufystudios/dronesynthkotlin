package xyz.sufystudios.synthguide

import android.graphics.Color
import android.os.Bundle
import android.view.WindowManager
import androidx.activity.ComponentActivity
import androidx.activity.SystemBarStyle
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
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
    }

    override fun onStart() {
        super.onStart()
        vm.startAudio()
    }

    override fun onResume() {
        super.onResume()
        // Picks up purchases made or refunded while the app was in the background.
        vm.refreshPurchases()
    }

    override fun onStop() {
        vm.stopAudio()
        super.onStop()
    }
}
