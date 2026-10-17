package xyz.sufystudios.synthguide.midi

import android.Manifest
import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.pm.PackageManager
import android.location.LocationManager
import android.media.midi.MidiDevice
import android.media.midi.MidiManager
import android.os.Build
import android.os.Handler
import android.os.ParcelUuid
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.update
import java.util.UUID

/** A Bluetooth MIDI keyboard seen by a scan. */
data class FoundKeyboard(val address: String, val name: String)

/**
 * Finds Bluetooth LE MIDI keyboards (they advertise the BLE-MIDI service) and
 * connects them through Android's MIDI service. Once connected, a keyboard shows
 * up as an ordinary MIDI device, which [MidiInput] then plays from. Connected
 * keyboards are remembered and reconnected when the app starts.
 */
class BluetoothMidi(
    context: Context,
    private val manager: MidiManager,
    private val handler: Handler,
    private val onDisconnect: (address: String) -> Unit,
) {
    private val context = context.applicationContext
    private val adapter: BluetoothAdapter? =
        if (context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            context.getSystemService(BluetoothManager::class.java)?.adapter
        } else null
    private val prefs = context.getSharedPreferences("midi", Context.MODE_PRIVATE)
    private val links = mutableMapOf<String, MidiDevice>()

    private val _scanning = MutableStateFlow(false)
    val scanning: StateFlow<Boolean> = _scanning.asStateFlow()

    private val _found = MutableStateFlow<List<FoundKeyboard>>(emptyList())

    /** Keyboards seen by the current or last scan. */
    val found: StateFlow<List<FoundKeyboard>> = _found.asStateFlow()

    private val _connecting = MutableStateFlow<Set<String>>(emptySet())

    /** Addresses being connected right now. */
    val connecting: StateFlow<Set<String>> = _connecting.asStateFlow()

    /** Whether this device has Bluetooth LE at all. */
    val isSupported: Boolean get() = adapter != null

    val isEnabled: Boolean get() = adapter?.isEnabled == true

    /** What a scan needs: nearby-devices on Android 12+, location before that. */
    val requiredPermissions: Array<String> =
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            arrayOf(Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT)
        } else {
            arrayOf(Manifest.permission.ACCESS_FINE_LOCATION)
        }

    fun hasPermissions(): Boolean =
        requiredPermissions.all { context.checkSelfPermission(it) == PackageManager.PERMISSION_GRANTED }

    /** Before Android 12, scans only return results while location is switched on. */
    val needsLocationOn: Boolean
        get() {
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) return false
            val location = context.getSystemService(LocationManager::class.java) ?: return false
            val on = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
                location.isLocationEnabled
            } else {
                location.isProviderEnabled(LocationManager.GPS_PROVIDER) || location.isProviderEnabled(LocationManager.NETWORK_PROVIDER)
            }
            return !on
        }

    private val scanCallback = object : ScanCallback() {
        @SuppressLint("MissingPermission") // Only running when the scan permissions were granted.
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val address = result.device.address
            val name = result.scanRecord?.deviceName ?: runCatching { result.device.name }.getOrNull() ?: address
            _found.update { list -> if (list.any { it.address == address }) list else list + FoundKeyboard(address, name) }
        }

        override fun onScanFailed(errorCode: Int) {
            _scanning.value = false
        }
    }

    private val stopScanLater = Runnable { stopScan() }

    @SuppressLint("MissingPermission") // Checked by hasPermissions().
    fun startScan() {
        val scanner = adapter?.takeIf { it.isEnabled }?.bluetoothLeScanner ?: return
        if (!hasPermissions() || _scanning.value) return
        _found.value = emptyList()
        val filter = ScanFilter.Builder().setServiceUuid(ParcelUuid(MIDI_SERVICE)).build()
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        runCatching { scanner.startScan(listOf(filter), settings, scanCallback) }.onFailure { return }
        _scanning.value = true
        handler.postDelayed(stopScanLater, SCAN_MILLIS)
    }

    @SuppressLint("MissingPermission")
    fun stopScan() {
        handler.removeCallbacks(stopScanLater)
        if (!_scanning.value) return
        _scanning.value = false
        if (hasPermissions()) runCatching { adapter?.bluetoothLeScanner?.stopScan(scanCallback) }
    }

    /** Connects a keyboard, and remembers it for next time. */
    fun connect(address: String) {
        stopScan()
        open(address)
        remembered = remembered + address
    }

    /** Disconnects a keyboard and stops reconnecting it. */
    fun disconnect(address: String) {
        remembered = remembered - address
        _connecting.update { it - address }
        links.remove(address)?.let { runCatching { it.close() } }
        onDisconnect(address)
    }

    /** Reconnects the keyboards connected last time, if they're switched on and in range. */
    fun reconnectRemembered() {
        if (!isEnabled || !hasPermissions()) return
        remembered.forEach(::open)
    }

    fun close() {
        stopScan()
        links.values.forEach { runCatching { it.close() } }
        links.clear()
    }

    private fun open(address: String) {
        val adapter = adapter ?: return
        if (address in links || address in _connecting.value || !hasPermissions()) return
        val device = runCatching { adapter.getRemoteDevice(address) }.getOrNull() ?: return
        _connecting.update { it + address }
        manager.openBluetoothDevice(device, { midiDevice ->
            val wanted = address in _connecting.value
            _connecting.update { it - address }
            when {
                midiDevice == null -> Unit
                wanted -> links[address] = midiDevice
                else -> runCatching { midiDevice.close() } // disconnected while it was connecting
            }
        }, handler)
    }

    private var remembered: Set<String>
        get() = prefs.getStringSet(KEY_REMEMBERED, emptySet()).orEmpty()
        set(value) = prefs.edit().putStringSet(KEY_REMEMBERED, value).apply()

    private companion object {
        /** The standard BLE-MIDI service every Bluetooth MIDI keyboard advertises. */
        val MIDI_SERVICE: UUID = UUID.fromString("03B80E5A-EDE8-4B33-A751-6CE34EC4C700")
        const val SCAN_MILLIS = 15_000L
        const val KEY_REMEMBERED = "bluetoothKeyboards"
    }
}
