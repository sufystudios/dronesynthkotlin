package xyz.sufystudios.synthguide.model

import android.content.Context
import androidx.core.content.edit
import kotlin.random.Random
import xyz.sufystudios.synthguide.BuildConfig

/**
 * Keeps how much Drone Synth has been used, on this device only, and decides once per
 * open whether this is a good time to ask for a rating (see [ReviewPolicy]).
 */
class ReviewPrompter(context: Context) {
    private val policy = ReviewPolicy()
    private val prefs = context.getSharedPreferences("review", Context.MODE_PRIVATE)
    private var usage = load()
    private var activeSince: Long? = null

    /** Whether this open is one to ask on, until the rating has been requested. */
    var wantsToAsk: Boolean
        private set

    init {
        val now = System.currentTimeMillis()
        usage = usage.copy(launches = usage.launches + 1, firstLaunch = usage.firstLaunch ?: now)
        save()
        wantsToAsk = policy.shouldAsk(usage, now, BuildConfig.VERSION_NAME, Random.nextDouble())
    }

    /** The app came to the front or left it: time in front counts as time played. */
    fun setActive(active: Boolean) {
        val now = System.currentTimeMillis()
        val since = activeSince
        if (active) {
            if (since == null) activeSince = now
        } else if (since != null) {
            usage = usage.copy(millisPlayed = usage.millisPlayed + (now - since).coerceAtLeast(0))
            activeSince = null
            save()
        }
    }

    /** The rating was requested: not again for this version, or for a while. */
    fun asked() {
        wantsToAsk = false
        usage = usage.copy(lastAsked = System.currentTimeMillis(), lastVersionAsked = BuildConfig.VERSION_NAME)
        save()
    }

    private fun load() = ReviewPolicy.Usage(
        launches = prefs.getInt("launches", 0),
        firstLaunch = prefs.getLong("firstLaunch", 0).takeIf { it > 0 },
        millisPlayed = prefs.getLong("millisPlayed", 0),
        lastAsked = prefs.getLong("lastAsked", 0).takeIf { it > 0 },
        lastVersionAsked = prefs.getString("lastVersionAsked", null),
    )

    private fun save() = prefs.edit {
        putInt("launches", usage.launches)
        putLong("firstLaunch", usage.firstLaunch ?: 0)
        putLong("millisPlayed", usage.millisPlayed)
        putLong("lastAsked", usage.lastAsked ?: 0)
        putString("lastVersionAsked", usage.lastVersionAsked)
    }
}
