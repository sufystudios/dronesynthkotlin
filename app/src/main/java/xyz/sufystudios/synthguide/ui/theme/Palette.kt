package xyz.sufystudios.synthguide.ui.theme

import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontFamily
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.TextUnit
import androidx.compose.ui.unit.sp

/** The neon green used for knobs, the glow and active states (matches iOS's system green). */
val Neon = Color(0xFF30D158)

/** Retro tape-deck palette shared by the drawers. */
val RetroAmber = Color(0xFFE6BF66)
val RetroLed = Color(0xFFE66633)
val RetroPanel = Color(0xFF261A12)
val RetroDim = Color(0xFF808080)

/** The small bold monospaced caption used across the drawers. */
fun retroLabel(size: TextUnit = 8.sp, color: Color = RetroAmber, weight: FontWeight = FontWeight.Bold) =
    TextStyle(fontFamily = FontFamily.Monospace, fontWeight = weight, fontSize = size, color = color)
