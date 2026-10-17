package xyz.sufystudios.synthguide.ui.panels

import android.app.Activity
import android.content.ClipData
import android.content.Intent
import android.net.Uri
import android.widget.Toast
import androidx.activity.compose.LocalActivity
import androidx.activity.compose.rememberLauncherForActivityResult
import androidx.activity.result.contract.ActivityResultContracts
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
import androidx.compose.runtime.rememberCoroutineScope
import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.SolidColor
import androidx.compose.ui.platform.LocalFocusManager
import androidx.compose.ui.semantics.Role
import androidx.compose.ui.semantics.contentDescription
import androidx.compose.ui.semantics.semantics
import androidx.compose.ui.text.input.ImeAction
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.core.content.FileProvider
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import xyz.sufystudios.synthguide.model.Preset
import xyz.sufystudios.synthguide.model.PresetFile
import xyz.sufystudios.synthguide.model.SynthViewModel
import xyz.sufystudios.synthguide.ui.components.RetroHeader
import xyz.sufystudios.synthguide.ui.components.RetroPanelChrome
import xyz.sufystudios.synthguide.ui.theme.RetroAmber
import xyz.sufystudios.synthguide.ui.theme.RetroDim
import xyz.sufystudios.synthguide.ui.theme.RetroLed
import xyz.sufystudios.synthguide.ui.theme.retroLabel
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** "Presets" drawer, styled as a cassette library: save, update, load, rename, delete, share and back up. */
@Composable
fun PresetsPanel(vm: SynthViewModel, modifier: Modifier = Modifier) {
    val focus = LocalFocusManager.current
    var newName by rememberSaveable { mutableStateOf("") }
    var renamingId by rememberSaveable { mutableStateOf<String?>(null) }
    var renameText by rememberSaveable { mutableStateOf("") }
    val selected = vm.presets.firstOrNull { it.id == vm.selectedPresetId }
    val activity = LocalActivity.current
    val scope = rememberCoroutineScope()

    val importer = rememberLauncherForActivityResult(ActivityResultContracts.OpenDocument()) { uri ->
        if (uri == null || activity == null) return@rememberLauncherForActivityResult
        scope.launch {
            val text = withContext(Dispatchers.IO) { readSmallText(activity, uri) }
            val added = text?.let(vm::importPresets)
            val message = when (added) {
                null -> "That file isn't a Drone Synth preset"
                1 -> "Added 1 preset"
                else -> "Added $added presets"
            }
            Toast.makeText(activity, message, Toast.LENGTH_SHORT).show()
        }
    }
    val backup = rememberLauncherForActivityResult(ActivityResultContracts.CreateDocument("application/json")) { uri ->
        if (uri == null || activity == null) return@rememberLauncherForActivityResult
        val text = vm.presetFileText(vm.presets.toList())
        scope.launch {
            val saved = withContext(Dispatchers.IO) {
                runCatching { activity.contentResolver.openOutputStream(uri, "wt")!!.use { it.write(text.toByteArray()) } }.isSuccess
            }
            Toast.makeText(activity, if (saved) "Presets backed up" else "Couldn't save the backup", Toast.LENGTH_SHORT).show()
        }
    }

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
                            onShare = { activity?.let { sharePreset(it, vm, preset) } },
                            onDelete = { vm.deletePreset(preset) },
                        )
                    }
                }
            }
        }

        Row(horizontalArrangement = Arrangement.spacedBy(6.dp)) {
            // Most apps send JSON as one of these, or as a generic file.
            SmallButton("Import", Modifier.weight(1f)) {
                importer.launch(arrayOf("application/json", "text/plain", "application/octet-stream", "*/*"))
            }
            if (vm.presets.isNotEmpty()) {
                SmallButton("Back up all", Modifier.weight(1f)) {
                    backup.launch("Drone Synth presets " + SimpleDateFormat("yyyy-MM-dd", Locale.US).format(Date()) + ".json")
                }
            }
        }

    }
}

@Composable
private fun PresetRow(
    preset: Preset,
    isSelected: Boolean,
    onLoad: () -> Unit,
    onRename: () -> Unit,
    onShare: () -> Unit,
    onDelete: () -> Unit,
) {
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
        IconText("✎", "Rename", onRename)
        IconText("↗", "Share", onShare)
        IconText("🗑", "Delete", onDelete)
    }
}

@Composable
private fun IconText(symbol: String, description: String, onClick: () -> Unit) {
    Box(
        Modifier
            .semantics { contentDescription = description }
            .clickable(role = Role.Button, onClick = onClick)
            .padding(horizontal = 7.dp, vertical = 8.dp),
    ) {
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

/** Sends one preset as a small .json file through the share sheet. */
private fun sharePreset(activity: Activity, vm: SynthViewModel, preset: Preset) {
    val dir = File(activity.cacheDir, "share/presets").apply { mkdirs() }
    dir.listFiles()?.forEach { it.delete() }
    val file = File(dir, PresetFile.fileName(preset.name)).apply { writeText(vm.presetFileText(listOf(preset))) }
    val uri = FileProvider.getUriForFile(activity, "${activity.packageName}.files", file)
    val send = Intent(Intent.ACTION_SEND).apply {
        type = "application/json"
        putExtra(Intent.EXTRA_STREAM, uri)
        putExtra(Intent.EXTRA_SUBJECT, "Drone Synth preset: ${preset.name}")
        clipData = ClipData.newRawUri(preset.name, uri)
        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
    }
    activity.startActivity(Intent.createChooser(send, "Share preset"))
}

/** A picked file's text, or null if it can't be read or is too big to be a preset file. */
private fun readSmallText(activity: Activity, uri: Uri): String? = runCatching {
    activity.contentResolver.openInputStream(uri)!!.use { input ->
        val bytes = input.readNBytesCompat(MAX_PRESET_FILE_BYTES + 1)
        if (bytes.size > MAX_PRESET_FILE_BYTES) null else bytes.decodeToString()
    }
}.getOrNull()

private fun java.io.InputStream.readNBytesCompat(limit: Int): ByteArray {
    val out = java.io.ByteArrayOutputStream()
    val buffer = ByteArray(8192)
    while (out.size() < limit) {
        val n = read(buffer, 0, minOf(buffer.size, limit - out.size()))
        if (n < 0) break
        out.write(buffer, 0, n)
    }
    return out.toByteArray()
}

private const val MAX_PRESET_FILE_BYTES = 2 * 1024 * 1024
