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
import xyz.sufystudios.synthguide.ui.theme.RetroLed

/** The drawer tabs, and which edge's rail each sits on (as on iOS). */
enum class DrawerTab(val label: String, val leftSide: Boolean) {
    Osc("OSC", false),
    Lfo("LFO", false),
    Adsr("ADSR", false),
    Rec("REC", false),
    Tune("TUNE", false),
    Delay("Delay", true),
    Presets("Presets", true),
    Reverb("Reverb", true),
    Looper("Looper", true),
    Arp("ARP", true),
}

/**
 * Small line-art icon for a drawer tab, drawn rather than loaded so it tints freely.
 * [loopProgress] (0..1) draws the Looper's position ring around its icon.
 */
@Composable
fun RailIcon(tab: DrawerTab, color: Color, modifier: Modifier = Modifier, loopProgress: Float? = null) {
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
            DrawerTab.Rec -> {
                drawRoundRect(color, Offset(s * 0.35f, s * 0.05f), Size(s * 0.3f, s * 0.55f), androidx.compose.ui.geometry.CornerRadius(s * 0.15f))
                drawArc(color, 0f, 180f, false, Offset(s * 0.22f, s * 0.3f), Size(s * 0.56f, s * 0.45f), style = stroke)
                drawLine(color, Offset(s / 2, s * 0.75f), Offset(s / 2, s * 0.95f), stroke.width, StrokeCap.Round)
            }
            DrawerTab.Tune -> {
                // A dial: its pointer straight up, with ticks either side.
                drawCircle(color, s * 0.32f, Offset(s / 2, s * 0.58f), style = stroke)
                drawLine(color, Offset(s / 2, s * 0.58f), Offset(s / 2, s * 0.36f), stroke.width, StrokeCap.Round)
                for (i in -1..1 step 2) {
                    drawLine(color, Offset(s * (0.5f + 0.32f * i), s * 0.12f), Offset(s * (0.5f + 0.24f * i), s * 0.2f), stroke.width * 0.8f, StrokeCap.Round)
                }
                drawLine(color, Offset(s / 2, s * 0.02f), Offset(s / 2, s * 0.12f), stroke.width * 0.8f, StrokeCap.Round)
            }
            DrawerTab.Looper -> {
                drawCircle(Color.White.copy(alpha = 0.25f), s * 0.45f, style = Stroke(1.5.dp.toPx()))
                if (loopProgress != null) {
                    drawArc(RetroLed, -90f, 360f * loopProgress, false, Offset(s * 0.05f, s * 0.05f), Size(s * 0.9f, s * 0.9f),
                        style = Stroke(1.5.dp.toPx(), cap = StrokeCap.Round))
                }
                // Two chasing arrows.
                drawArc(color, 200f, 120f, false, Offset(s * 0.25f, s * 0.25f), Size(s * 0.5f, s * 0.5f), style = stroke)
                drawArc(color, 20f, 120f, false, Offset(s * 0.25f, s * 0.25f), Size(s * 0.5f, s * 0.5f), style = stroke)
            }
            DrawerTab.Arp -> {
                // Piano keys: four white keys with three black ones between them.
                drawRoundRectOutline(color, s, stroke)
                for (i in 1..3) {
                    val x = s * (0.1f + 0.2f * i)
                    drawLine(color, Offset(x, s * 0.55f), Offset(x, s * 0.85f), stroke.width * 0.8f)
                    drawRect(color, Offset(x - s * 0.05f, s * 0.15f), Size(s * 0.1f, s * 0.4f))
                }
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
