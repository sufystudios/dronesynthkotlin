package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.clearAndSetSemantics
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.NeonKnob
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.pow
import kotlin.math.roundToInt
import kotlin.math.sin

/** A4 at a master tuning: 440 Hz moved by [cents] hundredths of a semitone. */
internal fun a4Frequency(cents: Float): Double = 440.0 * 2.0.pow(cents / 1200.0)

/**
 * "TUNE" drawer: the whole synth's tuning, in cents from standard pitch (A4 = 440 Hz),
 * with the resulting A4 to a tenth of a hertz, as on iOS.
 */
@Composable
fun TuningPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val param = SynthParam.MasterTune
    val cents = vm[param]
    val roundedCents = cents.roundToInt()
    RetroPanelChrome(modifier) {
        RetroHeader("TUNE", ledLit = roundedCents != 0)

        // The readout: A4 large, the detune in cents beneath it.
        Column(
            Modifier
                .fillMaxWidth()
                .clip(RoundedCornerShape(3.dp))
                .background(Color.Black.copy(alpha = 0.6f))
                .border(1.dp, RetroAmber.copy(alpha = 0.3f), RoundedCornerShape(3.dp))
                .padding(vertical = 10.dp)
                .semantics(mergeDescendants = true) {},
            horizontalAlignment = Alignment.CenterHorizontally,
            verticalArrangement = Arrangement.spacedBy(4.dp),
        ) {
            BasicText("A4", style = retroLabel(color = RetroAmber.copy(alpha = 0.85f)))
            BasicText("%.1f Hz".format(a4Frequency(cents)), style = retroLabel(26.sp, RetroLed, FontWeight.SemiBold), maxLines = 1)
            BasicText(
                if (roundedCents == 0) "STANDARD PITCH" else "%+d CENTS".format(roundedCents),
                style = retroLabel(10.sp, RetroLed.copy(alpha = 0.8f), FontWeight.SemiBold),
            )
        }

        // The knob, 0 straight up, with its scale around it; double-tap resets.
        Box(Modifier.size(130.dp), contentAlignment = Alignment.Center) {
            KnobScale()
            NeonKnob(
                position = param.toPosition(cents),
                onPositionChange = { vm.set(param, param.fromPosition(it)) },
                modifier = Modifier
                    .size(92.dp)
                    .semantics { stateDescription = "%+d cents".format(roundedCents) },
                description = "Master tune",
                onDoubleTap = { vm.set(param, param.default) },
            )
        }

        // One cent at a time, for landing exactly on a pitch like 432 Hz.
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(18.dp)) {
            // Steps to the next whole cent, so a dragged in-between value lands on one.
            NudgeButton("−", "Down one cent") { vm.set(param, (cents - 1f).roundToInt().toFloat()) }
            BasicText("1 CENT", style = retroLabel(color = RetroAmber.copy(alpha = 0.7f)))
            NudgeButton("+", "Up one cent") { vm.set(param, (cents + 1f).roundToInt().toFloat()) }
        }

        BasicText("DOUBLE-TAP KNOB TO RESET", style = retroLabel(color = RetroAmber.copy(alpha = 0.6f)))
    }
}

@Composable
private fun NudgeButton(symbol: String, description: String, onClick: () -> Unit) {
    Box(
        Modifier
            .size(34.dp, 26.dp)
            .clip(RoundedCornerShape(3.dp))
            .background(Color.Black.copy(alpha = 0.5f))
            .border(1.dp, RetroAmber.copy(alpha = 0.4f), RoundedCornerShape(3.dp))
            .semantics { contentDescription = description }
            .clickable(role = Role.Button, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        BasicText(symbol, style = retroLabel(14.sp))
    }
}

/** Tick marks around the knob's sweep, labelled at -100, 0 and +100 cents. */
@Composable
private fun KnobScale() {
    Box(Modifier.fillMaxSize().clearAndSetSemantics {}, contentAlignment = Alignment.Center) {
        Canvas(Modifier.fillMaxSize()) {
            val radius = size.minDimension / 2f
            for (i in 0..20) {
                // The same angles NeonKnob's pointer sweeps through.
                val angle = i / 20f * 1.6f * PI.toFloat() + 0.2f * PI.toFloat()
                val dir = Offset(-sin(angle), cos(angle))
                val major = i % 10 == 0
                val length = (if (major) 8 else 4).dp.toPx()
                drawLine(
                    if (major) RetroAmber else RetroAmber.copy(alpha = 0.4f),
                    center + dir * radius, center + dir * (radius - length),
                    (if (major) 2 else 1).dp.toPx(),
                )
            }
        }
        ScaleLabel("-100", 0.2 * PI, 75)
        ScaleLabel("0", PI, 73)
        ScaleLabel("+100", 1.8 * PI, 75)
    }
}

@Composable
private fun ScaleLabel(text: String, angle: Double, radius: Int) {
    BasicText(
        text,
        Modifier.offset((-sin(angle) * radius).dp, (cos(angle) * radius).dp),
        style = retroLabel(color = RetroAmber.copy(alpha = 0.8f)),
    )
}
