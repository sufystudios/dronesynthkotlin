package xyz.sufystudios.synthguide.ui.panels

import androidx.activity.compose.LocalActivity
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.alpha
import androidx.compose.ui.draw.clip
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.audio.NativeSynth.LooperCommand
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.model.TakeRecorder
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroPowerSwitch
import xyz.sufystudios.synthguide.ui.components.StatusLed
import xyz.sufystudios.synthguide.ui.theme.Neon
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel

private val CapRed = Color(0xFFE5392E)
private val CapDark = Color(0xFF262626)

/**
 * The Looper drawer: records up to 30 s of the synth's own output, loops it, and
 * overdubs further passes on top. Styled as a hardware stomp panel, as on iOS.
 */
@Composable
fun LooperPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val state = vm.looper
    Column(
        modifier
            .clip(RoundedCornerShape(6.dp))
            .background(Color(0xFF171717))
            .padding(horizontal = 16.dp, vertical = 20.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(20.dp),
    ) {
        BasicText(
            when {
                state.recording -> "REC ${formatTime(state.recordedSeconds)}"
                state.hasLoop -> "${formatTime(state.positionSeconds)} / ${formatTime(state.lengthSeconds)}"
                else -> "NO LOOP"
            },
            style = retroLabel(12.sp, RetroLed),
        )
        LooperCapButton("Play/Stop", Neon, active = state.playing, size = 52.dp, enabled = state.hasLoop) {
            vm.looperCommand(LooperCommand.TogglePlay)
        }
        Row(horizontalArrangement = Arrangement.spacedBy(36.dp)) {
            LooperCapButton("Record", CapRed, active = state.recording) { vm.looperCommand(LooperCommand.ToggleRecord) }
            LooperCapButton("Overdub", CapRed, active = state.overdubbing, enabled = state.hasLoop && !state.recording) {
                vm.looperCommand(LooperCommand.ToggleOverdub)
            }
        }
        Row(horizontalArrangement = Arrangement.spacedBy(36.dp)) {
            LooperCapButton("Undo", CapDark, enabled = state.canUndo) { vm.looperCommand(LooperCommand.Undo) }
            LooperCapButton("Clear", CapDark, enabled = state.hasLoop) { vm.looperCommand(LooperCommand.Clear) }
        }
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            StatusLed(state.enabled && state.playing)
            Spacer(Modifier.size(6.dp))
            BasicText("LOOPER", style = retroLabel(13.sp, Color.White.copy(alpha = 0.8f)))
            Spacer(Modifier.weight(1f))
            RetroPowerSwitch(state.enabled, vm::toggleLooperEnabled, "Looper output")
        }
    }
}

/** Metal-bezel push button with a coloured cap, like a hardware stomp switch. */
@Composable
internal fun LooperCapButton(
    label: String,
    cap: Color,
    active: Boolean = false,
    size: Dp = 46.dp,
    enabled: Boolean = true,
    onClick: () -> Unit,
) {
    Column(
        Modifier
            .alpha(if (enabled) 1f else 0.4f)
            .semantics { contentDescription = label; stateDescription = if (active) "On" else "Off" }
            .clickable(enabled = enabled, role = Role.Button, onClick = onClick),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        Canvas(Modifier.size(size)) {
            val r = this.size.minDimension / 2f
            drawCircle(Brush.sweepGradient(listOf(Color(0xFFCCCCCC), Color(0xFF595959), Color(0xFFCCCCCC), Color(0xFF595959), Color(0xFFCCCCCC))), r)
            drawCircle(Color.Black.copy(alpha = 0.6f), r - 1.dp.toPx(), style = Stroke(2.dp.toPx()))
            val capR = r * 0.6f
            if (active) drawCircle(cap.copy(alpha = 0.35f), capR * 1.45f)
            drawCircle(
                Brush.radialGradient(
                    listOf(cap.copy(alpha = if (active) 1f else 0.8f), cap.copy(alpha = if (active) 0.75f else 0.4f)),
                    center = center - Offset(capR * 0.4f, capR * 0.4f),
                    radius = capR * 1.6f,
                ),
                capR,
            )
        }
        BasicText(label.uppercase(), style = retroLabel())
    }
}

/** The REC drawer: one take of up to 3 minutes, with playback and Share. */
@Composable
fun RecorderPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val rec = vm.recorder
    val state = rec.state
    val activity = LocalActivity.current
    RetroPanelChrome(modifier) {
        RetroHeader("RECORDER", ledLit = state.recording || state.playing)
        BasicText(
            "${formatTime(state.elapsedSeconds)} / ${formatTime(TakeRecorder.MAX_SECONDS)}",
            style = retroLabel(13.sp, RetroLed),
        )
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
            val fraction = state.elapsedSeconds / TakeRecorder.MAX_SECONDS
            repeat(10) { i ->
                Box(Modifier.weight(1f).height(14.dp).background(if (i / 10f < fraction) RetroLed else Color(0xFF4D4D4D)))
            }
        }
        Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(8.dp)) {
            FlatButton("Record", RetroLed, active = state.recording, modifier = Modifier.weight(1f)) { rec.toggleRecord() }
            FlatButton("Play", Neon, active = state.playing, enabled = state.hasTake && !state.recording, modifier = Modifier.weight(1f)) {
                rec.togglePlay()
            }
            FlatButton("Stop", Color(0xFF999999), modifier = Modifier.weight(1f)) { rec.stopTransport() }
        }
        FlatButton(
            if (state.preparingShare) "…" else "Share",
            RetroAmber,
            enabled = state.hasTake && !state.preparingShare,
            modifier = Modifier.fillMaxWidth(),
        ) { activity?.let(rec::share) }
    }
}

@Composable
private fun FlatButton(
    label: String,
    accent: Color,
    modifier: Modifier = Modifier,
    active: Boolean = false,
    enabled: Boolean = true,
    onClick: () -> Unit,
) {
    Box(
        modifier
            .alpha(if (enabled) 1f else 0.4f)
            .clip(RoundedCornerShape(4.dp))
            .background(if (active) accent.copy(alpha = 0.85f) else Color.Black.copy(alpha = 0.5f))
            .border(1.dp, accent.copy(alpha = 0.6f), RoundedCornerShape(4.dp))
            .clickable(enabled = enabled, role = Role.Button, onClick = onClick)
            .padding(vertical = 9.dp),
        contentAlignment = Alignment.Center,
    ) {
        BasicText(label.uppercase(), style = retroLabel(9.sp, if (active) Color.White else accent), maxLines = 1)
    }
}

internal fun formatTime(seconds: Float): String {
    val s = seconds.coerceAtLeast(0f).toInt()
    return "${s / 60}:${(s % 60).toString().padStart(2, '0')}"
}
