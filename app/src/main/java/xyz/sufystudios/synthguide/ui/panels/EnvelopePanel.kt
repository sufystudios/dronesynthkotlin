package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.ParamKnob
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroPowerSwitch
import xyz.sufystudios.synthguide.ui.components.adsrPath
import xyz.sufystudios.synthguide.ui.theme.Neon
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.retroLabel

/** "ADSR" drawer: Moog 911-style T1/T2/Esus/T3 envelope, glide, and the filter EG patch. */
@Composable
fun EnvelopePanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    RetroPanelChrome(modifier) {
        RetroHeader("ENVELOPE", ledLit = true)
        Box(
            Modifier
                .fillMaxWidth()
                .height(90.dp)
                .clip(RoundedCornerShape(8.dp))
                .background(Color(0xFF051408))
                .padding(8.dp)
        ) {
            Canvas(Modifier.fillMaxSize()) {
                val grid = Neon.copy(alpha = 0.25f)
                for (i in 0..8) {
                    val x = size.width * i / 8f
                    drawLine(grid, Offset(x, 0f), Offset(x, size.height), 0.5.dp.toPx())
                }
                for (i in 0..3) {
                    val y = size.height * i / 3f
                    drawLine(grid, Offset(0f, y), Offset(size.width, y), 0.5.dp.toPx())
                }
                val path = adsrPath(vm[SynthParam.Attack], vm[SynthParam.Decay], vm[SynthParam.Sustain], vm[SynthParam.Release], size)
                drawPath(path, Neon.copy(alpha = 0.3f), style = Stroke(6.dp.toPx()))
                drawPath(path, Neon, style = Stroke(2.dp.toPx()))
            }
        }

        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly, verticalAlignment = Alignment.Top) {
            ParamKnob(vm, SynthParam.Glide, 52.dp, "GLIDE")
            StageKnob(vm, SynthParam.Attack, "T1", "ATTACK")
            StageKnob(vm, SynthParam.Decay, "T2", "DECAY")
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly, verticalAlignment = Alignment.Top) {
            StageKnob(vm, SynthParam.Sustain, "ESUS", "SUSTAIN")
            StageKnob(vm, SynthParam.Release, "T3", "RELEASE")
        }

        Divider()

        // EG -> VCF: the same envelope patched to the filter cutoff, like patching a
        // 911 to a filter on a modular rig. Short times make plucky cutoff "thwacks".
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(12.dp)) {
            Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(4.dp)) {
                BasicText("FILTER EG", style = retroLabel())
                RetroPowerSwitch(vm.isOn(SynthParam.FilterEnvEnabled), { vm.toggle(SynthParam.FilterEnvEnabled) }, "Filter envelope")
            }
            ParamKnob(vm, SynthParam.FilterEnvAmount, 52.dp, "EG AMOUNT")
            Spacer(Modifier.weight(1f))
        }
    }
}

@Composable
private fun StageKnob(vm: SynthViewModel, param: SynthParam, code: String, name: String) {
    Column(horizontalAlignment = Alignment.CenterHorizontally) {
        ParamKnob(vm, param, 52.dp, code, labelStyle = retroLabel(9.sp))
        BasicText(name, style = retroLabel(7.sp, RetroAmber.copy(alpha = 0.7f), androidx.compose.ui.text.font.FontWeight.SemiBold))
    }
}
