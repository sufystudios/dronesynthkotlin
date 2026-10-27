package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.DelayMediumKind
import xyz.sufystudios.synthguide.model.DelayMix
import xyz.sufystudios.synthguide.model.ReverbKind
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.CompactResetButton
import xyz.sufystudios.synthguide.ui.components.LedBarSlider
import xyz.sufystudios.synthguide.ui.components.ParamKnob
import xyz.sufystudios.synthguide.ui.components.ResetButton
import xyz.sufystudios.synthguide.ui.components.RetroFader
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroSegmentRow
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import java.util.Locale
import kotlin.math.roundToInt

/**
 * The Vintage Delay drawer. Timing follows Calf's ms mode: Time (or TAP) sets one
 * beat, Subdivide splits it into units, and Time L / Time R count units per channel.
 */
@Composable
fun DelayPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    // Names sit beside their controls rather than under them, and Reset in the title
    // row, to keep the drawer short enough for a phone in landscape.
    RetroPanelChrome(modifier, spacing = 6.dp) {
        RetroHeader("VINTAGE DELAY", ledLit = vm[SynthParam.DelayWet] > 0f) {
            Spacer(Modifier.width(6.dp))
            CompactResetButton(vm::resetDelay)
        }
        DelayReadout(vm)

        Column(
            Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(4.dp))
                .background(Color.Black.copy(alpha = 0.3f))
                .padding(horizontal = 8.dp, vertical = 4.dp),
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(2.dp),
        ) {
            Row(Modifier.fillMaxWidth().padding(horizontal = 4.dp, vertical = 2.dp), verticalAlignment = Alignment.CenterVertically) {
                ParamKnob(vm, SynthParam.DelayTime, 42.dp, resettable = false)
                BasicText("TIME", style = retroLabel(), modifier = Modifier.padding(start = 6.dp))
                Spacer(Modifier.weight(1f).widthIn(min = 8.dp))
                Box(
                    Modifier
                        .size(40.dp)
                        .clip(CircleShape)
                        .background(Brush.radialGradient(listOf(RetroLed, Color(0xFF801A0D))))
                        .semantics { contentDescription = "Tap tempo" }
                        .clickable(role = Role.Button) { vm.tapTempo() },
                    contentAlignment = Alignment.Center,
                ) {
                    BasicText("TAP", style = retroLabel(9.sp, Color.White))
                }
                BasicText("SETS\nTIME", style = retroLabel(8.sp, RetroAmber.copy(alpha = 0.7f)), modifier = Modifier.padding(start = 6.dp))
            }
            for ((label, param) in listOf(
                "SUBDIVIDE" to SynthParam.DelaySubdivide,
                "TIME L" to SynthParam.DelayTimeLeft,
                "TIME R" to SynthParam.DelayTimeRight,
            )) {
                RetroFader(label, vm[param], param.min..param.max) { vm.set(param, it) }
            }
        }

        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            SmallKnob(vm, SynthParam.DelayFeedback, "FEEDBACK", 40.dp)
            SmallKnob(vm, SynthParam.DelayWidth, "WIDTH", 40.dp)
            SmallKnob(vm, SynthParam.DelayWet, "AMOUNT", 40.dp)
            SmallKnob(vm, SynthParam.DelayDry, "DRY", 40.dp)
        }

        Column(verticalArrangement = Arrangement.spacedBy(3.dp)) {
            for ((label, row) in listOf(
                "MIX" to Pair(SynthParam.DelayMixMode, DelayMix.entries.map { it.label }),
                "MEDIUM" to Pair(SynthParam.DelayMedium, DelayMediumKind.entries.map { it.label }),
            )) {
                val (param, labels) = row
                Row(verticalAlignment = Alignment.CenterVertically) {
                    BasicText(label, style = retroLabel(), modifier = Modifier.width(44.dp))
                    RetroSegmentRow(labels, vm[param].roundToInt(), Modifier.weight(1f)) { vm.set(param, it.toFloat()) }
                }
            }
        }
    }
}

/** Beat and unit on the left, the per-channel delays as heard on the right. */
@Composable
private fun DelayReadout(vm: SynthViewModel) {
    val times = vm.delayTimes()
    val subdivide = vm[SynthParam.DelaySubdivide].roundToInt().coerceAtLeast(1)
    val unitMs = vm[SynthParam.DelayTime] / subdivide
    val led = retroLabel(10.sp, RetroLed, androidx.compose.ui.text.font.FontWeight.SemiBold)
    Row(
        Modifier
            .fillMaxWidth()
            .background(Color.Black.copy(alpha = 0.6f))
            .border(1.dp, RetroAmber.copy(alpha = 0.3f), RoundedCornerShape(3.dp))
            .padding(6.dp),
        verticalAlignment = Alignment.Top,
    ) {
        Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
            BasicText("${vm[SynthParam.DelayTime].roundToInt()} MS", style = retroLabel(12.sp, RetroLed))
            BasicText(
                "UNIT ÷$subdivide = ${String.format(Locale.US, "%.1f", unitMs)}",
                style = retroLabel(8.sp, RetroAmber.copy(alpha = 0.8f), androidx.compose.ui.text.font.FontWeight.Normal),
            )
        }
        Spacer(Modifier.weight(1f))
        Column(horizontalAlignment = Alignment.End, verticalArrangement = Arrangement.spacedBy(2.dp)) {
            BasicText("L ${vm[SynthParam.DelayTimeLeft].roundToInt()}× ${(times.leftSeconds * 1000).roundToInt()} MS", style = led, maxLines = 1)
            BasicText("R ${vm[SynthParam.DelayTimeRight].roundToInt()}× ${(times.rightSeconds * 1000).roundToInt()} MS", style = led, maxLines = 1)
            // The setting asks for more than the delay line holds; it plays at the longest length.
            if (times.clamped) BasicText("MAX LENGTH", style = retroLabel(7.sp))
        }
    }
}

/** The tape reverb drawer (hall/plate reverb, as on iOS). */
@Composable
fun ReverbPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    RetroPanelChrome(modifier) {
        RetroHeader("TAPE REVERB", ledLit = vm[SynthParam.ReverbMix] > 0f)
        RetroSegmentRow(ReverbKind.entries.map { it.label }, vm.reverbKind.ordinal) {
            vm.set(SynthParam.ReverbType, it.toFloat())
        }
        // No Lo Cut / Damp knobs, as on iOS - the reverb type's own crossover and
        // damping defaults (ReverbKind) set those.
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceEvenly) {
            SmallKnob(vm, SynthParam.ReverbSize, "SIZE")
            SmallKnob(vm, SynthParam.ReverbPredelay, "PRE-DLY")
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.SpaceBetween) {
            SmallKnob(vm, SynthParam.ReverbLowDecay, "LO DECAY")
            SmallKnob(vm, SynthParam.ReverbHighDecay, "HI DECAY")
            SmallKnob(vm, SynthParam.ReverbBody, "BODY")
            SmallKnob(vm, SynthParam.ReverbShimmer, "AIR")
        }
        Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(4.dp)) {
            BasicText("MIX", style = retroLabel(9.sp))
            LedBarSlider(
                position = SynthParam.ReverbMix.toPosition(vm[SynthParam.ReverbMix]),
                onChange = { vm.set(SynthParam.ReverbMix, SynthParam.ReverbMix.fromPosition(it)) },
                description = "Reverb mix",
            )
        }
        ResetButton(vm::resetReverb)
    }
}

@Composable
private fun SmallKnob(vm: SynthViewModel, param: SynthParam, label: String, size: Dp = 42.dp) {
    ParamKnob(vm, param, size, label, labelStyle = retroLabel(7.sp), resettable = false)
}
