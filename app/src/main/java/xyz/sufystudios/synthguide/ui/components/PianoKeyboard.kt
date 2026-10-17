package xyz.sufystudios.synthguide.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.rememberUpdatedState
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.CornerRadius
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.geometry.Rect
import androidx.compose.ui.geometry.Size
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.drawscope.DrawScope
import androidx.compose.ui.graphics.drawscope.drawIntoCanvas
import androidx.compose.ui.graphics.nativeCanvas
import androidx.compose.ui.graphics.toArgb
import androidx.compose.ui.input.pointer.PointerId
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.unit.dp
import xyz.sufystudios.synthguide.ui.theme.Neon

private val WhiteKey = Color(0xFFEBEBEB)
private val BlackKey = Color(0xFF141414)
private val isBlack = booleanArrayOf(false, true, false, true, false, false, true, false, true, false, true, false)

/** Geometry of one key on screen. */
private data class KeyRect(val note: Int, val rect: Rect, val black: Boolean)

private fun layoutKeys(firstNote: Int, lastNote: Int, size: Size): List<KeyRect> {
    val whites = (firstNote..lastNote).filter { !isBlack[it % 12] }
    val whiteWidth = size.width / whites.size
    val keys = ArrayList<KeyRect>()
    var whiteIndex = 0
    for (note in firstNote..lastNote) {
        if (!isBlack[note % 12]) {
            keys += KeyRect(note, Rect(whiteIndex * whiteWidth, 0f, (whiteIndex + 1) * whiteWidth, size.height), false)
            whiteIndex++
        } else {
            // A black key straddles the boundary after the previous white key.
            val center = whiteIndex * whiteWidth
            val w = whiteWidth * 0.58f
            keys += KeyRect(note, Rect(center - w / 2f, 0f, center + w / 2f, size.height * 0.62f), true)
        }
    }
    return keys
}

/**
 * A multi-touch piano keyboard. Each finger plays its own note, and sliding a
 * finger across keys moves its note with it (glissando).
 */
@Composable
fun PianoKeyboard(
    firstNote: Int,
    lastNote: Int,
    onNoteOn: (Int) -> Unit,
    onNoteOff: (Int) -> Unit,
    modifier: Modifier = Modifier,
) {
    val noteOn by rememberUpdatedState(onNoteOn)
    val noteOff by rememberUpdatedState(onNoteOff)
    // How many fingers are on each note (two fingers on one key = one note).
    val held = remember { mutableStateMapOf<Int, Int>() }

    fun press(note: Int) {
        val n = held[note] ?: 0
        held[note] = n + 1
        if (n == 0) noteOn(note)
    }

    fun release(note: Int) {
        val n = held[note] ?: return
        if (n <= 1) {
            held.remove(note)
            noteOff(note)
        } else {
            held[note] = n - 1
        }
    }

    DisposableEffect(firstNote, lastNote) {
        onDispose {
            held.keys.toList().forEach(noteOff)
            held.clear()
        }
    }

    Canvas(
        modifier
            .fillMaxSize()
            .semantics { contentDescription = "Keyboard" }
            .pointerInput(firstNote, lastNote) {
                val keys = layoutKeys(firstNote, lastNote, Size(size.width.toFloat(), size.height.toFloat()))
                val blacks = keys.filter { it.black }
                val whites = keys.filter { !it.black }
                fun noteAt(p: Offset): Int? =
                    (blacks.firstOrNull { it.rect.contains(p) } ?: whites.firstOrNull { it.rect.contains(p) })?.note
                val fingers = HashMap<PointerId, Int?>()
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent()
                        for (change in event.changes) {
                            if (change.pressed) {
                                val note = noteAt(change.position)
                                val previous = fingers[change.id]
                                if (!fingers.containsKey(change.id) || note != previous) {
                                    previous?.let(::release)
                                    note?.let(::press)
                                    fingers[change.id] = note
                                }
                            } else {
                                fingers.remove(change.id)?.let(::release)
                            }
                            change.consume()
                        }
                    }
                }
            }
    ) {
        val keys = layoutKeys(firstNote, lastNote, size)
        val gap = 1.dp.toPx()
        val radius = CornerRadius(6.dp.toPx())
        keys.filter { !it.black }.forEach { key ->
            drawRoundRect(
                if (key.note in held) Neon else WhiteKey,
                topLeft = Offset(key.rect.left + gap / 2f, key.rect.top),
                size = Size(key.rect.width - gap, key.rect.height),
                cornerRadius = radius,
            )
            if (key.note % 12 == 0) drawKeyLabel("C${key.note / 12 - 2}", key.rect)
        }
        keys.filter { it.black }.forEach { key ->
            drawRoundRect(
                if (key.note in held) Neon else BlackKey,
                topLeft = key.rect.topLeft,
                size = key.rect.size,
                cornerRadius = CornerRadius(3.dp.toPx()),
            )
        }
    }
}

private fun DrawScope.drawKeyLabel(text: String, rect: Rect) {
    drawIntoCanvas { canvas ->
        val paint = android.graphics.Paint().apply {
            isAntiAlias = true
            color = Color(0xFF1C1C1C).toArgb()
            textSize = (rect.width * 0.42f).coerceAtMost(22.dp.toPx())
        }
        canvas.nativeCanvas.drawText(text, rect.left + rect.width * 0.18f, rect.bottom - 10.dp.toPx(), paint)
    }
}
