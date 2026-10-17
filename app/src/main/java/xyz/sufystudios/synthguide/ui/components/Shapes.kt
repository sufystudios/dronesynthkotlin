package xyz.sufystudios.synthguide.ui.components

import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Path
import xyz.sufystudios.synthguide.model.LfoShape
import xyz.sufystudios.synthguide.model.OscWaveform
import kotlin.math.PI
import kotlin.math.floor
import kotlin.math.max
import kotlin.math.sin

/** Value of a waveform at phase [p] (cycles), roughly -1..1. [noiseSeed] animates pink noise. */
private fun oscValue(waveform: OscWaveform, phase: Double, index: Int, noiseSeed: Double, pinkState: DoubleArray): Double {
    val p = phase - floor(phase)
    return when (waveform) {
        OscWaveform.Sine -> sin(p * 2 * PI)
        OscWaveform.Triangle -> if (p < 0.5) 4 * p - 1 else 3 - 4 * p
        OscWaveform.Square -> if (p < 0.5) 1.0 else -1.0
        OscWaveform.Saw -> 2 * p - 1
        OscWaveform.Pink -> {
            val seed = sin(index * 12.9898 + noiseSeed * 78.233) * 43758.5453
            val raw = (seed - floor(seed)) * 2 - 1
            // A one-pole integrator rolls the highs off, like pink noise's -3 dB/octave.
            pinkState[0] += (raw - pinkState[0]) * 0.15
            pinkState[0] * 2.2
        }
    }
}

/** Path tracing [cycles] cycles of an oscillator waveform across [size]. */
fun oscWavePath(waveform: OscWaveform, size: Size, cycles: Double = 3.0, noiseSeed: Double = 0.0): Path {
    val path = Path()
    val points = 300
    val amp = size.height / 2f * 0.8f
    val pink = DoubleArray(1)
    for (i in 0..points) {
        val t = i.toDouble() / points
        val y = size.height / 2f - oscValue(waveform, t * cycles, i, noiseSeed, pink).toFloat() * amp
        val x = t.toFloat() * size.width
        if (i == 0) path.moveTo(x, y) else path.lineTo(x, y)
    }
    return path
}

/** Path tracing [cycles] cycles of an LFO shape; Random is a stable stepped preview. */
fun lfoWavePath(shape: LfoShape, size: Size, cycles: Double = 3.0): Path {
    val path = Path()
    val points = 200
    val amp = size.height / 2f * 0.8f
    for (i in 0..points) {
        val t = i.toDouble() / points
        val phase = t * cycles
        val p = phase - floor(phase)
        val value = when (shape) {
            LfoShape.Sine -> sin(phase * 2 * PI)
            LfoShape.Triangle -> if (p < 0.5) 4 * p - 1 else 3 - 4 * p
            LfoShape.Square -> if (p < 0.5) 1.0 else -1.0
            LfoShape.Random -> {
                val seed = sin(floor(phase) * 12.9898) * 43758.5453
                (seed - floor(seed)) * 2 - 1
            }
        }
        val x = t.toFloat() * size.width
        val y = size.height / 2f - value.toFloat() * amp
        if (i == 0) path.moveTo(x, y) else path.lineTo(x, y)
    }
    return path
}

/** ADSR outline: attack, decay, a fixed sustain hold, then release, scaled to fit. */
fun adsrPath(attack: Float, decay: Float, sustain: Float, release: Float, size: Size): Path {
    val hold = 0.6f
    val total = max(attack + decay + release, 0.001f) + hold
    val w = size.width
    val bottom = size.height
    val sustainY = bottom - sustain * bottom
    var x = 0f
    return Path().apply {
        moveTo(x, bottom)
        x += w * attack / total; lineTo(x, 0f)
        x += w * decay / total; lineTo(x, sustainY)
        x += w * hold / total; lineTo(x, sustainY)
        x += w * release / total; lineTo(x, bottom)
    }
}
