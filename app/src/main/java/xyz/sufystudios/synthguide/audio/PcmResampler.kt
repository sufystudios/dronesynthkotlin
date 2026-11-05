package xyz.sufystudios.synthguide.audio

import kotlin.math.PI
import kotlin.math.abs
import kotlin.math.ceil
import kotlin.math.cos
import kotlin.math.roundToInt
import kotlin.math.sin

/**
 * Lowers the sample rate of 16-bit little-endian PCM as it's read, with a windowed-sinc
 * filter. [readSource] reads like InputStream.read(bytes, offset, length).
 */
class PcmResampler(
    inRate: Int,
    outRate: Int,
    private val channels: Int,
    private val readSource: (ByteArray, Int, Int) -> Int,
) {
    private val step = inRate.toDouble() / outRate
    private val cutoff = outRate.toDouble() / inRate * 0.95
    private val halfWidth = ceil(ZERO_CROSSINGS / cutoff).toInt()
    private val bytesPerFrame = channels * 2

    private val windowFrames = 4096 + 2 * halfWidth
    private val window = ShortArray(windowFrames * channels)
    private val sourceBytes = ByteArray(windowFrames * bytesPerFrame)
    private var windowStart = 0L
    private var windowEnd = 0L
    private var sourceDone = false
    private var outputFrame = 0L

    init {
        require(outRate < inRate) { "PcmResampler only lowers the rate" }
    }

    /** Reads up to [length] bytes of whole frames into [out]. -1 once the source has run out. */
    fun read(out: ByteArray, offset: Int, length: Int): Int {
        val frames = length / bytesPerFrame
        var written = 0
        val sums = DoubleArray(channels)
        while (written < frames) {
            val position = outputFrame * step
            val centre = position.toLong()
            fillTo(centre + halfWidth + 1, centre - halfWidth + 1)
            if (sourceDone && centre >= windowEnd) break
            val fraction = position - centre
            sums.fill(0.0)
            for (k in 1 - halfWidth..halfWidth) {
                val h = kernel(abs((k - fraction) * cutoff))
                val frame = centre + k
                if (h == 0.0 || frame < windowStart || frame >= windowEnd) continue
                val at = (frame - windowStart).toInt() * channels
                for (c in 0 until channels) sums[c] += h * window[at + c]
            }
            for (c in 0 until channels) {
                val sample = (sums[c] * cutoff).roundToInt().coerceIn(-32768, 32767)
                val at = offset + written * bytesPerFrame + c * 2
                out[at] = sample.toByte()
                out[at + 1] = (sample shr 8).toByte()
            }
            written++
            outputFrame++
        }
        return if (written == 0 && frames > 0) -1 else written * bytesPerFrame
    }

    /** Makes the window reach [end] if the source can, dropping frames before [keepFrom]. */
    private fun fillTo(end: Long, keepFrom: Long) {
        if (end <= windowEnd || sourceDone) return
        val drop = (keepFrom - windowStart).coerceIn(0, windowEnd - windowStart).toInt()
        val kept = (windowEnd - windowStart).toInt() - drop
        window.copyInto(window, 0, drop * channels, (drop + kept) * channels)
        windowStart += drop
        windowEnd = windowStart + kept

        val wanted = (windowFrames - kept) * bytesPerFrame
        var filled = 0
        while (filled < wanted) {
            val read = readSource(sourceBytes, filled, wanted - filled)
            if (read <= 0) {
                sourceDone = true
                break
            }
            filled += read
        }
        val frames = filled / bytesPerFrame
        for (i in 0 until frames * channels) {
            val low = sourceBytes[2 * i].toInt() and 0xFF
            val high = sourceBytes[2 * i + 1].toInt()
            window[kept * channels + i] = ((high shl 8) or low).toShort()
        }
        windowEnd += frames
    }

    private companion object {
        const val ZERO_CROSSINGS = 16
        const val RESOLUTION = 256

        /** Half of a Blackman-windowed sinc. */
        val table = DoubleArray(ZERO_CROSSINGS * RESOLUTION + 2).also { table ->
            for (i in 0..ZERO_CROSSINGS * RESOLUTION) {
                val x = i.toDouble() / RESOLUTION
                val t = x / ZERO_CROSSINGS
                val sinc = if (i == 0) 1.0 else sin(PI * x) / (PI * x)
                table[i] = sinc * (0.42 + 0.5 * cos(PI * t) + 0.08 * cos(2 * PI * t))
            }
        }

        fun kernel(x: Double): Double {
            val scaled = x * RESOLUTION
            val index = scaled.toInt()
            if (index >= ZERO_CROSSINGS * RESOLUTION) return 0.0
            return table[index] + (scaled - index) * (table[index + 1] - table[index])
        }
    }
}
