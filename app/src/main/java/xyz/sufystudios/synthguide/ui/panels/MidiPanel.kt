package xyz.sufystudios.synthguide.ui.panels

import android.bluetooth.BluetoothAdapter
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import androidx.activity.compose.LocalActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.window.Dialog
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import xyz.sufystudios.synthguide.midi.BluetoothMidi
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroDim
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel

/** Something stopping a Bluetooth scan, and the button that fixes it. */
private data class ScanBlocker(val message: String, val action: String, val intent: Intent)

/**
 * The MIDI window: connected keyboards, and a Bluetooth scan to connect new ones.
 * USB keyboards need nothing here - they connect when plugged in.
 */
@Composable
fun MidiDialog(vm: SynthViewModel, onDismiss: () -> Unit) {
    val devices by vm.midiDevices.collectAsStateWithLifecycle()
    val bluetooth = vm.bluetoothMidi
    // Stop scanning (it uses the radio heavily) once the window closes.
    DisposableEffect(bluetooth) { onDispose { bluetooth?.stopScan() } }

    Dialog(onDismissRequest = onDismiss) {
        RetroPanelChrome(Modifier.widthIn(max = 360.dp).verticalScroll(rememberScrollState())) {
            RetroHeader("MIDI KEYBOARDS", ledLit = devices.isNotEmpty())

            Section("CONNECTED")
            if (devices.isEmpty()) {
                Hint("None yet. USB keyboards connect as soon as they're plugged in.")
            } else {
                devices.forEach { device ->
                    DeviceRow(device.name, if (device.bluetoothAddress != null) "Disconnect" else null) {
                        device.bluetoothAddress?.let { bluetooth?.disconnect(it) }
                    }
                }
            }

            if (bluetooth?.isSupported == true) {
                BluetoothSection(bluetooth, connectedAddresses = devices.mapNotNull { it.bluetoothAddress }.toSet())
            }
            PanelButton("Done", Modifier.fillMaxWidth(), onClick = onDismiss)
        }
    }
}

@Composable
private fun BluetoothSection(bluetooth: BluetoothMidi, connectedAddresses: Set<String>) {
    val activity = LocalActivity.current
    val scanning by bluetooth.scanning.collectAsStateWithLifecycle()
    val found by bluetooth.found.collectAsStateWithLifecycle()
    val connecting by bluetooth.connecting.collectAsStateWithLifecycle()
    var blocker by remember { mutableStateOf<ScanBlocker?>(null) }
    var scanned by remember { mutableStateOf(false) }

    val appSettings = Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS, Uri.fromParts("package", activity?.packageName, null))

    // Each step clears one thing in the way, then tries the scan again.
    lateinit var tryScan: () -> Unit
    val enableBluetooth = rememberLauncherForActivityResult(ActivityResultContracts.StartActivityForResult()) {
        if (bluetooth.isEnabled) tryScan()
    }
    val askPermissions = rememberLauncherForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) { results ->
        if (results.values.all { it }) {
            tryScan()
        } else {
            blocker = ScanBlocker("Drone Synth needs the Nearby devices permission to find Bluetooth keyboards.", "Open settings", appSettings)
        }
    }
    tryScan = {
        blocker = null
        when {
            !bluetooth.hasPermissions() -> askPermissions.launch(bluetooth.requiredPermissions)
            !bluetooth.isEnabled -> enableBluetooth.launch(Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE))
            bluetooth.needsLocationOn -> blocker = ScanBlocker(
                "Turn on Location: older Android versions need it to find Bluetooth keyboards.",
                "Location settings",
                Intent(Settings.ACTION_LOCATION_SOURCE_SETTINGS),
            )
            else -> {
                scanned = true
                bluetooth.startScan()
            }
        }
    }

    Section("BLUETOOTH")
    val available = found.filter { it.address !in connectedAddresses }
    available.forEach { keyboard ->
        val busy = keyboard.address in connecting
        DeviceRow(keyboard.name, if (busy) "Connecting…" else "Connect", enabled = !busy) { bluetooth.connect(keyboard.address) }
    }
    when {
        scanning -> Hint("Searching… switch your keyboard's Bluetooth on.")
        scanned && available.isEmpty() -> Hint("No Bluetooth MIDI keyboards found. Make sure yours is on and in pairing mode.")
    }
    blocker?.let { b ->
        Hint(b.message, color = RetroLed)
        PanelButton(b.action, Modifier.fillMaxWidth()) { runCatching { activity?.startActivity(b.intent) } }
    }
    PanelButton(if (scanning) "Stop searching" else "Search for Bluetooth keyboards", Modifier.fillMaxWidth()) {
        if (scanning) bluetooth.stopScan() else tryScan()
    }
}

@Composable
private fun Section(title: String) {
    BasicText(title, style = retroLabel(9.sp), modifier = Modifier.fillMaxWidth())
}

@Composable
private fun Hint(text: String, color: Color = RetroDim) {
    BasicText(text, style = retroLabel(9.sp, color, androidx.compose.ui.text.font.FontWeight.Normal), modifier = Modifier.fillMaxWidth())
}

@Composable
private fun DeviceRow(name: String, action: String?, enabled: Boolean = true, onAction: () -> Unit) {
    Row(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(4.dp))
            .background(Color.Black.copy(alpha = 0.4f))
            .padding(start = 10.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        BasicText(name, style = retroLabel(10.sp, Color.White), maxLines = 1, modifier = Modifier.weight(1f).padding(vertical = 10.dp))
        if (action != null) {
            Box(
                Modifier
                    .alpha(if (enabled) 1f else 0.5f)
                    .clickable(enabled = enabled, role = Role.Button, onClick = onAction)
                    .padding(horizontal = 10.dp, vertical = 10.dp),
            ) {
                BasicText(action.uppercase(), style = retroLabel(9.sp, RetroAmber))
            }
        }
    }
}

@Composable
private fun PanelButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Box(
        modifier
            .clip(RoundedCornerShape(4.dp))
            .background(Color.Black.copy(alpha = 0.5f))
            .border(1.dp, RetroAmber.copy(alpha = 0.5f), RoundedCornerShape(4.dp))
            .clickable(role = Role.Button, onClick = onClick)
            .padding(vertical = 10.dp),
        contentAlignment = Alignment.Center,
    ) {
        BasicText(label.uppercase(), style = retroLabel(9.sp, Color.White), maxLines = 1)
    }
}
