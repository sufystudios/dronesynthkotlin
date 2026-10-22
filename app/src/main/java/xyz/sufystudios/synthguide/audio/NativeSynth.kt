package xyz.sufystudios.synthguide.audio

import xyz.sufystudios.synthguide.model.SequencerSettings
import xyz.sufystudios.synthguide.model.SequencerState
import xyz.sufystudios.synthguide.model.SequencerStep

/**
 * The native engine (app/src/main/cpp/synth). One instance per process.
 *
 * Every call is cheap and safe from the main thread: parameters are atomics and
 * note events go through a lock-free queue to the audio thread.
 */
/** The looper's transport, as shown on its drawer and rail tab. */
data class LooperState(
    val hasLoop: Boolean = false,
    val recording: Boolean = false,
    val playing: Boolean = false,
    val overdubbing: Boolean = false,
    val canUndo: Boolean = false,
    val enabled: Boolean = true,
    val lengthSeconds: Float = 0f,
    val positionSeconds: Float = 0f,
    val recordedSeconds: Float = 0f,
)

object NativeSynth {
    init {
        System.loadLibrary("dronesynth")
        nativeCreate()
    }

    fun start(): Boolean = nativeStart()
    fun stop() = nativeStop()
    fun setParam(id: Int, value: Float) = nativeSetParam(id, value)
    fun noteOn(note: Int, velocity: Int = 127) = nativeNoteOn(note, velocity)
    fun noteOff(note: Int) = nativeNoteOff(note)

    /** Performance controls (MIDI); never stored in presets. Ids match Params.h. */
    fun setPitchBend(value: Float) = nativeSetParam(110, value.coerceIn(-1f, 1f))
    fun setModWheel(value: Float) = nativeSetParam(111, value.coerceIn(0f, 1f))
    fun allNotesOff() = nativeAllNotesOff()

    /** Peak output level since the last call, 0..1. */
    fun takePeak(): Float = nativeTakePeak()

    /** The delay's per-channel times exactly as the kernel will play them. */
    fun delayTimes(timeMs: Float, subdivide: Float, timeLeft: Float, timeRight: Float, mixMode: Int): DelayTimes {
        val v = nativeDelayTimes(timeMs, subdivide, timeLeft, timeRight, mixMode)
        return DelayTimes(leftSeconds = v[0], rightSeconds = v[1], clamped = v[2] > 0.5f)
    }

    data class DelayTimes(val leftSeconds: Float, val rightSeconds: Float, val clamped: Boolean)

    // MARK: - Looper

    /** Commands, in the same order as the native Looper::Command enum. */
    enum class LooperCommand { ToggleRecord, TogglePlay, ToggleOverdub, Undo, Clear, Enable, Disable }

    fun looperCommand(command: LooperCommand) = nativeLooperCommand(command.ordinal)

    fun looperState(): LooperState {
        val v = nativeLooperState()
        val rate = sampleRate().toFloat()
        return LooperState(
            hasLoop = v[0] and 1 != 0,
            recording = v[0] and 2 != 0,
            playing = v[0] and 4 != 0,
            overdubbing = v[0] and 8 != 0,
            canUndo = v[0] and 16 != 0,
            enabled = v[0] and 32 != 0,
            lengthSeconds = v[1] / rate,
            positionSeconds = v[2] / rate,
            recordedSeconds = v[3] / rate,
        )
    }

    // MARK: - Recorder

    /** Starts a new take, written as a WAV file at [path]. */
    fun recorderStart(path: String): Boolean = nativeRecorderStart(path)

    /** Stops and finalises the take. Blocks briefly while the file is flushed - call off the main thread. */
    fun recorderStop() = nativeRecorderStop()

    /** (recording, open, seconds recorded). A take is "open" until [recorderStop] finalises it. */
    fun recorderState(): Triple<Boolean, Boolean, Float> {
        val v = nativeRecorderState()
        return Triple(v[0] != 0L, v[1] != 0L, v[2] / sampleRate().toFloat())
    }

    // MARK: - ARP/SEQ

    fun sequencerSettings(s: SequencerSettings) = nativeSequencerSettings(
        s.mode.ordinal, s.playing, s.hold, s.bpm, s.division.ordinal, s.gate, s.order.ordinal, s.extraOctaves, s.pattern,
    )

    /** REC's REST/TIE button: a rest, or a tie while a key is held. */
    fun sequencerRestOrTie() = nativeSequencerRestOrTie()
    fun sequencerClearPattern(pattern: Int) = nativeSequencerClearPattern(pattern)
    fun sequencerSetPattern(pattern: Int, steps: List<SequencerStep>) =
        nativeSequencerSetPattern(pattern, IntArray(steps.size) { steps[it].packed() })

    /** A pattern as it stands, steps recorded in REC included. */
    fun sequencerPattern(pattern: Int): List<SequencerStep> = nativeSequencerPattern(pattern).map(SequencerStep::unpacked)

    /** The playing state, plus [pattern]'s steps if it's given. */
    fun sequencerState(pattern: Int?): SequencerState {
        val v = nativeSequencerState()
        return SequencerState(v[0], v[1] != 0, v[2], pattern?.let(::sequencerPattern) ?: emptyList())
    }

    fun sampleRate(): Int = nativeSampleRate()

    private external fun nativeCreate()
    private external fun nativeStart(): Boolean
    private external fun nativeStop()
    private external fun nativeSetParam(id: Int, value: Float)
    private external fun nativeNoteOn(note: Int, velocity: Int)
    private external fun nativeNoteOff(note: Int)
    private external fun nativeAllNotesOff()
    private external fun nativeTakePeak(): Float
    private external fun nativeLooperCommand(command: Int)
    private external fun nativeLooperState(): IntArray
    private external fun nativeRecorderStart(path: String): Boolean
    private external fun nativeRecorderStop()
    private external fun nativeRecorderState(): LongArray
    private external fun nativeSampleRate(): Int
    private external fun nativeSequencerSettings(
        mode: Int, playing: Boolean, hold: Boolean, bpm: Double, division: Int, gate: Float, order: Int, extraOctaves: Int, pattern: Int,
    )
    private external fun nativeSequencerRestOrTie()
    private external fun nativeSequencerClearPattern(pattern: Int)
    private external fun nativeSequencerSetPattern(pattern: Int, packed: IntArray)
    private external fun nativeSequencerPattern(pattern: Int): IntArray
    private external fun nativeSequencerState(): IntArray
    private external fun nativeDelayTimes(timeMs: Float, subdivide: Float, timeLeft: Float, timeRight: Float, mixMode: Int): FloatArray
}
