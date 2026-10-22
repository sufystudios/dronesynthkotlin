package xyz.sufystudios.synthguide.model

import org.json.JSONArray
import org.json.JSONObject

/** A preset read from a shared file: no id yet, it gets one when it joins the library. */
data class ImportedPreset(val name: String, val values: Map<SynthParam, Float>, val sequencer: SequencerPreset)

/**
 * The file format for sharing and backing up presets: one JSON object naming the
 * app and format version, with a list of named presets. Values are keyed by
 * [SynthParam.key], so files keep working as parameters are added.
 */
object PresetFile {
    private const val APP = "Drone Synth"
    private const val FORMAT = 1

    fun encode(presets: List<Preset>): String {
        val list = JSONArray()
        presets.forEach { p ->
            list.put(JSONObject().put("name", p.name).put("values", encodeValues(p.values)).put("sequencer", p.sequencer.toJson()))
        }
        return JSONObject().put("app", APP).put("format", FORMAT).put("presets", list).toString(2)
    }

    /**
     * The presets in [text], or null if it isn't a preset file. Also reads a bare
     * list of presets (the library's own storage), so a copied presets.json imports too.
     */
    fun decode(text: String): List<ImportedPreset>? {
        val list = runCatching {
            val trimmed = text.trimStart()
            if (trimmed.startsWith("[")) JSONArray(trimmed) else JSONObject(trimmed).getJSONArray("presets")
        }.getOrNull() ?: return null
        // A file from a newer version still imports whatever this version understands.
        val presets = (0 until list.length()).mapNotNull { i ->
            runCatching {
                val o = list.getJSONObject(i)
                ImportedPreset(
                    o.getString("name").trim().take(MAX_NAME).ifEmpty { "Imported" },
                    decodeValues(o.getJSONObject("values")),
                    SequencerPreset.fromJson(o.optJSONObject("sequencer")),
                )
            }.getOrNull()
        }
        return presets.takeIf { it.isNotEmpty() }
    }

    /** A file name for [name] that's safe on every file system. */
    fun fileName(name: String): String =
        "Drone Synth - " + name.replace(Regex("[^A-Za-z0-9 ._-]"), "_").trim().take(MAX_NAME).ifEmpty { "Preset" } + ".json"

    internal fun encodeValues(values: Map<SynthParam, Float>): JSONObject {
        val o = JSONObject()
        values.forEach { (param, value) -> o.put(param.key, value.toDouble()) }
        return o
    }

    /** Unknown keys are ignored and missing ones take their defaults. */
    internal fun decodeValues(o: JSONObject): Map<SynthParam, Float> {
        val values = SynthParam.entries.associateWith { it.default }.toMutableMap()
        o.keys().forEach { key ->
            val param = SynthParam.fromKey(key) ?: return@forEach
            val v = o.optDouble(key)
            if (!v.isNaN()) values[param] = param.clamp(v.toFloat())
        }
        return values
    }

    private const val MAX_NAME = 60
}
