package xyz.sufystudios.synthguide.ui.panels

import androidx.compose.animation.core.animateDpAsState
import androidx.compose.animation.core.tween
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import kotlinx.coroutines.delay
import xyz.sufystudios.synthguide.model.ArpOrder
import xyz.sufystudios.synthguide.model.Sequencer
import xyz.sufystudios.synthguide.model.SequencerMode
import xyz.sufystudios.synthguide.model.SequencerSettings
import xyz.sufystudios.synthguide.model.SequencerState
import xyz.sufystudios.synthguide.model.SequencerStep
import xyz.sufystudios.synthguide.model.StepDivision
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.NeonKnob
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroReel
import xyz.sufystudios.synthguide.ui.components.RetroTabBar
import xyz.sufystudios.synthguide.ui.components.StatusLed
import xyz.sufystudios.synthguide.ui.theme.Neon
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroDim
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.roundToInt

private val CapBlue = Color(0xFF0A84FF)

/**
 * The ARP/SEQ drawer: the Grandmother-style arpeggiator and step sequencer, built
 * from the retro parts the other drawers use, as on iOS.
 */
@Composable
fun ArpSeqPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val s = vm.sequencer
    val state = vm.sequencerState
    // Tight spacing, caps and knobs keep the drawer short enough for a phone in landscape.
    RetroPanelChrome(modifier, spacing = 6.dp) {
        Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
            StatusLed(s.playing)
            Spacer(Modifier.width(6.dp))
            BasicText("ARP / SEQ", style = retroLabel(13.sp, Color.White.copy(alpha = 0.85f)))
            Spacer(Modifier.weight(1f))
            RetroReel(spinning = s.playing && s.mode != SequencerMode.Rec)
        }

        ArpSeqReadout(s, state)

        ThreeWaySwitch("MODE", SequencerMode.entries.map { it.label }, s.mode.ordinal) { i ->
            vm.updateSequencer { it.copy(mode = SequencerMode.entries[i]) }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(14.dp), verticalAlignment = Alignment.Top) {
            LooperCapButton("Play", Neon, active = s.playing, size = 36.dp, enabled = s.mode != SequencerMode.Rec) {
                vm.updateSequencer { it.copy(playing = !it.playing) }
            }
            if (s.mode == SequencerMode.Rec) {
                LooperCapButton("Rest/Tie", CapBlue, size = 36.dp, onClick = vm::sequencerRestOrTie)
            } else {
                LooperCapButton("Hold", CapBlue, active = s.hold, size = 36.dp) { vm.updateSequencer { it.copy(hold = !it.hold) } }
            }
            TempoTapButton(vm::sequencerTapTempo)
        }

        Row(horizontalArrangement = Arrangement.spacedBy(24.dp), verticalAlignment = Alignment.Top) {
            Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(3.dp)) {
                // RATE spans 20...300 BPM on a logarithmic taper, like the app's time knobs.
                NeonKnob(
                    position = bpmPosition(s.bpm),
                    onPositionChange = { p -> vm.updateSequencer { it.copy(bpm = bpmAt(p)) } },
                    modifier = Modifier.size(40.dp),
                    description = "Rate",
                    onDoubleTap = { vm.updateSequencer { it.copy(bpm = SequencerSettings().bpm) } },
                )
                Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                    SequencerClockLed(state.clockTicks)
                    BasicText("RATE", style = retroLabel())
                }
            }
            Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(3.dp)) {
                val min = SequencerSettings.MIN_GATE
                NeonKnob(
                    position = (s.gate - min) / (1f - min),
                    onPositionChange = { p -> vm.updateSequencer { it.copy(gate = min + p * (1f - min)) } },
                    modifier = Modifier.size(40.dp),
                    description = "Gate",
                    onDoubleTap = { vm.updateSequencer { it.copy(gate = SequencerSettings().gate) } },
                )
                BasicText("GATE", style = retroLabel())
            }
        }

        Column(horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(2.dp)) {
            BasicText("DIVISION", style = retroLabel(color = RetroAmber.copy(alpha = 0.85f)))
            RetroTabBar(StepDivision.entries.map { it.label }, s.division.ordinal, { i ->
                vm.updateSequencer { it.copy(division = StepDivision.entries[i]) }
            })
        }

        if (s.mode == SequencerMode.Arp) {
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp)) {
                ThreeWaySwitch("ORDER", ArpOrder.entries.map { it.label }, s.order.ordinal, Modifier.weight(1f)) { i ->
                    vm.updateSequencer { it.copy(order = ArpOrder.entries[i]) }
                }
                ThreeWaySwitch("OCTAVES", listOf("0", "1", "2"), s.extraOctaves, Modifier.width(66.dp)) { i ->
                    vm.updateSequencer { it.copy(extraOctaves = i) }
                }
            }
        } else {
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp), verticalAlignment = Alignment.Bottom) {
                ThreeWaySwitch("PATTERN", listOf("1", "2", "3"), s.pattern, Modifier.weight(1f)) { i ->
                    vm.updateSequencer { it.copy(pattern = i) }
                }
                Box(
                    Modifier
                        .clip(RoundedCornerShape(3.dp))
                        .border(1.dp, RetroAmber.copy(alpha = 0.4f), RoundedCornerShape(3.dp))
                        .clickable(role = Role.Button, onClick = vm::clearSequencerPattern)
                        .padding(horizontal = 8.dp, vertical = 4.dp),
                ) {
                    BasicText("CLEAR", style = retroLabel(9.sp))
                }
            }
        }

        StepLeds(s, state)
    }
}

private val bpmSpan = ln(SequencerSettings.MAX_BPM / SequencerSettings.MIN_BPM)
private fun bpmPosition(bpm: Double) = (ln(bpm / SequencerSettings.MIN_BPM) / bpmSpan).toFloat()
private fun bpmAt(position: Float) = SequencerSettings.MIN_BPM * exp(position * bpmSpan)

/**
 * A hardware-style three-way toggle: its labels above a slot with the lever at the
 * selected position. Tap a position's label or slot to move it there.
 */
@Composable
private fun ThreeWaySwitch(title: String, labels: List<String>, selected: Int, modifier: Modifier = Modifier, onSelect: (Int) -> Unit) {
    Column(
        modifier.semantics { contentDescription = title; stateDescription = labels[selected.coerceIn(labels.indices)] },
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(3.dp),
    ) {
        BasicText(title, style = retroLabel(color = RetroAmber.copy(alpha = 0.85f)))
        Row(Modifier.fillMaxWidth()) {
            labels.forEachIndexed { i, label ->
                Box(Modifier.weight(1f).clickable(role = Role.RadioButton) { onSelect(i) }, contentAlignment = Alignment.Center) {
                    BasicText(label, style = retroLabel(9.sp, if (i == selected) RetroLed else RetroDim), maxLines = 1)
                }
            }
        }
        BoxWithConstraints(
            Modifier
                .fillMaxWidth()
                .height(14.dp)
                .clip(CircleShape)
                .background(Color.Black.copy(alpha = 0.7f))
                .pointerInput(labels.size) {
                    detectTapGestures { onSelect((it.x / size.width * labels.size).toInt().coerceIn(labels.indices)) }
                },
        ) {
            val slot = maxWidth / labels.size
            val x by animateDpAsState(slot * (selected + 0.5f) - 8.dp, tween(120), label = "lever")
            Box(
                Modifier
                    .align(Alignment.CenterStart)
                    .offset(x = x)
                    .size(16.dp, 12.dp)
                    .clip(RoundedCornerShape(3.dp))
                    .background(Brush.verticalGradient(listOf(Color(0xFFD9D9D9), Color(0xFF737373)))),
            )
        }
    }
}

/** TAP for the sequencer's own tempo, apart from the delay's. */
@Composable
private fun TempoTapButton(onTap: () -> Unit) {
    Column(
        Modifier
            .semantics { contentDescription = "Tap tempo" }
            .clickable(role = Role.Button, onClick = onTap),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        Box(
            Modifier.size(40.dp).clip(CircleShape).background(Brush.radialGradient(listOf(RetroLed, Color(0xFF801A0D)))),
            contentAlignment = Alignment.Center,
        ) {
            BasicText("TAP", style = retroLabel(9.sp, Color.White))
        }
        BasicText("TEMPO", style = retroLabel())
    }
}

/** Tempo and division on the left, what's playing on the right. */
@Composable
private fun ArpSeqReadout(s: SequencerSettings, state: SequencerState) {
    val pattern = "P${s.pattern + 1}"
    val detail = when (s.mode) {
        SequencerMode.Arp -> s.order.label + if (s.extraOctaves > 0) " +${s.extraOctaves} OCT" else ""
        SequencerMode.Seq -> when {
            state.pattern.isEmpty() -> "$pattern EMPTY"
            state.running -> "$pattern ${state.currentStep + 1}/${state.pattern.size}"
            else -> "$pattern ${state.pattern.size} STEPS"
        }
        SequencerMode.Rec -> "REC ${state.currentStep.coerceAtLeast(0)}/${Sequencer.MAX_STEPS}"
    }
    Row(
        Modifier
            .fillMaxWidth()
            .background(Color.Black.copy(alpha = 0.6f))
            .border(1.dp, RetroAmber.copy(alpha = 0.3f), RoundedCornerShape(3.dp))
            .padding(6.dp),
    ) {
        val style = retroLabel(10.sp, RetroLed, androidx.compose.ui.text.font.FontWeight.SemiBold)
        BasicText("${s.bpm.roundToInt()} BPM ${s.division.label}", style = style, maxLines = 1)
        Spacer(Modifier.weight(1f).width(6.dp))
        BasicText(detail, style = style, maxLines = 1)
    }
}

/**
 * Sixteen step LEDs, showing the page of sixteen the playing (or next recorded)
 * step is on: that step bright, the pattern's other notes dim, rests hollow.
 */
@Composable
private fun StepLeds(s: SequencerSettings, state: SequencerState) {
    // In ARP there's no stored pattern: just the step playing.
    val steps = if (s.mode == SequencerMode.Arp) emptyList() else state.pattern
    val current = when {
        s.mode == SequencerMode.Rec -> state.currentStep
        state.running -> state.currentStep
        else -> null
    }
    val page = (current ?: 0).coerceAtLeast(0) / 16 * 16
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
        repeat(16) { i ->
            val index = page + i
            val kind = steps.getOrNull(index)?.kind
            val shape = RoundedCornerShape(1.5.dp)
            val color = when {
                index == current -> RetroLed
                kind == SequencerStep.Kind.Note || kind == SequencerStep.Kind.Tie -> RetroLed.copy(alpha = 0.3f)
                else -> Color(0xFF333333)
            }
            Box(
                Modifier
                    .weight(1f)
                    .height(8.dp)
                    .clip(shape)
                    .background(color)
                    .border(1.dp, if (kind == SequencerStep.Kind.Rest) RetroLed.copy(alpha = 0.5f) else Color.Transparent, shape),
            )
        }
    }
}

/**
 * The RATE LED: flashes on every clock pulse, playing or not, so the speed shows
 * while the knob turns (as on the Grandmother).
 */
@Composable
fun SequencerClockLed(clockTicks: Int) {
    var lit by remember { mutableStateOf(false) }
    LaunchedEffect(clockTicks) {
        lit = true
        delay(50)
        lit = false
    }
    StatusLed(lit)
}
