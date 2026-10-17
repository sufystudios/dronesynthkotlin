package xyz.sufystudios.synthguide.ui.panels

import androidx.activity.compose.LocalActivity
import androidx.compose.foundation.background
import androidx.compose.foundation.border
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.foundation.text.BasicText
import androidx.compose.foundation.text.BasicTextField
import androidx.compose.foundation.text.KeyboardActions
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import xyz.sufystudios.synthguide.model.Preset
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.components.RetroSegmentRow
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroDim
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel

/** "Presets" drawer, styled as a cassette library: save, update, load, rename, delete. */
@Composable
fun PresetsPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val focus = LocalFocusManager.current
    var newName by rememberSaveable { mutableStateOf("") }
    var renamingId by rememberSaveable { mutableStateOf<String?>(null) }
    var renameText by rememberSaveable { mutableStateOf("") }
    val selected = vm.presets.firstOrNull { it.id == vm.selectedPresetId }

    RetroPanelChrome(modifier) {
        RetroHeader("CASSETTE LIBRARY", reels = true)

        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            RetroTextField(newName, { newName = it }, "Insert new tape...", Modifier.weight(1f)) {
                vm.savePreset(newName); newName = ""; focus.clearFocus()
            }
            SmallButton("REC") { vm.savePreset(newName); newName = ""; focus.clearFocus() }
        }
        if (selected != null) {
            SmallButton("Update \"${selected.name}\"", Modifier.fillMaxWidth()) { vm.updatePreset(selected) }
        }

        if (vm.presets.isEmpty()) {
            BasicText("No tapes recorded yet", style = retroLabel(9.sp, RetroDim))
        } else {
            Column(verticalArrangement = Arrangement.spacedBy(4.dp)) {
                vm.presets.forEach { preset ->
                    if (renamingId == preset.id) {
                        Row(verticalAlignment = Alignment.CenterVertically, horizontalArrangement = Arrangement.spacedBy(4.dp)) {
                            RetroTextField(renameText, { renameText = it }, "Name", Modifier.weight(1f)) {
                                vm.renamePreset(preset, renameText); renamingId = null; focus.clearFocus()
                            }
                            SmallButton("OK") { vm.renamePreset(preset, renameText); renamingId = null; focus.clearFocus() }
                            SmallButton("✕") { renamingId = null }
                        }
                    } else {
                        PresetRow(
                            preset,
                            isSelected = preset.id == vm.selectedPresetId,
                            onLoad = { vm.loadPreset(preset) },
                            onRename = { renamingId = preset.id; renameText = preset.name },
                            onDelete = { vm.deletePreset(preset) },
                        )
                    }
                }
            }
        }

        Divider()
        val activity = LocalActivity.current
        BasicText("BACKGROUND", style = retroLabel())
        RetroSegmentRow(listOf("Glow", "Space"), if (vm.spaceSkin) 1 else 0) { index ->
            if ((index == 1) != vm.spaceSkin && activity != null) vm.toggleSpaceSkin(activity)
        }
    }
}

@Composable
private fun PresetRow(preset: Preset, isSelected: Boolean, onLoad: () -> Unit, onRename: () -> Unit, onDelete: () -> Unit) {
    Row(
        Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(4.dp))
            .background(Color.Black.copy(alpha = if (isSelected) 0.6f else 0.35f))
            .border(1.dp, if (isSelected) RetroLed.copy(alpha = 0.6f) else Color.Transparent, RoundedCornerShape(4.dp))
            .clickable(role = Role.Button, onClick = onLoad)
            .padding(start = 8.dp),
        verticalAlignment = Alignment.CenterVertically,
    ) {
        BasicText(
            preset.name.uppercase(),
            style = retroLabel(10.sp, if (isSelected) RetroLed else Color.White),
            maxLines = 1,
            modifier = Modifier.weight(1f).padding(vertical = 8.dp),
        )
        IconText("✎", onRename)
        IconText("🗑", onDelete)
    }
}

@Composable
private fun IconText(symbol: String, onClick: () -> Unit) {
    Box(Modifier.clickable(role = Role.Button, onClick = onClick).padding(horizontal = 8.dp, vertical = 8.dp)) {
        BasicText(symbol, style = retroLabel(12.sp, RetroAmber))
    }
}

@Composable
private fun SmallButton(label: String, modifier: Modifier = Modifier, onClick: () -> Unit) {
    Box(
        modifier
            .clip(RoundedCornerShape(6.dp))
            .background(Color.Black.copy(alpha = 0.4f))
            .clickable(role = Role.Button, onClick = onClick)
            .padding(horizontal = 10.dp, vertical = 8.dp),
        contentAlignment = Alignment.Center,
    ) {
        BasicText(label, style = retroLabel(10.sp, Color.White), maxLines = 1)
    }
}

@Composable
private fun RetroTextField(value: String, onChange: (String) -> Unit, placeholder: String, modifier: Modifier, onDone: () -> Unit) {
    BasicTextField(
        value = value,
        onValueChange = onChange,
        singleLine = true,
        textStyle = retroLabel(11.sp, Color.White, androidx.compose.ui.text.font.FontWeight.Normal),
        cursorBrush = SolidColor(RetroLed),
        keyboardOptions = KeyboardOptions(imeAction = ImeAction.Done),
        keyboardActions = KeyboardActions(onDone = { onDone() }),
        modifier = modifier
            .clip(RoundedCornerShape(4.dp))
            .background(Color.Black.copy(alpha = 0.5f))
            .padding(horizontal = 8.dp, vertical = 8.dp),
        decorationBox = { inner ->
            Box {
                if (value.isEmpty()) BasicText(placeholder, style = retroLabel(11.sp, RetroDim, androidx.compose.ui.text.font.FontWeight.Normal))
                inner()
            }
        },
    )
}
