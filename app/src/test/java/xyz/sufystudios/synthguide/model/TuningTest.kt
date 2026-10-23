package xyz.sufystudios.synthguide.model

import org.junit.Assert.assertEquals
import org.junit.Test
import xyz.sufystudios.synthguide.ui.panels.a4Frequency
import xyz.sufystudios.synthguide.ui.panels.centsForA4
import xyz.sufystudios.synthguide.ui.panels.standardPitches

/** Master tuning: cents from A4 = 440 Hz, saved with each preset (as iOS's TuningTests). */
class TuningTest {
    @Test
    fun a4ReadsStandardPitch432AndASemitoneEitherWayAtTheEnds() {
        assertEquals(440.0, a4Frequency(0f), 0.0)
        assertEquals(432.0, a4Frequency((1200 * kotlin.math.log2(432.0 / 440.0)).toFloat()), 0.01)
        assertEquals(415.30, a4Frequency(-100f), 0.01)
        assertEquals(466.16, a4Frequency(100f), 0.01)
    }

    @Test
    fun outOfRangeValuesAreHeldToAHundredCents() {
        assertEquals(100f, SynthParam.MasterTune.clamp(250f))
        assertEquals(-100f, SynthParam.MasterTune.clamp(-999f))
    }

    @Test
    fun eachPresetKeepsItsTuningAndOlderOnesPlayAt440() {
        val values = SynthParam.entries.associateWith { it.default } + (SynthParam.MasterTune to -31.77f)
        val decoded = PresetFile.decode(PresetFile.encode(listOf(Preset("id", "432", values))))!!.single()
        assertEquals(-31.77f, decoded.values[SynthParam.MasterTune]!!, 1e-4f)

        val older = PresetFile.decode("""{"presets":[{"name":"Old","values":{"cutoff":300}}]}""")!!.single()
        assertEquals(0f, older.values[SynthParam.MasterTune])
    }

    @Test
    fun usesTheIosKeyAndAddress() {
        assertEquals("masterTune", SynthParam.MasterTune.key)
        assertEquals(87, SynthParam.MasterTune.id)
    }

    @Test
    fun everyStandardPitchFitsAndReadsAsItsWholeNumber() {
        standardPitches.forEach { pitch ->
            val cents = centsForA4(pitch.hertz.toDouble())
            assertEquals(cents, SynthParam.MasterTune.clamp(cents))
            // The readout shows a tenth of a hertz: it must say exactly "432.0", not 431.9.
            assertEquals("%.1f".format(pitch.hertz.toDouble()), "%.1f".format(a4Frequency(cents)))
        }
    }
}
