package xyz.sufystudios.synthguide.model

import org.junit.Assert.assertEquals
import org.junit.Test

/** The filter's starting point (as iOS's cutoffStartsHalfwayAlongTheKnob). */
class CutoffTest {
    @Test
    fun cutoffStartsWithItsKnobAt12OClockAbout894Hz() {
        assertEquals(894.4f, SynthParam.Cutoff.default, 0.1f)
        assertEquals(0.5f, SynthParam.Cutoff.toPosition(SynthParam.Cutoff.default), 1e-5f)
    }
}
