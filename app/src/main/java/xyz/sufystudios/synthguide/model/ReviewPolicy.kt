package xyz.sufystudios.synthguide.model

private const val DAY_MILLIS = 86_400_000L

/**
 * When to ask for a Play rating: rarely, never on someone's first visits, and more
 * often the more they've played (as iOS's ReviewPolicy). The ask itself is Play's own
 * card, which has its own limit on how often it shows and may show nothing; an app
 * can't see whether it did, or whether someone has already rated it.
 */
class ReviewPolicy {
    /** What the app has kept about how much it's been used. Times are epoch millis. */
    data class Usage(
        val launches: Int = 0,
        val firstLaunch: Long? = null,
        val millisPlayed: Long = 0,
        val lastAsked: Long? = null,
        val lastVersionAsked: String? = null,
    )

    /** The chance of asking on this open, 0 while it isn't allowed at all. */
    fun chance(usage: Usage, now: Long, version: String): Double {
        val first = usage.firstLaunch ?: return 0.0
        if (usage.launches < MINIMUM_LAUNCHES) return 0.0
        if (now - first < MINIMUM_DAYS * DAY_MILLIS) return 0.0
        if (usage.lastVersionAsked == version) return 0.0
        if (usage.lastAsked != null && now - usage.lastAsked < DAYS_BETWEEN_ASKS * DAY_MILLIS) return 0.0
        val experience = (usage.millisPlayed.toDouble() / EXPERIENCED_MILLIS).coerceIn(0.0, 1.0)
        return LOWEST_CHANCE + (HIGHEST_CHANCE - LOWEST_CHANCE) * experience
    }

    /** Whether to ask on this open, given a random [roll] in 0 until 1. */
    fun shouldAsk(usage: Usage, now: Long, version: String, roll: Double): Boolean =
        roll < chance(usage, now, version)

    private companion object {
        const val MINIMUM_LAUNCHES = 3
        const val MINIMUM_DAYS = 3
        const val DAYS_BETWEEN_ASKS = 60
        const val LOWEST_CHANCE = 0.03
        const val HIGHEST_CHANCE = 0.15
        const val EXPERIENCED_MILLIS = 5 * 60 * 60 * 1000L
    }
}
