package xyz.sufystudios.synthguide.midi

import android.bluetooth.BluetoothDevice
import android.content.Context
import android.content.pm.PackageManager
import android.media.midi.MidiDevice
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.media.midi.MidiOutputPort
import android.media.midi.MidiReceiver
import android.os.Build
import android.os.Handler
import android.os.Looper
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import xyz.sufystudios.synthguide.audio.NativeSynth

/** A connected MIDI device; [bluetoothAddress] is set for Bluetooth LE ones, which can be disconnected. */
data class MidiDeviceEntry(val name: String, val bluetoothAddress: String? = null)

/**
 * Plays the synth from MIDI keyboards: every connected device's output ports are
 * opened as devices come and go - USB, and Bluetooth LE keyboards connected
 * through [bluetooth]. Omni - all channels play.
 */
class MidiInput(context: Context) {
    private val manager: MidiManager? =
        if (context.packageManager.hasSystemFeature(PackageManager.FEATURE_MIDI)) context.getSystemService(MidiManager::class.java) else null
    private val handler = Handler(Looper.getMainLooper())
    private val open = mutableMapOf<Int, Pair<MidiDevice, List<MidiOutputPort>>>()
    private val entries = mutableMapOf<Int, MidiDeviceEntry>()

    private val _devices = MutableStateFlow<List<MidiDeviceEntry>>(emptyList())

    /** The connected MIDI devices. */
    val devices: StateFlow<List<MidiDeviceEntry>> = _devices.asStateFlow()

    /** Finds and connects Bluetooth LE MIDI keyboards; null without MIDI support. */
    val bluetooth: BluetoothMidi? = manager?.let { BluetoothMidi(context, it, handler, onDisconnect = ::closeBluetooth) }

    private val callback = object : MidiManager.DeviceCallback() {
        override fun onDeviceAdded(info: MidiDeviceInfo) = openDevice(info)
        override fun onDeviceRemoved(info: MidiDeviceInfo) = closeDevice(info.id)
    }

    @Suppress("DEPRECATION") // The executor/transport variants need API 33; these work on all versions.
    fun start() {
        val manager = manager ?: return
        manager.devices.forEach(::openDevice)
        manager.registerDeviceCallback(callback, handler)
        bluetooth?.reconnectRemembered()
    }

    fun stop() {
        manager?.unregisterDeviceCallback(callback)
        open.keys.toList().forEach(::closeDevice)
        bluetooth?.close()
    }

    private fun openDevice(info: MidiDeviceInfo) {
        // Output ports are where a keyboard sends its notes.
        if (info.outputPortCount == 0 || info.id in open) return
        manager?.openDevice(info, { device ->
            if (device == null) return@openDevice
            val ports = (0 until info.outputPortCount).mapNotNull { device.openOutputPort(it) }
            ports.forEach { it.connect(MidiParser()) }
            open[info.id] = device to ports
            val bluetoothDevice = info.properties.getParcelableCompat<BluetoothDevice>(MidiDeviceInfo.PROPERTY_BLUETOOTH_DEVICE)
            entries[info.id] = MidiDeviceEntry(
                info.properties.getString(MidiDeviceInfo.PROPERTY_NAME) ?: "MIDI device",
                bluetoothDevice?.address,
            )
            publish()
        }, handler)
    }

    private fun closeDevice(id: Int) {
        val (device, ports) = open.remove(id) ?: return
        ports.forEach { runCatching { it.close() } }
        runCatching { device.close() }
        entries.remove(id)
        NativeSynth.allNotesOff()
        publish()
    }

    /** Drops this side's handles on a Bluetooth device too, so its link actually closes. */
    private fun closeBluetooth(address: String) {
        entries.filterValues { it.bluetoothAddress == address }.keys.toList().forEach(::closeDevice)
    }

    private fun publish() {
        _devices.value = entries.values.toList()
    }
}

@Suppress("DEPRECATION")
private inline fun <reified T : android.os.Parcelable> android.os.Bundle.getParcelableCompat(key: String): T? =
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) getParcelable(key, T::class.java) else getParcelable(key)

/** Where parsed MIDI goes. The app's sink is the native synth; tests use a fake. */
interface MidiSink {
    fun noteOn(note: Int, velocity: Int)
    fun noteOff(note: Int)
    fun pitchBend(value: Float)   // -1..1
    fun modWheel(value: Float)    // 0..1
    fun allNotesOff()
}

private object SynthSink : MidiSink {
    override fun noteOn(note: Int, velocity: Int) = NativeSynth.noteOn(note, velocity)
    override fun noteOff(note: Int) = NativeSynth.noteOff(note)
    override fun pitchBend(value: Float) = NativeSynth.setPitchBend(value)
    override fun modWheel(value: Float) = NativeSynth.setModWheel(value)
    override fun allNotesOff() = NativeSynth.allNotesOff()
}

private class MidiParser : MidiReceiver() {
    private val parser = MidiByteParser(SynthSink)
    override fun onSend(msg: ByteArray, offset: Int, count: Int, timestamp: Long) {
        for (i in offset until offset + count) parser.handle(msg[i].toInt() and 0xFF)
    }
}

/** Turns a MIDI byte stream into [MidiSink] calls (running status, realtime and SysEx skipped). */
class MidiByteParser(private val sink: MidiSink) {
    private var status = 0
    private var data1 = -1
    private var inSysEx = false

    fun handle(byte: Int) {
        when {
            byte >= 0xF8 -> return                                  // realtime: clock, start/stop...
            byte == 0xF0 -> { inSysEx = true; return }
            byte == 0xF7 -> { inSysEx = false; return }
            inSysEx -> return
            byte >= 0xF0 -> { status = 0; return }                  // other system common
            byte >= 0x80 -> { status = byte; data1 = -1; return }   // new status
        }
        if (status == 0) return
        val type = status and 0xF0
        val needsTwo = type != 0xC0 && type != 0xD0
        if (needsTwo && data1 < 0) {
            data1 = byte
            return
        }
        dispatch(type, if (needsTwo) data1 else byte, byte)
        data1 = -1  // running status: the next data byte starts a new message
    }

    private fun dispatch(type: Int, d1: Int, d2: Int) {
        when (type) {
            0x80 -> sink.noteOff(d1)
            0x90 -> if (d2 == 0) sink.noteOff(d1) else sink.noteOn(d1, d2)
            0xB0 -> when (d1) {
                1 -> sink.modWheel(d2 / 127f)                 // mod wheel brightens the filter
                120, 123 -> sink.allNotesOff()                // all sound / all notes off
            }
            0xE0 -> sink.pitchBend((((d2 shl 7) or d1) - 8192) / 8192f)
        }
    }
}
