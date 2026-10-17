package xyz.sufystudios.synthguide.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test

class PresetFileTest {

    @Test
    fun roundTripsNamesAndValues() {
        val values = SynthParam.entries.associateWith { it.default }.toMutableMap()
        values[SynthParam.Cutoff] = 1234f
        values[SynthParam.Osc2Fine] = -12f
        val text = PresetFile.encode(listOf(Preset("id", "Deep Hum", values), Preset("id2", "Other", values)))

        val decoded = PresetFile.decode(text)!!
        assertEquals(listOf("Deep Hum", "Other"), decoded.map { it.name })
        assertEquals(1234f, decoded[0].values[SynthParam.Cutoff])
        assertEquals(-12f, decoded[0].values[SynthParam.Osc2Fine])
    }

    @Test
    fun missingKeysTakeDefaultsAndUnknownKeysAreIgnored() {
        val text = """{"app":"Drone Synth","format":9,"presets":[{"name":"Future","values":{"cutoff":500,"someNewKnob":3}}]}"""
        val p = PresetFile.decode(text)!!.single()
        assertEquals(500f, p.values[SynthParam.Cutoff])
        assertEquals(SynthParam.Attack.default, p.values[SynthParam.Attack])
    }

    @Test
    fun valuesAreClampedToTheirRange() {
        val text = """{"presets":[{"name":"Loud","values":{"cutoff":99999,"sustain":-4}}]}"""
        val p = PresetFile.decode(text)!!.single()
        assertEquals(SynthParam.Cutoff.max, p.values[SynthParam.Cutoff])
        assertEquals(0f, p.values[SynthParam.Sustain])
    }

    @Test
    fun readsTheLibrarysOwnBareList() {
        val text = """[{"id":"x","name":"Saved","values":{"cutoff":300}}]"""
        assertEquals("Saved", PresetFile.decode(text)!!.single().name)
    }

    @Test
    fun badEntriesAreSkippedAndNonPresetFilesRejected() {
        assertEquals(1, PresetFile.decode("""{"presets":[{"name":"ok","values":{}},{"oops":1}]}""")!!.size)
        assertNull(PresetFile.decode("""{"presets":[]}"""))
        assertNull(PresetFile.decode("""{"hello":"world"}"""))
        assertNull(PresetFile.decode("not json at all"))
    }

    @Test
    fun fileNamesAreSafe() {
        assertEquals("Drone Synth - Dark_Light 2.json", PresetFile.fileName("Dark/Light 2"))
        assertEquals("Drone Synth - Preset.json", PresetFile.fileName("   "))
    }
}
