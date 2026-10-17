package xyz.sufystudios.synthguide.ui.components

import androidx.compose.animation.core.LinearEasing
import androidx.compose.animation.core.RepeatMode
import androidx.compose.animation.core.animateFloat
import androidx.compose.animation.core.infiniteRepeatable
import androidx.compose.animation.core.rememberInfiniteTransition
import androidx.compose.animation.core.tween
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectDragGestures
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.ColumnScope
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.offset
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.draw.drawBehind
import androidx.compose.ui.draw.rotate
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.graphics.drawscope.rotate
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroDim
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.RetroPanel
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import kotlin.math.roundToInt

/** Shared chrome for every drawer: dark brown panel, thin amber border. */
@Composable
fun RetroPanelChrome(modifier: Modifier = Modifier, content: @Composable ColumnScope.() -> Unit) {
    Column(
        modifier
            .clip(RoundedCornerShape(6.dp))
            .background(RetroPanel)
            .border(1.dp, RetroAmber.copy(alpha = 0.4f), RoundedCornerShape(6.dp))
            .padding(14.dp),
        verticalArrangement = Arrangement.spacedBy(10.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        content = content,
    )
}

/** Panel title row: optional reels either side, a title and a status LED. */
@Composable
fun RetroHeader(title: String, ledLit: Boolean? = null, reels: Boolean = false) {
    Row(Modifier.fillMaxWidth(), verticalAlignment = Alignment.CenterVertically) {
        if (reels) {
            RetroReel()
            Spacer(Modifier.width(6.dp))
        }
        BasicText(title, style = retroLabel(11.sp), maxLines = 1)
        Spacer(Modifier.weight(1f))
        if (ledLit != null) StatusLed(ledLit)
        if (reels) {
            Spacer(Modifier.width(6.dp))
            RetroReel()
        }
    }
}

/** A small LED dot, lit orange or dim grey. */
@Composable
fun StatusLed(lit: Boolean) {
    Canvas(Modifier.size(10.dp)) {
        if (lit) drawCircle(RetroLed.copy(alpha = 0.35f), size.minDimension / 2f)
        drawCircle(if (lit) RetroLed else Color(0xFF404040), 3.dp.toPx())
    }
}

/** A spinning reel, like the tape transports on the iOS drawers. */
@Composable
fun RetroReel(spinning: Boolean = true) {
    val angle by rememberInfiniteTransition(label = "reel").animateFloat(
        0f, 360f, infiniteRepeatable(tween(9_000, easing = LinearEasing), RepeatMode.Restart), label = "reelAngle",
    )
    val grey = Color(0xFF808080)
    Canvas(Modifier.size(20.dp).rotate(if (spinning) angle else 0f)) {
        val r = size.minDimension / 2f
        drawCircle(grey, r - 1.dp.toPx(), style = Stroke(2.dp.toPx()))
        for (i in 0 until 3) {
            rotate(i * 120f) { drawLine(grey, center - Offset(0f, 5.dp.toPx()), center + Offset(0f, 5.dp.toPx()), 1.5.dp.toPx()) }
        }
        drawCircle(grey, 2.dp.toPx())
    }
}

/** A row of tappable text tabs in the retro palette (stands in for a segmented control). */
@Composable
fun RetroTabBar(labels: List<String>, selected: Int, onSelect: (Int) -> Unit, modifier: Modifier = Modifier) {
    Row(modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(4.dp)) {
        labels.forEachIndexed { i, label ->
            Box(
                Modifier
                    .weight(1f)
                    .background(Color.Black.copy(alpha = 0.5f))
                    .clickable(role = Role.Tab) { onSelect(i) }
                    .padding(vertical = 6.dp),
                contentAlignment = Alignment.Center,
            ) {
                BasicText(
                    label.uppercase(),
                    style = retroLabel(9.sp, if (i == selected) RetroLed else RetroDim),
                    maxLines = 1,
                )
            }
        }
    }
}

/** Small pill on/off switch, standing in for a hardware rocker. */
@Composable
fun RetroPowerSwitch(isOn: Boolean, onToggle: () -> Unit, description: String = "") {
    Box(
        Modifier
            .size(40.dp, 22.dp)
            .clip(CircleShape)
            .background(if (isOn) RetroLed.copy(alpha = 0.6f) else Color.Black.copy(alpha = 0.6f))
            .border(1.dp, Color.White.copy(alpha = 0.2f), CircleShape)
            .semantics { contentDescription = description; stateDescription = if (isOn) "On" else "Off" }
            .clickable(role = Role.Switch, onClick = onToggle)
            .padding(2.dp),
        contentAlignment = if (isOn) Alignment.CenterEnd else Alignment.CenterStart,
    ) {
        Box(Modifier.size(18.dp).clip(CircleShape).background(Color(0xFFD9D9D9)))
    }
}

/** LED + caption that toggles, used for LFO enable and OSC 3 sync. */
@Composable
fun LedToggle(label: String, lit: Boolean, onToggle: () -> Unit) {
    Row(
        Modifier
            .semantics { stateDescription = if (lit) "On" else "Off" }
            .clickable(role = Role.Switch, onClick = onToggle)
            .padding(4.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        StatusLed(lit)
        BasicText(label.uppercase(), style = retroLabel())
    }
}

/** A row of tappable segments bound to an index. */
@Composable
fun RetroSegmentRow(labels: List<String>, selected: Int, onSelect: (Int) -> Unit) {
    Row(Modifier.fillMaxWidth(), horizontalArrangement = Arrangement.spacedBy(3.dp)) {
        labels.forEachIndexed { i, label ->
            Box(
                Modifier
                    .weight(1f)
                    .background(Color.Black.copy(alpha = 0.5f))
                    .clickable(role = Role.RadioButton) { onSelect(i) }
                    .padding(vertical = 6.dp),
                contentAlignment = Alignment.Center,
            ) {
                BasicText(
                    label.uppercase(),
                    style = retroLabel(8.sp, if (i == selected) RetroLed else RetroDim).copy(textAlign = TextAlign.Center),
                    maxLines = 1,
                )
            }
        }
    }
}

/** −/+ stepper for whole-step values (octaves, semitones): exact taps, no drag to fight. */
@Composable
fun WholeStepStepper(value: Int, range: IntRange, title: String, onChange: (Int) -> Unit) {
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
        StepButton("−", enabled = value > range.first) { onChange(value - 1) }
        Column(Modifier.widthIn(min = 56.dp), horizontalAlignment = Alignment.CenterHorizontally) {
            BasicText(
                if (value > 0) "+$value" else "$value",
                style = retroLabel(15.sp, if (value == 0) RetroLed else Color.White),
            )
            BasicText(title.uppercase(), style = retroLabel())
        }
        StepButton("+", enabled = value < range.last) { onChange(value + 1) }
    }
}

@Composable
private fun StepButton(symbol: String, enabled: Boolean, onClick: () -> Unit) {
    Box(
        Modifier
            .size(32.dp)
            .clip(CircleShape)
            .background(Color.Black.copy(alpha = 0.4f))
            .clickable(enabled = enabled, role = Role.Button, onClick = onClick),
        contentAlignment = Alignment.Center,
    ) {
        BasicText(symbol, style = retroLabel(14.sp, if (enabled) Color.White else RetroDim))
    }
}

/**
 * Horizontal whole-step fader modelled on Calf's: label on the left, a groove with
 * sockets at each end, and a dark cap with an LED centre line. Snaps to integers.
 */
@Composable
fun RetroFader(label: String, value: Float, range: ClosedFloatingPointRange<Float>, onChange: (Float) -> Unit) {
    val onValue by rememberUpdatedState(onChange)
    val capWidth = 26.dp
    Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        BasicText(label, style = retroLabel(9.sp), modifier = Modifier.width(58.dp), maxLines = 1)
        BoxWithConstraints(
            Modifier
                .weight(1f)
                .height(34.dp)
                .semantics { contentDescription = label; stateDescription = "${value.roundToInt()}" }
        ) {
            val density = LocalDensity.current
            val capPx = with(density) { capWidth.toPx() }
            val usable = (constraints.maxWidth - capPx).coerceAtLeast(1f)
            val span = range.endInclusive - range.start
            val fraction = ((value.coerceIn(range) - range.start) / span)
            fun valueAt(x: Float): Float {
                val f = ((x - capPx / 2f) / usable).coerceIn(0f, 1f)
                return (range.start + f * span).roundToInt().toFloat()
            }
            Box(
                Modifier
                    .matchParentSize()
                    .pointerInput(range) {
                        detectTapGestures { onValue(valueAt(it.x)) }
                    }
                    .pointerInput(range) {
                        detectDragGestures { change, _ -> change.consume(); onValue(valueAt(change.position.x)) }
                    }
                    .drawBehind {
                        val y = size.height / 2f + 6.dp.toPx()
                        drawLine(Color.Black.copy(alpha = 0.75f), Offset(capPx / 2f, y), Offset(size.width - capPx / 2f, y), 3.dp.toPx())
                        for (x in listOf(4.dp.toPx(), size.width - 4.dp.toPx())) {
                            drawCircle(Color(0xFF1A1A1A), 4.dp.toPx(), Offset(x, y))
                            drawCircle(Color.White.copy(alpha = 0.25f), 4.dp.toPx(), Offset(x, y), style = Stroke(1.dp.toPx()))
                        }
                    }
            )
            Column(
                Modifier.offset(x = with(density) { (usable * fraction).toDp() }),
                horizontalAlignment = Alignment.CenterHorizontally,
            ) {
                BasicText("${value.roundToInt()}", style = retroLabel(9.sp, RetroAmber, androidx.compose.ui.text.font.FontWeight.SemiBold))
                Box(
                    Modifier
                        .size(capWidth, 18.dp)
                        .clip(RoundedCornerShape(3.dp))
                        .background(Brush.verticalGradient(listOf(Color(0xFF474747), Color(0xFF141414)))),
                    contentAlignment = Alignment.Center,
                ) {
                    Box(Modifier.size(2.dp, 12.dp).background(RetroLed))
                }
            }
        }
    }
}

/** A draggable row of LED segments, standing in for a slider. */
@Composable
fun LedBarSlider(position: Float, onChange: (Float) -> Unit, segments: Int = 10, description: String = "") {
    val onValue by rememberUpdatedState(onChange)
    val segmentWidth = 8.dp
    Canvas(
        Modifier
            .size(segmentWidth * segments, 18.dp)
            .semantics { contentDescription = description; stateDescription = "${(position * 100).roundToInt()}%" }
            .pointerInput(Unit) { detectTapGestures { onValue((it.x / size.width).coerceIn(0f, 1f)) } }
            .pointerInput(Unit) {
                detectDragGestures { change, _ -> change.consume(); onValue((change.position.x / size.width).coerceIn(0f, 1f)) }
            }
    ) {
        val w = segmentWidth.toPx()
        for (i in 0 until segments) {
            val lit = i.toFloat() / segments < position
            drawRect(
                if (lit) RetroLed else Color(0xFF4D4D4D),
                topLeft = Offset(i * w, 2.dp.toPx()),
                size = androidx.compose.ui.geometry.Size(5.dp.toPx(), 14.dp.toPx()),
            )
        }
    }
}

/** The plain "Reset" button at the bottom of the Delay and Reverb drawers. */
@Composable
fun ResetButton(onClick: () -> Unit) {
    Box(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(6.dp))
            .background(Color.Black.copy(alpha = 0.4f))
            .clickable(role = Role.Button, onClick = onClick)
            .padding(vertical = 8.dp),
        contentAlignment = Alignment.Center,
    ) {
        BasicText("Reset", style = retroLabel(11.sp, Color.White, androidx.compose.ui.text.font.FontWeight.SemiBold))
    }
}
