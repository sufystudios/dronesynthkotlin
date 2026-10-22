package xyz.sufystudios.synthguide.model

import android.content.Context
import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.util.UUID

/**
 * A saved patch: a name plus every parameter's value, keyed by [SynthParam.key],
 * and the ARP/SEQ module (a preset saved before it had one loads it fresh).
 */
data class Preset(
    val id: String,
    val name: String,
    val values: Map<SynthParam, Float>,
    val sequencer: SequencerPreset = SequencerPreset(),
)

/**
 * Persists the live patch (restored on next launch) and the preset library, as
 * small JSON files in app-private storage. Unknown keys are ignored and missing
 * ones fall back to defaults, so presets survive parameters being added later.
 */
class PatchStore(context: Context) {
    private val currentFile = File(context.filesDir, "current_patch.json")
    private val presetsFile = File(context.filesDir, "presets.json")
    private val currentSequencerFile = File(context.filesDir, "current_sequencer.json")

    fun loadCurrent(): Map<SynthParam, Float>? =
        runCatching { PresetFile.decodeValues(JSONObject(currentFile.readText())) }.getOrNull()

    fun saveCurrent(values: Map<SynthParam, Float>) {
        writeAtomically(currentFile, PresetFile.encodeValues(values).toString())
    }

    fun loadCurrentSequencer(): SequencerPreset? =
        runCatching { SequencerPreset.fromJson(JSONObject(currentSequencerFile.readText())) }.getOrNull()

    fun saveCurrentSequencer(sequencer: SequencerPreset) {
        writeAtomically(currentSequencerFile, sequencer.toJson().toString())
    }

    fun loadPresets(): List<Preset> {
        val array = runCatching { JSONArray(presetsFile.readText()) }.getOrNull() ?: return emptyList()
        // One bad entry only drops itself, not the whole library.
        return (0 until array.length()).mapNotNull { i ->
            runCatching {
                val o = array.getJSONObject(i)
                Preset(
                    o.getString("id"), o.getString("name"), PresetFile.decodeValues(o.getJSONObject("values")),
                    SequencerPreset.fromJson(o.optJSONObject("sequencer")),
                )
            }.getOrNull()
        }
    }

    fun savePresets(presets: List<Preset>) {
        val array = JSONArray()
        presets.forEach { p ->
            array.put(
                JSONObject().put("id", p.id).put("name", p.name).put("values", PresetFile.encodeValues(p.values))
                    .put("sequencer", p.sequencer.toJson())
            )
        }
        writeAtomically(presetsFile, array.toString())
    }

    fun newPresetId(): String = UUID.randomUUID().toString()

    private fun writeAtomically(file: File, text: String) {
        val tmp = File(file.parentFile, file.name + ".tmp")
        tmp.writeText(text)
        tmp.renameTo(file)
    }
}
