package xyz.sufystudios.synthguide.audio

/**
 * The native engine (app/src/main/cpp/synth). One instance per process.
 *
 * Every call is cheap and safe from the main thread: parameters are atomics and
 * note events go through a lock-free queue to the audio thread.
 */
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
    fun allNotesOff() = nativeAllNotesOff()

    /** Peak output level since the last call, 0..1. */
    fun takePeak(): Float = nativeTakePeak()

    /** The delay's per-channel times exactly as the kernel will play them. */
    fun delayTimes(timeMs: Float, subdivide: Float, timeLeft: Float, timeRight: Float, mixMode: Int): DelayTimes {
        val v = nativeDelayTimes(timeMs, subdivide, timeLeft, timeRight, mixMode)
        return DelayTimes(leftSeconds = v[0], rightSeconds = v[1], clamped = v[2] > 0.5f)
    }

    data class DelayTimes(val leftSeconds: Float, val rightSeconds: Float, val clamped: Boolean)

    private external fun nativeCreate()
    private external fun nativeStart(): Boolean
    private external fun nativeStop()
    private external fun nativeSetParam(id: Int, value: Float)
    private external fun nativeNoteOn(note: Int, velocity: Int)
    private external fun nativeNoteOff(note: Int)
    private external fun nativeAllNotesOff()
    private external fun nativeTakePeak(): Float
    private external fun nativeDelayTimes(timeMs: Float, subdivide: Float, timeLeft: Float, timeRight: Float, mixMode: Int): FloatArray
}
