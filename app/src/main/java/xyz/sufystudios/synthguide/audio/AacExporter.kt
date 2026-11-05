package xyz.sufystudios.synthguide.audio

import android.media.MediaCodec
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.media.MediaMuxer
import java.io.File
import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Converts the recorder's 16-bit WAV take into a compact AAC .m4a for sharing
 * (about a tenth of the size), using the platform encoder. Call off the main thread.
 */
object AacExporter {
    private const val TIMEOUT_US = 10_000L
    private const val MAX_ENCODER_RATE = 48_000

    fun export(wav: File, out: File, bitRate: Int = 192_000) {
        RandomAccessFile(wav, "r").use { input ->
            val header = ByteArray(44).also { input.readFully(it) }
            val h = ByteBuffer.wrap(header).order(ByteOrder.LITTLE_ENDIAN)
            val channels = h.getShort(22).toInt()
            val sampleRate = h.getInt(24)
            val bytesPerFrame = channels * 2
            // Android's own AAC encoder only promises rates up to 48 kHz.
            val encoderRate = minOf(sampleRate, MAX_ENCODER_RATE)
            val readPcm: (ByteArray, Int, Int) -> Int = if (sampleRate > encoderRate) {
                PcmResampler(sampleRate, encoderRate, channels) { b, o, n -> input.read(b, o, n) }::read
            } else {
                { b, o, n -> input.read(b, o, n) }
            }

            val format = MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, encoderRate, channels).apply {
                setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC)
                setInteger(MediaFormat.KEY_BIT_RATE, bitRate)
                setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 16_384)
            }
            val codec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC)
            codec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
            codec.start()
            out.delete()
            val muxer = MediaMuxer(out.path, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
            try {
                val info = MediaCodec.BufferInfo()
                val chunk = ByteArray(16_384 - 16_384 % bytesPerFrame)
                var framesQueued = 0L
                var inputDone = false
                var track = -1
                while (true) {
                    if (!inputDone) {
                        val index = codec.dequeueInputBuffer(TIMEOUT_US)
                        if (index >= 0) {
                            val buffer = codec.getInputBuffer(index)!!
                            val read = readPcm(chunk, 0, minOf(chunk.size, buffer.remaining()))
                            val timeUs = framesQueued * 1_000_000L / encoderRate
                            if (read <= 0) {
                                codec.queueInputBuffer(index, 0, 0, timeUs, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                                inputDone = true
                            } else {
                                buffer.clear()
                                buffer.put(chunk, 0, read)
                                codec.queueInputBuffer(index, 0, read, timeUs, 0)
                                framesQueued += read / bytesPerFrame
                            }
                        }
                    }
                    val outIndex = codec.dequeueOutputBuffer(info, TIMEOUT_US)
                    when {
                        outIndex == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                            track = muxer.addTrack(codec.outputFormat)
                            muxer.start()
                        }
                        outIndex >= 0 -> {
                            val isConfig = info.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG != 0
                            if (!isConfig && info.size > 0 && track >= 0) {
                                muxer.writeSampleData(track, codec.getOutputBuffer(outIndex)!!, info)
                            }
                            codec.releaseOutputBuffer(outIndex, false)
                            if (info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM != 0) break
                        }
                    }
                }
            } finally {
                codec.stop()
                codec.release()
                runCatching { muxer.stop() }
                muxer.release()
            }
        }
    }
}
