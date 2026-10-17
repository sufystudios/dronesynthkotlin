package xyz.sufystudios.synthguide.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.size
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import xyz.sufystudios.synthguide.model.LfoShape
import xyz.sufystudios.synthguide.model.OscWaveform

/** The drawer tabs, and which edge's rail each sits on (as on iOS). */
enum class DrawerTab(val label: String, val leftSide: Boolean) {
    Osc("OSC", false),
    Lfo("LFO", false),
    Adsr("ADSR", false),
    Delay("Delay", true),
    Presets("Presets", true),
    Reverb("Reverb", true),
}

/** Small line-art icon for a drawer tab, drawn rather than loaded so it tints freely. */
@Composable
fun RailIcon(tab: DrawerTab, color: Color, modifier: Modifier = Modifier) {
    Canvas(modifier.size(20.dp)) {
        val s = size.minDimension
        val stroke = Stroke(1.6.dp.toPx(), cap = StrokeCap.Round)
        when (tab) {
            DrawerTab.Osc -> tuningFork(color, s, stroke)
            DrawerTab.Lfo -> drawPath(lfoWavePath(LfoShape.Sine, Size(s, s * 0.7f), cycles = 1.5).shifted(0f, s * 0.15f), color, style = stroke)
            DrawerTab.Adsr -> drawPath(adsrPath(0.3f, 0.3f, 0.5f, 0.5f, Size(s, s * 0.7f)).shifted(0f, s * 0.15f), color, style = stroke)
            DrawerTab.Delay -> {
                drawCircle(color, s * 0.4f, Offset(s / 2, s * 0.55f), style = stroke)
                drawLine(color, Offset(s / 2, s * 0.55f), Offset(s / 2, s * 0.3f), stroke.width, StrokeCap.Round)
                drawLine(color, Offset(s * 0.4f, s * 0.05f), Offset(s * 0.6f, s * 0.05f), stroke.width, StrokeCap.Round)
            }
            DrawerTab.Presets -> {
                drawRoundRectOutline(color, s, stroke)
                for (i in 0 until 3) {
                    val y = s * (0.35f + i * 0.17f)
                    drawCircle(color, s * 0.04f, Offset(s * 0.28f, y))
                    drawLine(color, Offset(s * 0.4f, y), Offset(s * 0.75f, y), stroke.width * 0.8f, StrokeCap.Round)
                }
            }
            DrawerTab.Reverb -> for (i in 0 until 3) {
                drawPath(oscWavePath(OscWaveform.Sine, Size(s, s * 0.2f), cycles = 2.0).shifted(0f, s * (0.15f + i * 0.25f)), color, style = stroke)
            }
        }
    }
}

private fun DrawScope.tuningFork(color: Color, s: Float, stroke: Stroke) {
    val path = Path().apply {
        moveTo(s * 0.3f, s * 0.05f)
        lineTo(s * 0.3f, s * 0.45f)
        quadraticTo(s * 0.3f, s * 0.62f, s * 0.5f, s * 0.62f)
        quadraticTo(s * 0.7f, s * 0.62f, s * 0.7f, s * 0.45f)
        lineTo(s * 0.7f, s * 0.05f)
        moveTo(s * 0.5f, s * 0.62f)
        lineTo(s * 0.5f, s * 0.95f)
    }
    drawPath(path, color, style = stroke)
}

private fun DrawScope.drawRoundRectOutline(color: Color, s: Float, stroke: Stroke) {
    drawRoundRect(
        color,
        topLeft = Offset(s * 0.1f, s * 0.15f),
        size = Size(s * 0.8f, s * 0.7f),
        cornerRadius = androidx.compose.ui.geometry.CornerRadius(s * 0.1f),
        style = stroke,
    )
}

private fun Path.shifted(dx: Float, dy: Float): Path = apply { translate(Offset(dx, dy)) }
