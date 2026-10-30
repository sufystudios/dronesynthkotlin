package xyz.sufystudios.synthguide.audio

import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.ServiceInfo
import android.graphics.drawable.Icon
import android.media.MediaMetadata
import android.media.session.MediaSession
import android.media.session.PlaybackState
import android.os.Build
import android.os.Bundle
import android.os.IBinder
import androidx.core.app.ServiceCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.SharedFlow
import kotlinx.coroutines.flow.asSharedFlow
import xyz.sufystudios.synthguide.MainActivity
import xyz.sufystudios.synthguide.R

/**
 * Keeps the synth sounding while the app is in the background - a held drone, a
 * playing loop or a recording - as a media foreground service with a Stop action.
 * It runs only while something is sounding; the view model starts and stops it.
 *
 * The notification is media-style with a media session, so it shows as the media
 * player at the top of the shade and on the lock screen (a plain low-importance one
 * got filed under "Silent", where people didn't find it), and Android 13+ shows it
 * even without the notification permission. Pause, Stop, a headset's pause button
 * and swiping the notification away all stop the sound.
 */
class PlaybackService : Service() {

    private var session: MediaSession? = null

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onCreate() {
        super.onCreate()
        session = MediaSession(this, "DroneSynth").apply {
            setCallback(object : MediaSession.Callback() {
                override fun onPause() = requestStop()
                override fun onStop() = requestStop()
                override fun onCustomAction(action: String, extras: Bundle?) {
                    if (action == ACTION_STOP_SOUND) requestStop()
                }
            })
            setMetadata(
                MediaMetadata.Builder()
                    .putString(MediaMetadata.METADATA_KEY_TITLE, "Drone Synth")
                    .putString(MediaMetadata.METADATA_KEY_ARTIST, "Playing in the background")
                    .build()
            )
            setPlaybackState(
                PlaybackState.Builder()
                    .setActions(PlaybackState.ACTION_PAUSE or PlaybackState.ACTION_PLAY_PAUSE or PlaybackState.ACTION_STOP)
                    // Android 13+ builds its media controls from the session, not the notification's actions.
                    .addCustomAction(
                        PlaybackState.CustomAction.Builder(ACTION_STOP_SOUND, "Stop", R.drawable.ic_stop).build()
                    )
                    .setState(PlaybackState.STATE_PLAYING, PlaybackState.PLAYBACK_POSITION_UNKNOWN, 1f)
                    .build()
            )
            setSessionActivity(openAppIntent())
            isActive = true
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        if (intent?.action == ACTION_STOP_SOUND) {
            requestStop()
            return START_NOT_STICKY
        }
        ServiceCompat.startForeground(
            this,
            NOTIFICATION_ID,
            buildNotification(),
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK else 0,
        )
        return START_NOT_STICKY
    }

    override fun onDestroy() {
        session?.release()
        session = null
        super.onDestroy()
    }

    private fun requestStop() {
        _stopRequests.tryEmit(Unit)
        stopSelf()
    }

    private fun openAppIntent(): PendingIntent = PendingIntent.getActivity(
        this, 0,
        Intent(this, MainActivity::class.java).addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP),
        PendingIntent.FLAG_IMMUTABLE,
    )

    private fun buildNotification(): Notification {
        val builder = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            getSystemService(NotificationManager::class.java).createNotificationChannel(
                NotificationChannel(CHANNEL_ID, "Playback", NotificationManager.IMPORTANCE_LOW).apply {
                    description = "Shown while Drone Synth keeps playing in the background"
                }
            )
            Notification.Builder(this, CHANNEL_ID)
        } else {
            @Suppress("DEPRECATION")
            Notification.Builder(this)
        }
        val stop = PendingIntent.getService(
            this, 1,
            Intent(this, PlaybackService::class.java).setAction(ACTION_STOP_SOUND),
            PendingIntent.FLAG_IMMUTABLE,
        )
        return builder
            .setSmallIcon(R.drawable.ic_stat_drone)
            .setContentTitle("Drone Synth is playing")
            .setContentText("Tap to return, or stop the sound")
            .setContentIntent(openAppIntent())
            .setDeleteIntent(stop)
            .addAction(Notification.Action.Builder(Icon.createWithResource(this, R.drawable.ic_stop), "Stop", stop).build())
            .setStyle(Notification.MediaStyle().setMediaSession(session?.sessionToken).setShowActionsInCompactView(0))
            .setOngoing(true)
            .setCategory(Notification.CATEGORY_TRANSPORT)
            .setVisibility(Notification.VISIBILITY_PUBLIC)
            .build()
    }

    companion object {
        private const val CHANNEL_ID = "playback"
        private const val NOTIFICATION_ID = 1
        private const val ACTION_STOP_SOUND = "xyz.sufystudios.synthguide.STOP_SOUND"

        private val _stopRequests = MutableSharedFlow<Unit>(extraBufferCapacity = 1)

        /** Emits when the notification or media controls ask to stop the sound. */
        val stopRequests: SharedFlow<Unit> = _stopRequests.asSharedFlow()

        fun start(context: Context) =
            ContextCompat.startForegroundService(context, Intent(context, PlaybackService::class.java))

        fun stop(context: Context) {
            context.stopService(Intent(context, PlaybackService::class.java))
        }
    }
}
