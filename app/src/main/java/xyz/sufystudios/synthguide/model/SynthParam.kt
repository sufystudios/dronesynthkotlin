package xyz.sufystudios.synthguide.model

import kotlin.math.pow

/** How a knob's 0..1 position maps onto a parameter's range. */
enum class Taper {
    /** Straight line. */
    Linear,

    /** Exponential - for frequencies and times, matching how we hear them. */
    Log,

    /** Squared - more resolution in the lower, more musical part of the range. */
    Curved,
}

/**
 * Every engine parameter. [id] matches the native ParamId (Params.h) and the iOS
 * app's ParameterAddress, and is permanent: never renumber or reuse one. [key] is
 * what presets store.
 */
enum class SynthParam(
    val id: Int,
    val key: String,
    val min: Float,
    val max: Float,
    val default: Float,
    val taper: Taper = Taper.Linear,
) {
    // Filter
    Cutoff(0, "cutoff", 40f, 20_000f, 200f, Taper.Log),
    LeftResonance(1, "leftResonance", 0f, 0.99f, 0.5f),
    RightResonance(2, "rightResonance", 0f, 0.99f, 0.5f),
    StereoFilters(3, "stereoFiltersEnabled", 0f, 1f, 0f),
    FilterEnvEnabled(4, "filterEnvelopeModEnabled", 0f, 1f, 0f),
    FilterEnvAmount(5, "filterEnvelopeModAmount", 0f, 1f, 0.5f),

    // Envelope
    Attack(6, "attack", 0.002f, 10f, 0.1f, Taper.Log),
    Decay(7, "decay", 0.002f, 10f, 0.1f, Taper.Log),
    Release(8, "release", 0.002f, 10f, 0.1f, Taper.Log),
    Sustain(9, "sustain", 0f, 1f, 1f),
    Glide(10, "glideTime", 0f, 2f, 0.1f),
    Drone(11, "isDroneMode", 0f, 1f, 0f),

    // Oscillator 1
    Osc1Waveform(12, "osc1Waveform", 0f, 4f, OscWaveform.Saw.ordinal.toFloat()),
    Osc1Volume(13, "osc1Volume", 0f, 1f, 0.8f),
    Osc1Octave(14, "osc1Octave", -3f, 3f, 0f),
    Osc1Semitone(15, "osc1Semitone", -12f, 12f, 0f),
    Osc1Fine(16, "osc1Fine", -100f, 100f, 0f),
    Osc1VibratoEnabled(17, "osc1VibratoEnabled", 0f, 1f, 0f),
    Osc1VibratoRate(18, "osc1VibratoRate", 0.05f, 20f, 4f, Taper.Log),
    Osc1VibratoDepth(19, "osc1VibratoDepth", 0f, 1f, 0.2f),

    // Oscillator 2
    Osc2Waveform(20, "osc2Waveform", 0f, 4f, OscWaveform.Saw.ordinal.toFloat()),
    Osc2Volume(21, "osc2Volume", 0f, 1f, 0.35f),
    Osc2Octave(22, "osc2Octave", -3f, 3f, 0f),
    Osc2Semitone(23, "osc2Semitone", -12f, 12f, 0f),
    Osc2Fine(24, "osc2Fine", -100f, 100f, 8f),
    Osc2VibratoEnabled(25, "osc2VibratoEnabled", 0f, 1f, 0f),
    Osc2VibratoRate(26, "osc2VibratoRate", 0.05f, 20f, 4f, Taper.Log),
    Osc2VibratoDepth(27, "osc2VibratoDepth", 0f, 1f, 0.2f),

    // Oscillator 3 (its Pitch knob sweeps ±700 cents rather than ±100)
    Osc3Waveform(28, "osc3Waveform", 0f, 4f, OscWaveform.Sine.ordinal.toFloat()),
    Osc3Volume(29, "osc3Volume", 0f, 1f, 0f),
    Osc3Octave(30, "osc3Octave", -3f, 3f, 1f),
    Osc3Semitone(31, "osc3Semitone", -12f, 12f, 0f),
    Osc3Fine(32, "osc3Fine", -700f, 700f, 0f),
    Osc3VibratoEnabled(33, "osc3VibratoEnabled", 0f, 1f, 0f),
    Osc3VibratoRate(34, "osc3VibratoRate", 0.05f, 20f, 4f, Taper.Log),
    Osc3VibratoDepth(35, "osc3VibratoDepth", 0f, 1f, 0.2f),
    Osc3SyncToOsc1(36, "osc3SyncToOsc1", 0f, 1f, 0f),

    // LFO: Filter
    FilterLfoShape(37, "filterLFOShape", 0f, 3f, 0f),
    FilterLfoRate(38, "filterLFORate", 0.05f, 20f, 2f, Taper.Log),
    FilterLfoRandomize(39, "filterLFORandomize", 0f, 1f, 0.3f),
    FilterLfoEnabled(40, "filterLFOEnabled", 0f, 1f, 0f),
    FilterLfoDepth(41, "filterLFODepth", 0f, 1f, 0.4f),

    // LFO: Pitch OSC 1 & 2
    PitchLfoOsc12Shape(42, "pitchLFOOsc12Shape", 0f, 3f, 0f),
    PitchLfoOsc12Rate(43, "pitchLFOOsc12Rate", 0.05f, 20f, 2f, Taper.Log),
    PitchLfoOsc12Randomize(44, "pitchLFOOsc12Randomize", 0f, 1f, 0.3f),
    PitchLfoOsc12Enabled(45, "pitchLFOOsc12Enabled", 0f, 1f, 0f),
    PitchLfoOsc12Depth(46, "pitchLFOOsc12Depth", 0f, 1f, 0.2f),

    // LFO: Pitch OSC 3
    PitchLfoOsc3Shape(47, "pitchLFOOsc3Shape", 0f, 3f, 0f),
    PitchLfoOsc3Rate(48, "pitchLFOOsc3Rate", 0.05f, 20f, 2f, Taper.Log),
    PitchLfoOsc3Randomize(49, "pitchLFOOsc3Randomize", 0f, 1f, 0.3f),
    PitchLfoOsc3Enabled(50, "pitchLFOOsc3Enabled", 0f, 1f, 0f),
    PitchLfoOsc3Depth(51, "pitchLFOOsc3Depth", 0f, 1f, 0.2f),

    // Reverb (52..56 are retired iOS delay addresses)
    ReverbType(57, "reverbType", 0f, 3f, ReverbKind.Hall.ordinal.toFloat()),
    ReverbLowCutoff(58, "reverbLowCutoff", 50f, 1_000f, ReverbKind.Hall.crossoverFrequency, Taper.Log),
    ReverbDamping(59, "reverbDamping", 1_500f, 20_000f, ReverbKind.Hall.dampingFrequency, Taper.Log),
    ReverbLowDecay(60, "reverbLowDecay", 1f, 8f, ReverbKind.Hall.lowDecay(0.5f)),
    ReverbHighDecay(61, "reverbHighDecay", 1f, 8f, ReverbKind.Hall.highDecay(0.5f)),
    ReverbSize(62, "reverbSize", 0f, 1f, 0.5f),
    ReverbPredelay(63, "reverbPredelay", 0f, 1f, 0.5f),
    ReverbMix(64, "reverbMix", 0f, 0.9f, 0f),
    ReverbBody(65, "reverbBodyLevel", -15f, 15f, 0f),
    ReverbShimmer(66, "reverbShimmerLevel", -15f, 15f, 0f),

    // Vintage delay
    DelayTime(67, "delayMasterTime", 10f, 2_000f, 500f, Taper.Log),
    DelaySubdivide(68, "delaySubdivide", 1f, 16f, 4f),
    DelayTimeLeft(69, "delayTimeLeft", 1f, 16f, 3f),
    DelayTimeRight(70, "delayTimeRight", 1f, 16f, 5f),
    DelayFeedback(71, "delayRepeatFeedback", 0f, 1f, 0.5f, Taper.Curved),
    DelayWet(72, "delayWet", 0f, 4f, 0f, Taper.Curved),
    DelayDry(73, "delayDry", 0f, 4f, 1f, Taper.Curved),
    DelayWidth(74, "delayWidth", -1f, 1f, 1f),
    DelayMixMode(75, "delayMixMode", 0f, 3f, DelayMix.PingPong.ordinal.toFloat()),
    DelayMedium(76, "delayMedium", 0f, 2f, DelayMediumKind.Tape.ordinal.toFloat());

    // 100 and 101 were the Android-only tube stage (retired - never reuse).

    fun clamp(value: Float): Float = value.coerceIn(min, max)

    /** Knob position (0..1) for a real value, following [taper]. */
    fun toPosition(value: Float): Float {
        val v = clamp(value)
        return when (taper) {
            Taper.Linear -> (v - min) / (max - min)
            Taper.Log -> (kotlin.math.ln(v / min) / kotlin.math.ln(max / min))
            Taper.Curved -> ((v - min) / (max - min)).coerceAtLeast(0f).pow(0.5f)
        }.coerceIn(0f, 1f)
    }

    /** Real value for a knob position (0..1), following [taper]. */
    fun fromPosition(position: Float): Float {
        val t = position.coerceIn(0f, 1f)
        return clamp(
            when (taper) {
                Taper.Linear -> min + t * (max - min)
                Taper.Log -> min * (max / min).pow(t)
                Taper.Curved -> min + t * t * (max - min)
            }
        )
    }

    companion object {
        private val byKey = entries.associateBy { it.key }
        fun fromKey(key: String): SynthParam? = byKey[key]
    }
}

/** The five waveforms per oscillator, in the engine's (and iOS's) order. */
enum class OscWaveform(val label: String) { Sine("Sine"), Triangle("Tri"), Square("Sqr"), Saw("Saw"), Pink("Pink") }

/** LFO shapes, in the engine's order. */
enum class LfoShape(val label: String) { Sine("Sine"), Triangle("Tri"), Square("Square"), Random("Random") }

/** Reverb types: each reshapes the reverb's ranges, as on iOS. */
enum class ReverbKind(
    val label: String,
    private val releaseTimeScale: Float,
    val crossoverFrequency: Float,
    val dampingFrequency: Float,
) {
    Room("Room", 0.4f, 200f, 10_000f),
    Hall("Hall", 1.0f, 200f, 16_000f),
    Plate("Plate", 0.6f, 50f, 20_000f),
    Cathedral("Cathedral", 1.6f, 400f, 14_000f);

    fun lowDecay(size: Float): Float = (releaseTimeScale * 8f.pow(size)).coerceIn(1f, 8f)
    fun highDecay(size: Float): Float = (releaseTimeScale * 7f.pow(size)).coerceIn(1f, 8f)
}

/** Vintage delay mix modes, in Calf's (and the kernel's) order. */
enum class DelayMix(val label: String) { Stereo("Stereo"), PingPong("Ping-Pong"), LeftThenRight("L then R"), RightThenLeft("R then L") }

/** Vintage delay media, in Calf's (and the kernel's) order. */
enum class DelayMediumKind(val label: String) { Plain("Plain"), Tape("Tape"), OldTape("Old Tape") }

/** The three oscillator slots' parameters, so UI code can address a slot by index. */
data class OscSlotParams(
    val label: String,
    val waveform: SynthParam,
    val volume: SynthParam,
    val octave: SynthParam,
    val semitone: SynthParam,
    val fine: SynthParam,
) {
    companion object {
        val all = listOf(
            OscSlotParams("OSC 1", SynthParam.Osc1Waveform, SynthParam.Osc1Volume, SynthParam.Osc1Octave, SynthParam.Osc1Semitone, SynthParam.Osc1Fine),
            OscSlotParams("OSC 2", SynthParam.Osc2Waveform, SynthParam.Osc2Volume, SynthParam.Osc2Octave, SynthParam.Osc2Semitone, SynthParam.Osc2Fine),
            OscSlotParams("OSC 3", SynthParam.Osc3Waveform, SynthParam.Osc3Volume, SynthParam.Osc3Octave, SynthParam.Osc3Semitone, SynthParam.Osc3Fine),
        )
    }
}

/** The three LFOs' parameters. */
data class LfoParams(
    val label: String,
    val shape: SynthParam,
    val rate: SynthParam,
    val randomize: SynthParam,
    val enabled: SynthParam,
    val depth: SynthParam,
) {
    companion object {
        val all = listOf(
            LfoParams("Filter", SynthParam.FilterLfoShape, SynthParam.FilterLfoRate, SynthParam.FilterLfoRandomize, SynthParam.FilterLfoEnabled, SynthParam.FilterLfoDepth),
            LfoParams("OSC 1 & 2", SynthParam.PitchLfoOsc12Shape, SynthParam.PitchLfoOsc12Rate, SynthParam.PitchLfoOsc12Randomize, SynthParam.PitchLfoOsc12Enabled, SynthParam.PitchLfoOsc12Depth),
            LfoParams("OSC 3", SynthParam.PitchLfoOsc3Shape, SynthParam.PitchLfoOsc3Rate, SynthParam.PitchLfoOsc3Randomize, SynthParam.PitchLfoOsc3Enabled, SynthParam.PitchLfoOsc3Depth),
        )
    }
}
