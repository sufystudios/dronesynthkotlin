package xyz.sufystudios.synthguide.ui

import androidx.activity.compose.BackHandler
import androidx.activity.compose.LocalActivity
import androidx.compose.animation.AnimatedVisibility
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.animation.slideInHorizontally
import androidx.compose.animation.slideOutHorizontally
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.BoxWithConstraints
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxHeight
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.heightIn
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.CircleShape
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.foundation.verticalScroll
import androidx.compose.runtime.Composable
import androidx.compose.runtime.CompositionLocalProvider
import androidx.compose.runtime.staticCompositionLocalOf
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.platform.LocalDensity
import androidx.compose.ui.unit.Density
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.semantics.stateDescription
import androidx.compose.ui.text.TextStyle
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.Dp
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import xyz.sufystudios.synthguide.R
import xyz.sufystudios.synthguide.billing.Product
import xyz.sufystudios.synthguide.model.SynthParam
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.DrawerTab
import xyz.sufystudios.synthguide.ui.components.ParamKnob
import xyz.sufystudios.synthguide.ui.components.PianoKeyboard
import xyz.sufystudios.synthguide.ui.components.RailIcon
import xyz.sufystudios.synthguide.ui.components.mainLabel
import xyz.sufystudios.synthguide.ui.panels.ArpSeqPanel
import xyz.sufystudios.synthguide.ui.panels.DelayPanel
import xyz.sufystudios.synthguide.ui.panels.EnvelopePanel
import xyz.sufystudios.synthguide.ui.panels.LfoPanel
import xyz.sufystudios.synthguide.ui.panels.LooperPanel
import xyz.sufystudios.synthguide.ui.panels.RecorderPanel
import xyz.sufystudios.synthguide.ui.panels.formatTime
import xyz.sufystudios.synthguide.ui.panels.OscillatorPanel
import xyz.sufystudios.synthguide.ui.panels.PresetsPanel
import xyz.sufystudios.synthguide.ui.panels.ReverbPanel
import xyz.sufystudios.synthguide.ui.panels.SequencerClockLed
import xyz.sufystudios.synthguide.ui.panels.TuningPanel
import xyz.sufystudios.synthguide.ui.theme.Neon
import xyz.sufystudios.synthguide.ui.theme.RetroLed

private val DrawerWidth = 230.dp
private val KeyboardHeight = 250.dp

/** How many octaves the keyboard shows: two on phones, more on tablets' wider screens. */
private val LocalKeyboardOctaves = staticCompositionLocalOf { 2 }

/** The keyboard's first C, kept low enough that its top key stays at or below C8. */
private val SynthViewModel.firstKeyboardOctave: Int
    @Composable get() = keyboardOctave.coerceAtMost(maxKeyboardOctave)

private val maxKeyboardOctave: Int
    @Composable get() = 7 - LocalKeyboardOctaves.current

@Composable
fun DroneSynthScreen(vm: SynthViewModel) {
    val owned by vm.owned.collectAsStateWithLifecycle()
    var openTab by rememberSaveable { mutableStateOf<DrawerTab?>(null) }
    val toggleTab: (DrawerTab) -> Unit = { tab -> openTab = if (openTab == tab) null else tab }
    // Back closes an open drawer before it leaves the app.
    BackHandler(enabled = openTab != null) { openTab = null }

    BoxWithConstraints(Modifier.fillMaxSize().background(Color.Black)) {
        Backdrop(onTap = { openTab = null })
        val landscape = maxWidth > maxHeight
        // Tablets get the phone layout scaled up, so knobs, drawers and text keep
        // their proportions to the screen instead of floating small in the middle.
        val tablet = minOf(maxWidth, maxHeight) >= 600.dp
        val scale = if (!tablet) 1f else if (landscape) {
            minOf(maxWidth / 880.dp, maxHeight / 400.dp).coerceIn(1f, 1.6f)
        } else {
            minOf(maxWidth / 411.dp, maxHeight / 860.dp).coerceIn(1f, 1.6f)
        }
        // As many octaves as fit with white keys at least 34dp wide (plus the top C).
        val octaves = if (!tablet) 2 else ((maxWidth / 34.dp - 1f) / 7f).toInt().coerceIn(2, 4)
        val density = LocalDensity.current
        CompositionLocalProvider(
            LocalDensity provides Density(density.density * scale, density.fontScale),
            LocalKeyboardOctaves provides octaves,
        ) {
            if (landscape) {
                LandscapeLayout(vm, openTab, toggleTab)
            } else {
                PortraitLayout(vm, openTab, toggleTab)
            }
        }
    }
}

/** The green radial glow from the iOS app. Tapping it closes drawers. */
@Composable
private fun Backdrop(onTap: () -> Unit) {
    val tap = Modifier.fillMaxSize().pointerInput(Unit) { detectTapGestures { onTap() } }
    val radius = with(LocalDensity.current) { 650.dp.toPx() }
    Box(tap.background(Brush.radialGradient(listOf(Neon.copy(alpha = 0.5f), Color.Black), radius = radius)))
}

// MARK: - Portrait

@Composable
private fun PortraitLayout(vm: SynthViewModel, openTab: DrawerTab?, toggleTab: (DrawerTab) -> Unit) {
    BoxWithConstraints(Modifier.fillMaxSize().windowInsetsPadding(WindowInsets.safeDrawing)) {
        // Drawers open upward from just above the keyboard and scroll if they're taller than the room above it.
        val drawerMaxHeight = maxHeight - KeyboardHeight - 16.dp
        Column(Modifier.fillMaxSize(), horizontalAlignment = Alignment.CenterHorizontally) {
            PillRow(vm, Modifier.padding(top = 6.dp, bottom = 4.dp))
            CutoffKnob(vm, 150.dp)
            ResonanceRow(vm, 60.dp, Modifier.padding(top = 4.dp))
            Spacer(Modifier.weight(1f))
            Row(Modifier.fillMaxWidth().padding(horizontal = 50.dp, vertical = 6.dp), verticalAlignment = Alignment.CenterVertically) {
                OctaveButtons(vm, "◂ OCT", "OCT ▸") { lower, upper ->
                    lower()
                    Spacer(Modifier.weight(1f))
                    DronePill(vm)
                    Spacer(Modifier.weight(1f))
                    upper()
                }
            }
            Row(horizontalArrangement = Arrangement.spacedBy(10.dp), modifier = Modifier.padding(bottom = 6.dp)) {
                ParamKnob(vm, SynthParam.Attack, 50.dp, "Attack", labelStyle = mainLabel)
                ParamKnob(vm, SynthParam.Decay, 50.dp, "Decay", labelStyle = mainLabel)
                ParamKnob(vm, SynthParam.Sustain, 50.dp, "Sustain", labelStyle = mainLabel)
                ParamKnob(vm, SynthParam.Release, 50.dp, "Release", labelStyle = mainLabel)
            }
            KeyboardPanel(vm, Modifier.fillMaxWidth().height(KeyboardHeight))
        }

        DrawerSide(vm, left = false, openTab, toggleTab, drawerMaxHeight,
            Modifier.align(Alignment.BottomEnd).padding(bottom = KeyboardHeight + 10.dp))
        DrawerSide(vm, left = true, openTab, toggleTab, drawerMaxHeight,
            Modifier.align(Alignment.BottomStart).padding(bottom = KeyboardHeight + 10.dp))
    }
}

/** One edge's tab rail plus its sliding drawer. */
@Composable
private fun DrawerSide(
    vm: SynthViewModel,
    left: Boolean,
    openTab: DrawerTab?,
    toggleTab: (DrawerTab) -> Unit,
    maxHeight: Dp,
    modifier: Modifier,
) {
    val tabs = DrawerTab.entries.filter { it.leftSide == left }
    val showing = openTab?.takeIf { it.leftSide == left }
    Row(modifier.clip(RoundedCornerShape(10.dp)), verticalAlignment = Alignment.Bottom) {
        if (!left) DrawerSlot(vm, showing, left, maxHeight)
        Column {
            // Five tabs a rail, in the height four used to take, so the rails stay
            // level and clear the keyboard on short phones.
            val tabHeight = 320.dp / tabs.size
            tabs.forEach { tab ->
                RailButton(vm, tab, selected = tab == openTab, Modifier.size(38.dp, tabHeight)) { toggleTab(tab) }
            }
        }
        if (left) DrawerSlot(vm, showing, left, maxHeight)
    }
}

@Composable
private fun DrawerSlot(vm: SynthViewModel, tab: DrawerTab?, left: Boolean, maxHeight: Dp) {
    AnimatedVisibility(
        visible = tab != null,
        enter = slideInHorizontally { if (left) -it else it } + fadeIn(),
        exit = slideOutHorizontally { if (left) -it else it } + fadeOut(),
    ) {
        // Remember the last tab while sliding out, so the content doesn't vanish mid-animation.
        var shown by rememberSaveable { mutableStateOf(tab ?: DrawerTab.Adsr) }
        if (tab != null) shown = tab
        Box(Modifier.width(DrawerWidth).heightIn(max = maxHeight).verticalScroll(rememberScrollState())) {
            DrawerContent(vm, shown)
        }
    }
}

@Composable
private fun DrawerContent(vm: SynthViewModel, tab: DrawerTab) {
    val m = Modifier.fillMaxWidth()
    when (tab) {
        DrawerTab.Osc -> OscillatorPanel(vm, m)
        DrawerTab.Lfo -> LfoPanel(vm, m)
        DrawerTab.Adsr -> EnvelopePanel(vm, m)
        DrawerTab.Delay -> DelayPanel(vm, m)
        DrawerTab.Presets -> PresetsPanel(vm, m)
        DrawerTab.Reverb -> ReverbPanel(vm, m)
        DrawerTab.Rec -> RecorderPanel(vm, m)
        DrawerTab.Tune -> TuningPanel(vm, m)
        DrawerTab.Looper -> LooperPanel(vm, m)
        DrawerTab.Arp -> ArpSeqPanel(vm, m)
    }
}

@Composable
private fun RailButton(vm: SynthViewModel, tab: DrawerTab, selected: Boolean, modifier: Modifier, onClick: () -> Unit) {
    val color = if (selected) Neon else Color.White
    val looper = vm.looper
    val loopProgress = if (tab == DrawerTab.Looper && looper.hasLoop && looper.lengthSeconds > 0f) {
        looper.positionSeconds / looper.lengthSeconds
    } else null
    Column(
        modifier
            .background(Color.Black.copy(alpha = 0.6f))
            .semantics { contentDescription = tab.label; stateDescription = if (selected) "Open" else "Closed" }
            .clickable(role = Role.Tab, onClick = onClick),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(4.dp, Alignment.CenterVertically),
    ) {
        RailIcon(tab, color, loopProgress = loopProgress)
        val label = @Composable {
            BasicText(tab.label, style = TextStyle(color = color, fontSize = 10.sp, fontWeight = FontWeight.SemiBold), maxLines = 1)
        }
        if (tab == DrawerTab.Arp) {
            // The RATE LED beside the label keeps the clock showing while the drawer is closed.
            Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(2.dp)) {
                SequencerClockLed(vm.sequencerState.clockTicks)
                label()
            }
        } else {
            label()
        }
        // The loop's length stays visible while its drawer is closed, as on iOS.
        if (loopProgress != null) {
            BasicText(formatTime(looper.lengthSeconds), style = TextStyle(color = RetroLed, fontSize = 8.sp, fontWeight = FontWeight.SemiBold))
        }
    }
}

// MARK: - Landscape

@Composable
private fun LandscapeLayout(vm: SynthViewModel, openTab: DrawerTab?, toggleTab: (DrawerTab) -> Unit) {
    BoxWithConstraints(Modifier.fillMaxSize().windowInsetsPadding(WindowInsets.safeDrawing).padding(10.dp)) {
        // Phones are short in landscape, so the top row keeps a fixed height and drawers
        // scroll within it rather than growing over the keyboard (as on iPad). Taller
        // screens (tablets, foldables) give their extra height to the drawers.
        val topHeight = (maxHeight * 0.5f).coerceIn(220.dp, 400.dp)
        Column(Modifier.fillMaxSize()) {
            Row(Modifier.fillMaxWidth().height(topHeight)) {
                LandscapeRail(vm, left = true, openTab, toggleTab)
                LandscapeDrawer(vm, openTab?.takeIf { it.leftSide }, left = true)
                Spacer(Modifier.weight(1f))
                LandscapeFilterPanel(vm, Modifier.width(250.dp).fillMaxHeight())
                Spacer(Modifier.weight(1f))
                LandscapeDrawer(vm, openTab?.takeIf { !it.leftSide }, left = false)
                LandscapeRail(vm, left = false, openTab, toggleTab)
            }
            Spacer(Modifier.height(10.dp))
            KeyboardPanel(vm, Modifier.fillMaxWidth().weight(1f))
        }
    }
}

@Composable
private fun LandscapeRail(vm: SynthViewModel, left: Boolean, openTab: DrawerTab?, toggleTab: (DrawerTab) -> Unit) {
    Column(Modifier.width(64.dp).fillMaxHeight().clip(RoundedCornerShape(10.dp))) {
        DrawerTab.entries.filter { it.leftSide == left }.forEach { tab ->
            RailButton(vm, tab, tab == openTab, Modifier.fillMaxWidth().weight(1f)) { toggleTab(tab) }
        }
    }
}

@Composable
private fun LandscapeDrawer(vm: SynthViewModel, tab: DrawerTab?, left: Boolean) {
    AnimatedVisibility(
        visible = tab != null,
        enter = slideInHorizontally { if (left) -it / 2 else it / 2 } + fadeIn(),
        exit = slideOutHorizontally { if (left) -it / 2 else it / 2 } + fadeOut(),
    ) {
        var shown by rememberSaveable { mutableStateOf(tab ?: DrawerTab.Adsr) }
        if (tab != null) shown = tab
        Box(
            Modifier
                .padding(horizontal = 8.dp)
                .width(DrawerWidth)
                .fillMaxHeight()
                .clip(RoundedCornerShape(10.dp))
                .verticalScroll(rememberScrollState())
        ) {
            DrawerContent(vm, shown)
        }
    }
}

@Composable
private fun LandscapeFilterPanel(vm: SynthViewModel, modifier: Modifier) {
    Column(
        modifier
            .clip(RoundedCornerShape(10.dp))
            .background(Color.Black.copy(alpha = 0.6f))
            .padding(10.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.spacedBy(8.dp),
    ) {
        PillRow(vm, compact = true)
        // Side by side rather than stacked, to fit a phone's short landscape height.
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(10.dp)) {
            CutoffKnob(vm, 90.dp)
            ResonanceRow(vm, 40.dp, spacing = 8.dp)
        }
        Spacer(Modifier.weight(1f))
        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            OctaveButtons(vm, "◂", "▸") { lower, upper ->
                lower()
                DronePill(vm)
                upper()
            }
        }
    }
}

// MARK: - Shared pieces

@Composable
private fun CutoffKnob(vm: SynthViewModel, size: Dp) {
    ParamKnob(vm, SynthParam.Cutoff, size, resettable = false)
}

/** One resonance knob in mono; separate L and R knobs once the stereo filters are on. */
@Composable
private fun ResonanceRow(vm: SynthViewModel, size: Dp, modifier: Modifier = Modifier, spacing: Dp = 16.dp) {
    val owned by vm.owned.collectAsStateWithLifecycle()
    val stereo = vm.isOn(SynthParam.StereoFilters) && Product.StereoFilters in owned
    Row(modifier, horizontalArrangement = Arrangement.spacedBy(spacing), verticalAlignment = Alignment.Top) {
        if (stereo) {
            ParamKnob(vm, SynthParam.LeftResonance, size, "L", labelStyle = mainLabel)
            ParamKnob(vm, SynthParam.RightResonance, size, "R", labelStyle = mainLabel)
        } else {
            ParamKnob(vm, SynthParam.LeftResonance, size, "Resonance", labelStyle = mainLabel)
        }
    }
}

/** The Mono/Stereo pill: the stereo filters are a purchase, offered when the pill is locked. */
@Composable
private fun PillRow(vm: SynthViewModel, modifier: Modifier = Modifier, compact: Boolean = false) {
    val activity = LocalActivity.current
    val owned by vm.owned.collectAsStateWithLifecycle()
    Row(modifier, horizontalArrangement = Arrangement.spacedBy(8.dp)) {
        val stereoOwned = Product.StereoFilters in owned
        val stereoOn = stereoOwned && vm.isOn(SynthParam.StereoFilters)
        Pill(
            label = (if (stereoOwned) "" else "🔒 ") + if (stereoOn) "STEREO" else "MONO",
            active = stereoOn,
            colors = listOf(Neon, Color(0xFF14B8A6)),
            compact = compact,
            description = "Stereo filters",
        ) { activity?.let { vm.toggleGated(it, SynthParam.StereoFilters) } }
    }
}

@Composable
private fun Pill(label: String, active: Boolean, colors: List<Color>, compact: Boolean, description: String, onClick: () -> Unit) {
    val background = if (active) Brush.horizontalGradient(colors) else Brush.horizontalGradient(colors.map { it.copy(alpha = 0.35f) })
    Box(
        Modifier
            .clip(CircleShape)
            .background(background)
            .semantics { contentDescription = description; stateDescription = if (active) "On" else "Off" }
            .clickable(role = Role.Switch, onClick = onClick)
            .padding(horizontal = if (compact) 10.dp else 16.dp, vertical = 7.dp),
    ) {
        BasicText(label, style = TextStyle(color = Color.White, fontSize = if (compact) 10.sp else 12.sp, fontWeight = FontWeight.Bold))
    }
}

@Composable
private fun DronePill(vm: SynthViewModel) {
    val on = vm.isOn(SynthParam.Drone)
    Row(
        Modifier
            .clip(CircleShape)
            .background(Color.Black.copy(alpha = 0.4f))
            .semantics { stateDescription = if (on) "On" else "Off" }
            .clickable(role = Role.Switch) { vm.toggle(SynthParam.Drone) }
            .padding(horizontal = 14.dp, vertical = 7.dp),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(6.dp),
    ) {
        val color = if (on) Neon else Color.White
        BasicText(if (on) "●" else "○", style = TextStyle(color = color, fontSize = 12.sp))
        BasicText("Drone", style = TextStyle(color = color, fontSize = 13.sp))
    }
}

/** The octave down/up buttons, handed to [content] to place either side of something. */
@Composable
private fun OctaveButtons(
    vm: SynthViewModel,
    downLabel: String,
    upLabel: String,
    content: @Composable (lower: @Composable () -> Unit, upper: @Composable () -> Unit) -> Unit,
) {
    val first = vm.firstKeyboardOctave
    val max = maxKeyboardOctave
    content(
        { OctaveButton(downLabel, enabled = first > 0) { vm.shiftKeyboard(-1, max) } },
        { OctaveButton(upLabel, enabled = first < max) { vm.shiftKeyboard(1, max) } },
    )
}

@Composable
private fun OctaveButton(label: String, enabled: Boolean, onClick: () -> Unit) {
    Box(
        Modifier
            .clip(CircleShape)
            .background(Color.Black.copy(alpha = 0.4f))
            .clickable(enabled = enabled, role = Role.Button, onClick = onClick)
            .padding(horizontal = 10.dp, vertical = 7.dp),
    ) {
        BasicText(label, style = TextStyle(color = Color.White.copy(alpha = if (enabled) 1f else 0.3f), fontSize = 11.sp, fontWeight = FontWeight.SemiBold))
    }
}

@Composable
private fun KeyboardPanel(vm: SynthViewModel, modifier: Modifier) {
    val first = 24 + 12 * vm.firstKeyboardOctave
    Box(
        modifier
            .clip(RoundedCornerShape(12.dp))
            .background(Color.Black.copy(alpha = 0.25f))
            .padding(10.dp)
    ) {
        PianoKeyboard(first, first + 12 * LocalKeyboardOctaves.current, vm::noteOn, vm::noteOff, Modifier.clip(RoundedCornerShape(6.dp)))
    }
}
