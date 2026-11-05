package xyz.sufystudios.synthguide.model

import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test

private const val DAY = 86_400_000L
private const val HOUR = 3_600_000L
private const val START = 1_800_000_000_000L

/** When Drone Synth asks for a Play rating (as iOS's ReviewPolicyTests). */
class ReviewPolicyTest {
    private val policy = ReviewPolicy()

    private fun usage(launches: Int, hours: Double = 0.0) = ReviewPolicy.Usage(
        launches = launches,
        firstLaunch = START,
        millisPlayed = (hours * HOUR).toLong(),
    )

    @Test
    fun neverOnTheFirstVisitsOrInTheFirstFewDays() {
        assertEquals(0.0, policy.chance(usage(1), START + 30 * DAY, "3.1.5"), 0.0)
        assertEquals(0.0, policy.chance(usage(2), START + 30 * DAY, "3.1.5"), 0.0)
        assertEquals(0.0, policy.chance(usage(20), START + DAY, "3.1.5"), 0.0)
        assertTrue(policy.chance(usage(3), START + 3 * DAY, "3.1.5") > 0)
    }

    @Test
    fun rarelyForANewcomerMoreOftenTheMoreTheyvePlayed() {
        val now = START + 30 * DAY
        val newcomer = policy.chance(usage(5, 0.0), now, "3.1.5")
        val regular = policy.chance(usage(5, 2.5), now, "3.1.5")
        val experienced = policy.chance(usage(5, 50.0), now, "3.1.5")
        assertEquals(0.03, newcomer, 1e-9)
        assertTrue(regular > newcomer && regular < experienced)
        assertEquals("capped however long they've played", 0.15, experienced, 1e-9)
    }

    @Test
    fun oncePerVersionAndNotAgainFor60Days() {
        val asked = usage(10, 10.0).copy(lastAsked = START + 100 * DAY, lastVersionAsked = "3.1.5")
        assertEquals("this version's been asked", 0.0, policy.chance(asked, START + 200 * DAY, "3.1.5"), 0.0)
        assertEquals("too soon after the last ask", 0.0, policy.chance(asked, START + 130 * DAY, "3.2.0"), 0.0)
        assertTrue(policy.chance(asked, START + 161 * DAY, "3.2.0") > 0)
    }

    @Test
    fun theRollDecidesSoMostEligibleOpensDontAsk() {
        val now = START + 30 * DAY
        assertTrue(policy.shouldAsk(usage(5), now, "3.1.5", roll = 0.01))
        assertFalse(policy.shouldAsk(usage(5), now, "3.1.5", roll = 0.5))
    }
}
