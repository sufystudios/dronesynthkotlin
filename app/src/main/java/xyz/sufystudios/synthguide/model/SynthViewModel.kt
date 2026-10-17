package xyz.sufystudios.synthguide.model

import android.app.Activity
import android.app.Application
import android.content.Context
import android.os.SystemClock
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.compose.runtime.snapshotFlow
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import xyz.sufystudios.synthguide.BuildConfig
import kotlinx.coroutines.isActive
import xyz.sufystudios.synthguide.audio.LooperState
import xyz.sufystudios.synthguide.audio.NativeSynth
import xyz.sufystudios.synthguide.audio.NativeSynth.LooperCommand
import xyz.sufystudios.synthguide.audio.PlaybackService
import xyz.sufystudios.synthguide.billing.Product
import xyz.sufystudios.synthguide.billing.PurchaseManager
import xyz.sufystudios.synthguide.midi.MidiInput

/**
 * The live patch and everything the UI can do to it. Each change is written
 * straight to the native engine and autosaved, so the app reopens as it was left.
 */
class SynthViewModel(application: Application) : AndroidViewModel(application) {
    private val store = PatchStore(application)
    private val uiPrefs = application.getSharedPreferences("ui", Context.MODE_PRIVATE)

    // Debug builds unlock purchases so every feature can be tried on an emulator.
    private val purchases = PurchaseManager(application, viewModelScope, unlockAll = BuildConfig.DEBUG)
    val owned: StateFlow<Set<Product>> = purchases.owned

    private val values = mutableStateMapOf<SynthParam, Float>()
    val presets = mutableStateListOf<Preset>()
    var selectedPresetId by mutableStateOf<String?>(null)
        private set

    /** First octave shown on the keyboard (C of that octave = MIDI 24 + 12 × octave). */
    var keyboardOctave by mutableIntStateOf(uiPrefs.getInt("keyboardOctave", 1))
        private set

    /** The looper's transport, refreshed ~30 times a second. */
    var looper by mutableStateOf(LooperState())
        private set
    val recorder = TakeRecorder(application, viewModelScope)

    private val midi = MidiInput(application)

    private var saveJob: Job? = null
    private val tapTimes = ArrayDeque<Long>()

    init {
        val restored = store.loadCurrent() ?: emptyMap()
        SynthParam.entries.forEach { p -> values[p] = restored[p] ?: p.default }
        // Never reopen stuck in a held drone.
        values[SynthParam.Drone] = 0f
        SynthParam.entries.forEach { pushToEngine(it) }
        presets.addAll(store.loadPresets())
        purchases.connect()
        midi.start()
        viewModelScope.launch {
            while (isActive) {
                looper = NativeSynth.looperState()
                recorder.poll()
                delay(33)
            }
        }
        viewModelScope.launch {
            // Background playback runs exactly while something is sounding.
            snapshotFlow { isSustaining }.distinctUntilChanged().collect { sounding ->
                if (sounding) {
                    if (inForeground) PlaybackService.start(application)
                } else {
                    PlaybackService.stop(application)
                    if (!inForeground) stopAudio()
                }
            }
        }
        viewModelScope.launch {
            PlaybackService.stopRequests.collect { stopAllSound() }
        }
        viewModelScope.launch {
            // Re-apply purchase-gated parameters whenever ownership is (re)confirmed.
            owned.collect { gatedParams.keys.forEach { pushToEngine(it) } }
        }
    }

    // MARK: - Parameters

    operator fun get(param: SynthParam): Float = values[param] ?: param.default
    fun isOn(param: SynthParam): Boolean = get(param) >= 0.5f

    fun set(param: SynthParam, value: Float) {
        val v = param.clamp(value)
        if (values[param] == v) return
        values[param] = v
        pushToEngine(param)
        when (param) {
            // A reverb type sets its own starting crossover/damping, then the decays.
            SynthParam.ReverbType -> {
                val kind = reverbKind
                setQuietly(SynthParam.ReverbLowCutoff, kind.crossoverFrequency)
                setQuietly(SynthParam.ReverbDamping, kind.dampingFrequency)
                updateReverbDecays()
            }
            SynthParam.ReverbSize -> updateReverbDecays()
            else -> Unit
        }
        scheduleSave()
    }

    fun toggle(param: SynthParam) = set(param, if (isOn(param)) 0f else 1f)

    val reverbKind: ReverbKind get() = ReverbKind.entries[get(SynthParam.ReverbType).toInt().coerceIn(0, 3)]

    private fun setQuietly(param: SynthParam, value: Float) {
        values[param] = param.clamp(value)
        pushToEngine(param)
    }

    private fun updateReverbDecays() {
        val size = get(SynthParam.ReverbSize)
        setQuietly(SynthParam.ReverbLowDecay, reverbKind.lowDecay(size))
        setQuietly(SynthParam.ReverbHighDecay, reverbKind.highDecay(size))
    }

    private fun pushToEngine(param: SynthParam) {
        val gate = gatedParams[param]
        val value = if (gate != null && !purchases.owns(gate)) 0f else get(param)
        NativeSynth.setParam(param.id, value)
    }

    // MARK: - Purchases

    /** Effective state of a purchase-gated switch: on only if it's both on and owned. */
    fun isUnlockedAndOn(param: SynthParam): Boolean = isOn(param) && gatedParams[param]?.let(purchases::owns) != false

    /** Toggles a purchase-gated switch, or offers the purchase if it isn't owned yet. */
    fun toggleGated(activity: Activity, param: SynthParam) {
        val product = gatedParams[param] ?: return toggle(param)
        if (!purchases.owns(product)) {
            purchases.purchase(activity, product)
            return
        }
        toggle(param)
    }

    fun refreshPurchases() = purchases.connect()

    // MARK: - Audio and background playback

    /** True while any of the app's screens is visible (see [visibleScreens]). */
    private val inForeground: Boolean
        get() = visibleScreens > 0

    /**
     * True while something should keep sounding with the app in the background:
     * a held drone, the looper playing or recording, or the recorder running.
     */
    val isSustaining: Boolean
        get() = isOn(SynthParam.Drone) || looper.playing || looper.recording ||
            recorder.state.recording || recorder.state.playing

    /** The app became visible: (re)start output. Goes through here so the saved patch is applied first. */
    fun onAppForeground() {
        visibleScreens++
        NativeSynth.start()
    }

    /** The app left the screen: keep sounding only if something is (the service then holds it). */
    fun onAppBackground() {
        visibleScreens = (visibleScreens - 1).coerceAtLeast(0)
        if (!inForeground && !isSustaining) stopAudio()
    }

    private fun stopAudio() {
        NativeSynth.allNotesOff()
        NativeSynth.stop()
    }

    /** The notification's Stop: silence everything that was keeping the app playing. */
    private fun stopAllSound() {
        set(SynthParam.Drone, 0f)
        if (looper.recording) looperCommand(LooperCommand.ToggleRecord)
        if (looper.playing) looperCommand(LooperCommand.TogglePlay)
        recorder.stopTransport()
        NativeSynth.allNotesOff()
        // The looper's state is polled; refresh now so isSustaining reflects the stop.
        looper = looper.copy(playing = false, recording = false, overdubbing = false)
    }

    // MARK: - Looper

    fun looperCommand(command: LooperCommand) = NativeSynth.looperCommand(command)

    fun toggleLooperEnabled() =
        NativeSynth.looperCommand(if (looper.enabled) LooperCommand.Disable else LooperCommand.Enable)

    override fun onCleared() {
        recorder.release()
        midi.stop()
        PlaybackService.stop(getApplication())
        super.onCleared()
    }

    // MARK: - Keyboard

    fun noteOn(note: Int) = NativeSynth.noteOn(note)
    fun noteOff(note: Int) = NativeSynth.noteOff(note)

    /** Moves the keyboard by octaves; [maxOctave] keeps its top key at C8 for however many octaves it shows. */
    fun shiftKeyboard(by: Int, maxOctave: Int) {
        keyboardOctave = (keyboardOctave.coerceAtMost(maxOctave) + by).coerceIn(0, maxOctave)
        uiPrefs.edit().putInt("keyboardOctave", keyboardOctave).apply()
    }

    // MARK: - Delay

    fun delayTimes(): NativeSynth.DelayTimes = NativeSynth.delayTimes(
        get(SynthParam.DelayTime), get(SynthParam.DelaySubdivide), get(SynthParam.DelayTimeLeft),
        get(SynthParam.DelayTimeRight), get(SynthParam.DelayMixMode).toInt(),
    )

    /** Averages the last few taps into the delay's master time (one beat). */
    fun tapTempo() {
        val now = SystemClock.elapsedRealtime()
        if (tapTimes.isNotEmpty() && now - tapTimes.last() > 2_000) tapTimes.clear()
        tapTimes.addLast(now)
        while (tapTimes.size > 5) tapTimes.removeFirst()
        if (tapTimes.size < 2) return
        val intervals = tapTimes.zipWithNext { a, b -> b - a }
        set(SynthParam.DelayTime, intervals.average().toFloat())
    }

    fun resetDelay() = delayParams.forEach { set(it, it.default) }

    fun resetReverb() {
        set(SynthParam.ReverbType, SynthParam.ReverbType.default)
        listOf(SynthParam.ReverbSize, SynthParam.ReverbPredelay, SynthParam.ReverbMix, SynthParam.ReverbBody, SynthParam.ReverbShimmer)
            .forEach { set(it, it.default) }
    }

    // MARK: - Presets

    fun savePreset(name: String) {
        val preset = Preset(store.newPresetId(), name.trim().ifEmpty { "Tape ${presets.size + 1}" }, snapshot())
        presets.add(preset)
        selectedPresetId = preset.id
        persistPresets()
    }

    fun updatePreset(preset: Preset) {
        val i = presets.indexOfFirst { it.id == preset.id }
        if (i < 0) return
        presets[i] = preset.copy(values = snapshot())
        persistPresets()
    }

    fun renamePreset(preset: Preset, name: String) {
        val i = presets.indexOfFirst { it.id == preset.id }
        if (i < 0 || name.isBlank()) return
        presets[i] = preset.copy(name = name.trim())
        persistPresets()
    }

    fun deletePreset(preset: Preset) {
        presets.removeAll { it.id == preset.id }
        if (selectedPresetId == preset.id) selectedPresetId = null
        persistPresets()
    }

    fun loadPreset(preset: Preset) {
        // Type first: it derives the defaults the rest of the reverb then overrides.
        set(SynthParam.ReverbType, preset.values[SynthParam.ReverbType] ?: SynthParam.ReverbType.default)
        SynthParam.entries.filter { it != SynthParam.ReverbType && it != SynthParam.Drone }.forEach { p ->
            setQuietly(p, preset.values[p] ?: p.default)
        }
        selectedPresetId = preset.id
        scheduleSave()
    }

    /** [list] as a preset file, for sharing or backing up. */
    fun presetFileText(list: List<Preset>): String = PresetFile.encode(list)

    /**
     * Adds the presets in a shared or backed-up file to the library, renaming any
     * that clash with an existing name. Returns how many were added, or null if
     * [text] isn't a preset file.
     */
    fun importPresets(text: String): Int? {
        val imported = PresetFile.decode(text) ?: return null
        val names = presets.map { it.name.lowercase() }.toMutableSet()
        imported.forEach { p ->
            var name = p.name
            var n = 2
            while (name.lowercase() in names) name = "${p.name} (${n++})"
            names += name.lowercase()
            presets.add(Preset(store.newPresetId(), name, p.values))
        }
        persistPresets()
        return imported.size
    }

    private fun snapshot(): Map<SynthParam, Float> = SynthParam.entries.associateWith { get(it) }

    private fun persistPresets() {
        val copy = presets.toList()
        viewModelScope.launch(Dispatchers.IO) { store.savePresets(copy) }
    }

    private fun scheduleSave() {
        saveJob?.cancel()
        saveJob = viewModelScope.launch {
            delay(500)
            val copy = snapshot()
            withContext(Dispatchers.IO) { store.saveCurrent(copy) }
        }
    }

    private companion object {
        /**
         * Started screens across the whole app (main thread only). When Android swaps
         * one MainActivity for another, the old one's onStop can arrive after the new
         * one's onStart, so audio stops only once the last visible screen has gone.
         */
        var visibleScreens = 0

        val gatedParams = mapOf(
            SynthParam.StereoFilters to Product.StereoFilters,
        )
        val delayParams = listOf(
            SynthParam.DelayTime, SynthParam.DelaySubdivide, SynthParam.DelayTimeLeft, SynthParam.DelayTimeRight,
            SynthParam.DelayFeedback, SynthParam.DelayWet, SynthParam.DelayDry, SynthParam.DelayWidth,
            SynthParam.DelayMixMode, SynthParam.DelayMedium,
        )
    }
}
