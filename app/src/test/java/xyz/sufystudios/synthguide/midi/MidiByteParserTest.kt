package xyz.sufystudios.synthguide.midi

import org.junit.Assert.assertEquals
import org.junit.Test

class MidiByteParserTest {
    private val events = mutableListOf<String>()
    private val parser = MidiByteParser(object : MidiSink {
        override fun noteOn(note: Int, velocity: Int) { events += "on $note $velocity" }
        override fun noteOff(note: Int) { events += "off $note" }
        override fun pitchBend(value: Float) { events += "bend $value" }
        override fun modWheel(value: Float) { events += "mod $value" }
        override fun allNotesOff() { events += "all off" }
    })

    private fun send(vararg bytes: Int) = bytes.forEach(parser::handle)

    @Test fun noteOnAndOff() {
        send(0x90, 60, 100, 0x80, 60, 0)
        assertEquals(listOf("on 60 100", "off 60"), events)
    }

    @Test fun noteOnWithZeroVelocityIsNoteOff() {
        send(0x92, 64, 0)
        assertEquals(listOf("off 64"), events)
    }

    @Test fun runningStatusRepeatsTheLastStatus() {
        send(0x90, 60, 90, 62, 91, 64, 0)
        assertEquals(listOf("on 60 90", "on 62 91", "off 64"), events)
    }

    @Test fun realtimeBytesInsideAMessageAreIgnored() {
        send(0x90, 60, 0xF8, 100)
        assertEquals(listOf("on 60 100"), events)
    }

    @Test fun sysExIsSkipped() {
        send(0xF0, 0x7E, 0x01, 0x02, 0xF7, 0x90, 48, 70)
        assertEquals(listOf("on 48 70"), events)
    }

    @Test fun pitchBendCentreAndExtremes() {
        send(0xE0, 0x00, 0x40, 0xE0, 0x00, 0x00, 0xE0, 0x7F, 0x7F)
        // 14-bit range 0..16383 around a centre of 8192, so the top is just under +1.
        assertEquals(listOf("bend 0.0", "bend -1.0", "bend ${8191 / 8192f}"), events)
    }

    @Test fun modWheelAndAllNotesOff() {
        send(0xB0, 1, 127, 0xB0, 123, 0)
        assertEquals(listOf("mod 1.0", "all off"), events)
    }

    @Test fun programChangeTakesOneDataByte() {
        send(0xC0, 5, 0x90, 60, 80)
        assertEquals(listOf("on 60 80"), events)
    }
}
