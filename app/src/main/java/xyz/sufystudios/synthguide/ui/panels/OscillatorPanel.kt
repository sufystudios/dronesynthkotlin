package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.OscSlotParams
import xyz.sufystudios.synthguide.model.OscWaveform
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.LedToggle
import xyz.sufystudios.synthguide.ui.components.ParamKnob
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroTabBar
import xyz.sufystudios.synthguide.ui.components.WholeStepStepper
import xyz.sufystudios.synthguide.ui.components.oscWavePath
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import kotlin.math.abs
import kotlin.math.roundToInt

/** "OSC" drawer: three independent oscillator slots behind a tab switcher. */
@Composable
fun OscillatorPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    var selected by rememberSaveable { mutableIntStateOf(0) }
    val slot = OscSlotParams.all[selected]
    val isOsc3 = selected == 2
    RetroPanelChrome(modifier) {
        RetroHeader(slot.label, reels = true)
        RetroTabBar(OscSlotParams.all.map { it.label }, selected, { selected = it })

        val waveform = OscWaveform.entries[vm[slot.waveform].roundToInt().coerceIn(0, 4)]
        WaveformScope(waveform)

        Row(horizontalArrangement = Arrangement.spacedBy(4.dp)) {
            OscWaveform.entries.forEach { w ->
                val active = w == waveform
                Column(
                    Modifier
                        .size(36.dp)
                        .clip(RoundedCornerShape(6.dp))
                        .background(Color.Black.copy(alpha = 0.4f))
                        .semantics { contentDescription = "${w.label} waveform" }
                        .clickable(role = Role.RadioButton) { vm.set(slot.waveform, w.ordinal.toFloat()) },
                    horizontalAlignment = Alignment.CenterHorizontally,
                    verticalArrangement = Arrangement.Center,
                ) {
                    val color = if (active) RetroLed else Color.White
                    Canvas(Modifier.size(20.dp, 12.dp)) {
                        drawPath(oscWavePath(w, size, cycles = 1.5), color, style = Stroke(1.5.dp.toPx()))
                    }
                    BasicText(w.label, style = retroLabel(8.sp, color, androidx.compose.ui.text.font.FontWeight.SemiBold))
                }
            }
        }

        Divider()

        ParamKnob(vm, slot.volume, 54.dp, "VOL")
        WholeStepStepper(vm[slot.octave].roundToInt(), -3..3, "Octave") { vm.set(slot.octave, it.toFloat()) }
        WholeStepStepper(vm[slot.semitone].roundToInt(), -12..12, "Semitone") { vm.set(slot.semitone, it.toFloat()) }

        // Continuous detune; OSC 3's is a wide ±700 cent "Pitch" knob for sync sweeps.
        val fine = vm[slot.fine]
        Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(2.dp)) {
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
                BasicText("${slot.fine.min.toInt()}", style = retroLabel(8.sp, Color.White.copy(alpha = 0.5f)))
                ParamKnob(vm, slot.fine, if (isOsc3) 96.dp else 70.dp)
                BasicText("+${slot.fine.max.toInt()}", style = retroLabel(8.sp, Color.White.copy(alpha = 0.5f)))
            }
            val cents = fine.roundToInt()
            BasicText(
                (if (cents > 0) "+" else if (cents < 0) "-" else "") + "${abs(cents)}¢",
                style = retroLabel(11.sp, if (abs(fine) < 0.5f) RetroLed else Color.White),
            )
            BasicText(if (isOsc3) "PITCH" else "FINE", style = retroLabel())
        }

        if (isOsc3) {
            Divider()
            LedToggle("Sync to OSC 1", vm.isOn(SynthParam.Osc3SyncToOsc1)) { vm.toggle(SynthParam.Osc3SyncToOsc1) }
        }
    }
}

/** Live preview of the selected waveform; pink noise shimmers. */
@Composable
private fun WaveformScope(waveform: OscWaveform) {
    val noise by rememberInfiniteTransition(label = "noise").animateFloat(
        0f, 1000f, infiniteRepeatable(tween(33_000, easing = LinearEasing), RepeatMode.Restart), label = "seed",
    )
    Box(
        Modifier
            .fillMaxWidth()
            .height(70.dp)
            .clip(RoundedCornerShape(8.dp))
            .background(Color.Black.copy(alpha = 0.35f))
            .padding(8.dp)
    ) {
        Canvas(Modifier.fillMaxWidth().height(54.dp)) {
            val seed = if (waveform == OscWaveform.Pink) noise.toDouble() else 0.0
            drawPath(oscWavePath(waveform, size, noiseSeed = seed), RetroLed, style = Stroke(2.dp.toPx()))
        }
    }
}

@Composable
internal fun Divider() {
    Box(Modifier.fillMaxWidth().height(1.dp).background(RetroAmber.copy(alpha = 0.3f)))
}
