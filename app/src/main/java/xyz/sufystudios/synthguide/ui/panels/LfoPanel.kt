package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.animation.core.withInfiniteAnimationFrameMillis
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import xyz.sufystudios.synthguide.model.LfoParams
import xyz.sufystudios.synthguide.model.LfoShape
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.LedToggle
import xyz.sufystudios.synthguide.ui.components.ParamKnob
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroTabBar
import xyz.sufystudios.synthguide.ui.components.lfoWavePath
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import kotlin.math.roundToInt

/** "LFO" drawer: three independent LFOs (filter, pitch OSC 1 & 2, pitch OSC 3). */
@Composable
fun LfoPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    var selected by rememberSaveable { mutableIntStateOf(0) }
    val lfo = LfoParams.all[selected]
    val enabled = vm.isOn(lfo.enabled)
    val shape = LfoShape.entries[vm[lfo.shape].roundToInt().coerceIn(0, 3)]
    RetroPanelChrome(modifier) {
        RetroHeader("MODULATION", ledLit = enabled)
        RetroTabBar(LfoParams.all.map { it.label }, selected, { selected = it })

        Box(
            Modifier
                .fillMaxWidth()
                .height(80.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(Color.Black.copy(alpha = 0.35f))
        ) {
            Canvas(Modifier.fillMaxSize().padding(8.dp)) {
                drawPath(lfoWavePath(shape, size), RetroLed, style = Stroke(2.dp.toPx()))
            }
            if (enabled) ScanLine(rateHz = vm[lfo.rate])
        }

        Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            LfoShape.entries.forEach { s ->
                val color = if (s == shape) RetroLed else Color.White
                Box(
                    Modifier
                        .size(32.dp, 26.dp)
                        .clip(RoundedCornerShape(6.dp))
                        .background(Color.Black.copy(alpha = 0.4f))
                        .semantics { contentDescription = "${s.label} shape" }
                        .clickable(role = Role.RadioButton) { vm.set(lfo.shape, s.ordinal.toFloat()) }
                        .padding(5.dp)
                ) {
                    Canvas(Modifier.fillMaxSize()) { drawPath(lfoWavePath(s, size, cycles = 1.5), color, style = Stroke(1.5.dp.toPx())) }
                }
            }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(14.dp)) {
            ParamKnob(vm, lfo.rate, 54.dp, "RATE")
            ParamKnob(vm, lfo.randomize, 54.dp, "RANDOM")
        }

        Divider()

        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            LedToggle(lfo.label, enabled) { vm.toggle(lfo.enabled) }
            Spacer(Modifier.weight(1f))
            ParamKnob(vm, lfo.depth, 50.dp, "DEPTH", enabled = enabled)
        }
    }
}

/** A "tape head" line sweeping across the preview at a fraction of the LFO's rate. */
@Composable
private fun ScanLine(rateHz: Float) {
    var phase by remember { mutableFloatStateOf(0f) }
    LaunchedEffect(rateHz) {
        var last = -1L
        while (true) {
            withInfiniteAnimationFrameMillis { now ->
                if (last >= 0) phase = (phase + (now - last) / 1000f * rateHz / 3f) % 1f
                last = now
            }
        }
    }
    Canvas(Modifier.fillMaxSize()) {
        val x = phase * size.width
        drawLine(Color.White.copy(alpha = 0.5f), Offset(x, 0f), Offset(x, size.height), 1.5.dp.toPx())
    }
}
