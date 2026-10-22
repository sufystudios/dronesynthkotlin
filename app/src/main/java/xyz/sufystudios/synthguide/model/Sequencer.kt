package xyz.sufystudios.synthguide.model

import org.json.JSONArray
import org.json.JSONObject

/*
 * The ARP/SEQ module: a Grandmother-style arpeggiator and step sequencer, run by
 * the native StepSequencer (shared unchanged with the iOS app). Enum order
 * matches the native enums.
 */

enum class SequencerMode(val label: String) { Arp("ARP"), Seq("SEQ"), Rec("REC") }

enum class ArpOrder(val label: String) { Played("PLAY"), UpDown("UP/DN"), Random("RND") }

enum class StepDivision(val label: String) {
    Quarter("1/4"), Eighth("1/8"), Sixteenth("1/16"),
    QuarterTriplet("1/4T"), EighthTriplet("1/8T"), SixteenthTriplet("1/16T"),
}

data class SequencerSettings(
    val mode: SequencerMode = SequencerMode.Arp,
    val playing: Boolean = false,
    val hold: Boolean = false,
    /** 20..300. */
    val bpm: Double = 120.0,
    val division: StepDivision = StepDivision.Sixteenth,
    /** Share of each step a note sounds, 0.05..1. */
    val gate: Float = 0.5f,
    val order: ArpOrder = ArpOrder.Played,
    /** Octaves played above the held keys, 0..2. */
    val extraOctaves: Int = 0,
    /** 0 until [Sequencer.PATTERNS]. */
    val pattern: Int = 0,
) {
    fun clamped() = copy(
        bpm = bpm.coerceIn(MIN_BPM, MAX_BPM),
        gate = gate.coerceIn(MIN_GATE, 1f),
        extraOctaves = extraOctaves.coerceIn(0, 2),
        pattern = pattern.coerceIn(0, Sequencer.PATTERNS - 1),
    )

    companion object {
        const val MIN_BPM = 20.0
        const val MAX_BPM = 300.0
        const val MIN_GATE = 0.05f
    }
}

data class SequencerStep(val kind: Kind, val note: Int = 0, val velocity: Int = 0) {
    enum class Kind { Rest, Note, Tie }

    /** As the native side takes it: kind | note << 8 | velocity << 16. */
    fun packed(): Int = kind.ordinal or (note.coerceIn(0, 127) shl 8) or (velocity.coerceIn(0, 127) shl 16)

    companion object {
        fun unpacked(v: Int) = SequencerStep(
            Kind.entries[(v and 0xFF).coerceIn(0, 2)], (v shr 8) and 0x7F, (v shr 16) and 0x7F,
        )
    }
}

/** What the sequencer is doing now, polled from the audio thread for the UI. */
data class SequencerState(
    /** The step playing (its index in the arpeggio or pattern), or -1; in REC, the steps recorded. */
    val currentStep: Int = -1,
    val running: Boolean = false,
    /** Clock pulses so far, playing or not: the RATE LED flashes when it changes. */
    val clockTicks: Int = 0,
    /** The selected pattern's steps (empty in ARP). */
    val pattern: List<SequencerStep> = emptyList(),
)

/**
 * The module as saved in a patch: settings plus all three patterns. Stored under a
 * preset's "sequencer" key with the iOS app's SequencerPreset JSON keys, so the
 * two apps read each other's ARP/SEQ.
 */
data class SequencerPreset(
    val settings: SequencerSettings = SequencerSettings(),
    val patterns: List<List<SequencerStep>> = List(Sequencer.PATTERNS) { emptyList() },
) {
    fun toJson(): JSONObject {
        val s = settings
        val patternsJson = JSONArray()
        patterns.forEach { steps ->
            val list = JSONArray()
            steps.forEach { list.put(JSONObject().put("kind", it.kind.ordinal).put("note", it.note).put("velocity", it.velocity)) }
            patternsJson.put(list)
        }
        return JSONObject()
            // Never REC: loading can't leave the sequencer armed to overwrite a pattern.
            .put("mode", (if (s.mode == SequencerMode.Rec) SequencerMode.Seq else s.mode).ordinal)
            .put("playing", s.playing)
            .put("hold", s.hold)
            .put("bpm", s.bpm)
            .put("division", s.division.ordinal)
            .put("gate", s.gate.toDouble())
            .put("order", s.order.ordinal)
            .put("extraOctaves", s.extraOctaves)
            .put("pattern", s.pattern)
            .put("patterns", patternsJson)
    }

    companion object {
        /** Every field is optional: missing ones take a fresh synth's values. */
        fun fromJson(o: JSONObject?): SequencerPreset {
            if (o == null) return SequencerPreset()
            val d = SequencerSettings()
            fun <T> pick(entries: List<T>, key: String, default: T): T =
                entries.getOrNull(o.optInt(key, entries.indexOf(default))) ?: default
            val mode = pick(SequencerMode.entries, "mode", d.mode)
            val settings = SequencerSettings(
                mode = if (mode == SequencerMode.Rec) SequencerMode.Seq else mode,
                playing = o.optBoolean("playing", d.playing),
                hold = o.optBoolean("hold", d.hold),
                bpm = o.optDouble("bpm", d.bpm).takeUnless { it.isNaN() } ?: d.bpm,
                division = pick(StepDivision.entries, "division", d.division),
                gate = o.optDouble("gate", d.gate.toDouble()).toFloat().takeUnless { it.isNaN() } ?: d.gate,
                order = pick(ArpOrder.entries, "order", d.order),
                extraOctaves = o.optInt("extraOctaves", d.extraOctaves),
                pattern = o.optInt("pattern", d.pattern),
            ).clamped()
            val list = o.optJSONArray("patterns")
            val patterns = List(Sequencer.PATTERNS) { i ->
                val steps = list?.optJSONArray(i) ?: return@List emptyList()
                (0 until minOf(steps.length(), Sequencer.MAX_STEPS)).mapNotNull { j ->
                    val step = steps.optJSONObject(j) ?: return@mapNotNull null
                    SequencerStep(
                        SequencerStep.Kind.entries.getOrNull(step.optInt("kind")) ?: SequencerStep.Kind.Rest,
                        step.optInt("note").coerceIn(0, 127),
                        step.optInt("velocity").coerceIn(0, 127),
                    )
                }
            }
            return SequencerPreset(settings, patterns)
        }
    }
}

object Sequencer {
    const val PATTERNS = 3
    const val MAX_STEPS = 256
}
