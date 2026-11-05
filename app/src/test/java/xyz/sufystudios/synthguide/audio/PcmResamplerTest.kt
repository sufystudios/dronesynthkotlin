package xyz.sufystudios.synthguide.audio

import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import java.io.ByteArrayInputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.log10
import kotlin.math.sin
import kotlin.math.sqrt

/** A take above 48 kHz is lowered to 48 kHz for the AAC encoder, keeping its pitch and length. */
class PcmResamplerTest {
    private fun stereoTone(rate: Int, hz: Double, seconds: Double, amplitude: Double = 0.5): ByteArray {
        val frames = (rate * seconds).toInt()
        val bytes = ByteBuffer.allocate(frames * 4).order(ByteOrder.LITTLE_ENDIAN)
        for (i in 0 until frames) {
            val s = (amplitude * 32767 * sin(2 * PI * hz * i / rate)).toInt().toShort()
            bytes.putShort(s)
            bytes.putShort((-s).toShort())
        }
        return bytes.array()
    }

    private fun resample(input: ByteArray, from: Int, to: Int, chunk: Int = 16_384): Pair<ShortArray, ShortArray> {
        val stream = ByteArrayInputStream(input)
        val resampler = PcmResampler(from, to, 2) { b, o, n -> stream.read(b, o, n) }
        val out = java.io.ByteArrayOutputStream()
        val buffer = ByteArray(chunk)
        while (true) {
            val read = resampler.read(buffer, 0, buffer.size)
            if (read < 0) break
            out.write(buffer, 0, read)
        }
        val shorts = ByteBuffer.wrap(out.toByteArray()).order(ByteOrder.LITTLE_ENDIAN).asShortBuffer()
        val left = ShortArray(shorts.remaining() / 2)
        val right = ShortArray(left.size)
        for (i in left.indices) {
            left[i] = shorts.get()
            right[i] = shorts.get()
        }
        return left to right
    }

    /** Level of [hz] in the middle of [x], as a fraction of full scale. */
    private fun levelAt(x: ShortArray, rate: Int, hz: Double): Double {
        val from = x.size / 4
        val to = x.size * 3 / 4
        var re = 0.0
        var im = 0.0
        for (i in from until to) {
            re += x[i] * cos(2 * PI * hz * i / rate)
            im += x[i] * sin(2 * PI * hz * i / rate)
        }
        return 2 * sqrt(re * re + im * im) / (to - from) / 32767
    }

    private fun rmsOf(x: ShortArray): Double {
        val from = x.size / 4
        val to = x.size * 3 / 4
        var s = 0.0
        for (i in from until to) s += (x[i] / 32767.0) * (x[i] / 32767.0)
        return sqrt(s / (to - from))
    }

    @Test
    fun keepsTheLengthAndTheToneAtEveryCommonRate() {
        for (rate in intArrayOf(88_200, 96_000, 192_000)) {
            val (left, right) = resample(stereoTone(rate, 1000.0, 1.0), rate, 48_000)
            assertEquals("length at $rate", 48_000.0, left.size.toDouble(), 2.0)
            assertEquals("level at $rate", 0.5, levelAt(left, 48_000, 1000.0), 0.005)
            assertEquals("the right channel stays itself at $rate", 0.5, levelAt(right, 48_000, 1000.0), 0.005)
            // All of the signal is at 1 kHz: nothing else crept in.
            val residual = rmsOf(left) * rmsOf(left) - 0.125
            assertEquals("no other energy at $rate", 0.0, residual, 0.001)
        }
    }

    @Test
    fun whatTheNewRateCantHoldIsRemovedRatherThanFoldedBack() {
        val (left, _) = resample(stereoTone(96_000, 30_000.0, 1.0), 96_000, 48_000)
        // A 30 kHz tone would alias to 18 kHz if it got through.
        assertTrue(20 * log10(rmsOf(left).coerceAtLeast(1e-9)) < -55)
    }

    @Test
    fun readSizeDoesNotChangeTheResult() {
        val input = stereoTone(96_000, 440.0, 0.5)
        val (a, _) = resample(input, 96_000, 48_000, chunk = 16_384)
        val (b, _) = resample(input, 96_000, 48_000, chunk = 100)
        assertTrue(a.contentEquals(b))
    }

    @Test
    fun anEmptyTakeEndsAtOnce() {
        val (left, _) = resample(ByteArray(0), 96_000, 48_000)
        assertEquals(0, left.size)
    }
}
