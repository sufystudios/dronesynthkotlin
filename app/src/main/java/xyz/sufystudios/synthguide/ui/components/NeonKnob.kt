package xyz.sufystudios.synthguide.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.gestures.awaitEachGesture
import androidx.compose.foundation.gestures.awaitFirstDown
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.gestures.drag
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.aspectRatio
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.text.BasicText
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.StrokeCap
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.input.pointer.positionChange
import androidx.compose.ui.semantics.ProgressBarRangeInfo
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.progressBarRangeInfo
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.setProgress
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.theme.Neon
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import kotlin.math.PI
import kotlin.math.cos
import kotlin.math.sin

/**
 * The app's knob: a black face with a glowing green pointer, sweeping 288° from
 * the 7 o'clock position. Drag up or right to increase; double tap to reset.
 *
 * @param position knob position, 0..1 (map real values with [SynthParam.toPosition]).
 */
@Composable
fun NeonKnob(
    position: Float,
    onPositionChange: (Float) -> Unit,
    modifier: Modifier = Modifier,
    description: String = "",
    enabled: Boolean = true,
    onDoubleTap: (() -> Unit)? = null,
) {
    val current by rememberUpdatedState(position)
    val onChange by rememberUpdatedState(onPositionChange)
    val alpha = if (enabled) 1f else 0.4f
    Canvas(
        modifier
            .aspectRatio(1f)
            .semantics {
                contentDescription = description
                progressBarRangeInfo = ProgressBarRangeInfo(position, 0f..1f)
                setProgress { onChange(it.coerceIn(0f, 1f)); true }
            }
            .pointerInput(Unit) {
                // The knob takes the touch as it lands, with no slop to cross first, so it
                // follows the finger from the first pixel and a drawer's scroll can't steal it.
                awaitEachGesture {
                    val down = awaitFirstDown(requireUnconsumed = false)
                    var dragged = current
                    drag(down.id) { change ->
                        val amount = change.positionChange()
                        change.consume()
                        // Full sweep takes about 2.5 knob-widths of travel, horizontal or vertical.
                        dragged = (dragged + (amount.x - amount.y) / (size.width * 2.5f)).coerceIn(0f, 1f)
                        onChange(dragged)
                    }
                }
            }
            .pointerInput(onDoubleTap) {
                if (onDoubleTap != null) detectTapGestures(onDoubleTap = { onDoubleTap() })
            }
    ) {
        val r = size.minDimension / 2f
        val c = center
        drawCircle(Color.Black, r)
        drawCircle(Neon.copy(alpha = 0.25f * alpha), r - 1.dp.toPx(), style = Stroke(2.dp.toPx()))

        val angle = position.coerceIn(0f, 1f) * 1.6f * PI.toFloat() + 0.2f * PI.toFloat()
        val dir = Offset(-sin(angle), cos(angle))
        val mid = r * 0.75f
        val half = size.minDimension / 6.4f
        val start = c + dir * (mid - half)
        val end = c + dir * (mid + half)
        val width = size.minDimension / 22f
        // Layered strokes stand in for the iOS version's double shadow glow.
        drawLine(Neon.copy(alpha = 0.12f * alpha), start, end, width * 5f, StrokeCap.Round)
        drawLine(Neon.copy(alpha = 0.3f * alpha), start, end, width * 2.6f, StrokeCap.Round)
        drawLine(Neon.copy(alpha = alpha), start, end, width)
    }
}

/** A [NeonKnob] bound to a [SynthParam], following its taper, with an optional caption. */
@Composable
fun ParamKnob(
    vm: SynthViewModel,
    param: SynthParam,
    size: Dp,
    label: String? = null,
    modifier: Modifier = Modifier,
    labelStyle: androidx.compose.ui.text.TextStyle = retroLabel(),
    enabled: Boolean = true,
    resettable: Boolean = true,
) {
    Column(modifier, horizontalAlignment = Alignment.CenterHorizontally, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        NeonKnob(
            position = param.toPosition(vm[param]),
            onPositionChange = { vm.set(param, param.fromPosition(it)) },
            modifier = Modifier.size(size),
            description = label ?: param.key,
            enabled = enabled,
            onDoubleTap = if (resettable) ({ vm.set(param, param.default) }) else null,
        )
        if (label != null) BasicText(label, style = labelStyle)
    }
}

/** Plain white caption under the main-screen knobs. */
val mainLabel = androidx.compose.ui.text.TextStyle(color = Color.White, fontSize = 11.sp)
