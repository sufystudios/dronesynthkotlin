package xyz.sufystudios.synthguide.model

import android.app.Activity
import android.content.Context
import android.content.Intent
import android.media.MediaPlayer
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import androidx.core.content.FileProvider
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import xyz.sufystudios.synthguide.audio.AacExporter
import xyz.sufystudios.synthguide.audio.NativeSynth
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/** What the REC drawer shows. */
data class RecorderUiState(
    val recording: Boolean = false,
    val hasTake: Boolean = false,
    val playing: Boolean = false,
    val elapsedSeconds: Float = 0f,
    val preparingShare: Boolean = false,
)

/**
 * The REC drawer's single take: the native recorder writes it as a WAV file, it
 * plays back through MediaPlayer, and Share exports a compact .m4a copy.
 */
class TakeRecorder(private val context: Context, private val scope: CoroutineScope) {
    private val takeFile = File(File(context.filesDir, "recordings").apply { mkdirs() }, "take.wav")
    private var player: MediaPlayer? = null
    private var finalizing = false

    var state by mutableStateOf(RecorderUiState(hasTake = takeFile.length() > WAV_HEADER_BYTES))
        private set

    /** Called a few times a second to follow the native recorder. */
    fun poll() {
        val (recording, open, seconds) = NativeSynth.recorderState()
        // The take stops itself at 3:00; finalise the file once it has.
        if (open && !recording && !finalizing) finishRecording()
        if (recording) state = state.copy(recording = true, elapsedSeconds = seconds)
    }

    fun toggleRecord() {
        if (state.recording) {
            finishRecording()
            return
        }
        stopPlayback()
        if (NativeSynth.recorderStart(takeFile.path)) state = state.copy(recording = true, elapsedSeconds = 0f)
    }

    fun togglePlay() {
        if (state.playing) {
            stopPlayback()
            return
        }
        if (!state.hasTake || state.recording) return
        player = MediaPlayer().apply {
            setDataSource(takeFile.path)
            setOnCompletionListener { stopPlayback() }
            prepare()
            start()
        }
        state = state.copy(playing = true)
    }

    /** STOP: ends whatever the recorder is doing. */
    fun stopTransport() {
        if (state.recording) finishRecording()
        stopPlayback()
    }

    fun share(activity: Activity) {
        if (!state.hasTake || state.preparingShare) return
        stopTransport()
        state = state.copy(preparingShare = true)
        scope.launch {
            val file = withContext(Dispatchers.IO) {
                runCatching {
                    val dir = File(context.cacheDir, "share").apply { mkdirs() }
                    dir.listFiles()?.forEach { it.delete() }
                    val name = "Drone Synth " + SimpleDateFormat("yyyy-MM-dd HHmm", Locale.US).format(Date()) + ".m4a"
                    File(dir, name).also { AacExporter.export(takeFile, it) }
                }.getOrNull()
            }
            state = state.copy(preparingShare = false)
            if (file == null) return@launch
            val uri = FileProvider.getUriForFile(activity, "${activity.packageName}.files", file)
            val send = Intent(Intent.ACTION_SEND).apply {
                type = "audio/mp4"
                putExtra(Intent.EXTRA_STREAM, uri)
                addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
            }
            activity.startActivity(Intent.createChooser(send, "Share recording"))
        }
    }

    fun release() = stopPlayback()

    private fun finishRecording() {
        finalizing = true
        state = state.copy(recording = false)
        scope.launch {
            withContext(Dispatchers.IO) { NativeSynth.recorderStop() }
            finalizing = false
            state = state.copy(hasTake = takeFile.length() > WAV_HEADER_BYTES, elapsedSeconds = 0f)
        }
    }

    private fun stopPlayback() {
        player?.release()
        player = null
        if (state.playing) state = state.copy(playing = false)
    }

    companion object {
        const val MAX_SECONDS = 180f
        private const val WAV_HEADER_BYTES = 44L
    }
}
