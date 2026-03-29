// test_momentum.cpp — MomentumController unit tests
//
// Tests cover: momentum acceleration/deceleration, service braking per
// loco type, consist power scaling, emergency stop, momentum levels,
// and direction change safety.
//
// Uses injectable clock (ClockFn) so time is deterministic.

#include <gtest/gtest.h>

// Include the stub SoundController BEFORE MomentumController.h so the
// forward declaration is satisfied with the full type.
#include "../../test/stubs/SoundController.h"
#include "../../test/stubs/ThrottleManager.h"
#include "../../test/stubs/storage/ConfigStore.h"  // LocoType

// Include the production source directly — compiles MomentumController as
// part of this translation unit so linker finds all symbols.
#include "../../src/core/MomentumController.cpp"

// ============================================================================
// Fake clock — global so a plain function pointer can reference it
// ============================================================================
static unsigned long g_fakeTime = 0;
static unsigned long fakeClock() { return g_fakeTime; }

// ============================================================================
// Test fixture — provides a MomentumController with a fake clock
// ============================================================================

class MomentumTest : public ::testing::Test {
protected:
    SoundController stubSound;

    // Build a MomentumController with our fake clock
    MomentumController mc{&fakeClock};

    void SetUp() override {
        g_fakeTime = 1000;  // Start at 1s (avoid zero-time edge cases)
        // Wire up the stub sound controller so sound events can be inspected
        mc.begin(nullptr, &stubSound);
    }

    // Advance time by ms milliseconds
    void advanceMs(unsigned long ms) { g_fakeTime += ms; }

    // Run the update loop for a given duration, returning final actual speed
    int runFor(int throttle, unsigned long ms, unsigned long stepMs = 50) {
        for (unsigned long t = 0; t < ms; t += stepMs) {
            advanceMs(stepMs);
            mc.update();
        }
        return mc.getActualSpeed(throttle);
    }

    // Run until actual speed reaches target (or timeout)
    int runUntilTarget(int throttle, unsigned long timeoutMs = 60000, unsigned long stepMs = 50) {
        unsigned long elapsed = 0;
        while (elapsed < timeoutMs) {
            advanceMs(stepMs);
            mc.update();
            elapsed += stepMs;
            if (mc.getActualSpeed(throttle) == mc.getTargetSpeed(throttle)) break;
        }
        return mc.getActualSpeed(throttle);
    }
};

// ============================================================================
// Basic momentum behavior
// ============================================================================

TEST_F(MomentumTest, NoMomentum_SpeedChangesInstantly) {
    mc.setMomentumLevel(0, MomentumLevel::Off);
    mc.setTargetSpeed(0, 80);
    // With momentum off, setTargetSpeed snaps actual to target
    EXPECT_EQ(mc.getActualSpeed(0), 80);
}

TEST_F(MomentumTest, WithMomentum_SpeedRampsGradually) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setTargetSpeed(0, 80);

    // Should not jump instantly
    advanceMs(100);
    mc.update();
    int speedAfterOneUpdate = mc.getActualSpeed(0);
    EXPECT_GT(speedAfterOneUpdate, 0);
    EXPECT_LT(speedAfterOneUpdate, 80);
}

TEST_F(MomentumTest, WithMomentum_EventuallyReachesTarget) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setTargetSpeed(0, 80);

    int finalSpeed = runUntilTarget(0, 30000);
    EXPECT_EQ(finalSpeed, 80);
}

TEST_F(MomentumTest, Deceleration_GraduallySlows) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setTargetSpeed(0, 0);
    int speedAfter2s = runFor(0, 2000);

    // Should be between 0 and 80 (decelerating but not stopped yet)
    EXPECT_GT(speedAfter2s, 0);
    EXPECT_LT(speedAfter2s, 80);
}

// ============================================================================
// Momentum levels
// ============================================================================

TEST_F(MomentumTest, HigherMomentum_SlowerAcceleration) {
    // Low momentum — accelerate for 3 seconds
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setTargetSpeed(0, 80);
    int lowSpeed = runFor(0, 3000);

    // Reset
    mc.emergencyStop(0);
    g_fakeTime += 1000; // gap

    // High momentum — accelerate for same 3 seconds
    mc.setMomentumLevel(0, MomentumLevel::High);
    mc.setTargetSpeed(0, 80);
    int highSpeed = runFor(0, 3000);

    // Low momentum should reach higher speed in same time
    EXPECT_GT(lowSpeed, highSpeed);
}

TEST_F(MomentumTest, CycleMomentumLevel_Wraps) {
    mc.setMomentumLevel(0, MomentumLevel::Off);
    EXPECT_EQ(mc.getMomentumLevel(0), MomentumLevel::Off);

    mc.cycleMomentumLevel(0);
    EXPECT_EQ(mc.getMomentumLevel(0), MomentumLevel::Low);

    mc.cycleMomentumLevel(0);
    EXPECT_EQ(mc.getMomentumLevel(0), MomentumLevel::Medium);

    mc.cycleMomentumLevel(0);
    EXPECT_EQ(mc.getMomentumLevel(0), MomentumLevel::High);

    mc.cycleMomentumLevel(0);
    EXPECT_EQ(mc.getMomentumLevel(0), MomentumLevel::Off);
}

TEST_F(MomentumTest, DisablingMomentum_SnapsToTarget) {
    mc.setMomentumLevel(0, MomentumLevel::High);
    mc.setTargetSpeed(0, 80);
    runFor(0, 1000); // partially there

    EXPECT_NE(mc.getActualSpeed(0), 80); // not yet at target

    mc.setMomentumLevel(0, MomentumLevel::Off);
    EXPECT_EQ(mc.getActualSpeed(0), 80); // snapped
}

// ============================================================================
// Emergency stop
// ============================================================================

TEST_F(MomentumTest, EmergencyStop_BypassesMomentum) {
    mc.setMomentumLevel(0, MomentumLevel::High);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.emergencyStop(0);
    EXPECT_EQ(mc.getActualSpeed(0), 0);
    EXPECT_EQ(mc.getTargetSpeed(0), 0);
}

TEST_F(MomentumTest, EmergencyStopAll_StopsAllThrottles) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setMomentumLevel(1, MomentumLevel::Low);
    mc.setTargetSpeed(0, 80);
    mc.setTargetSpeed(1, 60);
    runUntilTarget(0);

    mc.emergencyStopAll();
    EXPECT_EQ(mc.getActualSpeed(0), 0);
    EXPECT_EQ(mc.getActualSpeed(1), 0);
}

// ============================================================================
// Service braking — basic behavior
// ============================================================================

TEST_F(MomentumTest, DynamicBrake_DecreasesActualSpeed) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    int speed = runFor(0, 2000);

    EXPECT_LT(speed, 80);
    EXPECT_GT(speed, 0);
}

TEST_F(MomentumTest, DynamicBrake_TargetSpeedUnchanged) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    runFor(0, 2000);

    // Display speed (target) should still show 80
    EXPECT_EQ(mc.getTargetSpeed(0), 80);
    // But actual speed should be lower
    EXPECT_LT(mc.getActualSpeed(0), 80);
}

TEST_F(MomentumTest, DynamicBrake_ReleaseReturnsToTarget) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    runFor(0, 3000); // brake for 3 seconds

    mc.setDynamicBraking(0, false);
    int finalSpeed = runUntilTarget(0, 30000);

    EXPECT_EQ(finalSpeed, 80); // should return to set speed
}

TEST_F(MomentumTest, DynamicBrake_WontEngageBelowMinSpeed) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setLocoType(0, LocoType::Diesel); // minSpeed = 10
    mc.setTargetSpeed(0, 5);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    // Should refuse — speed is below the profile's minimum
    EXPECT_FALSE(mc.isDynamicBraking(0));
}

TEST_F(MomentumTest, DynamicBrake_EngagesAboveMinSpeed) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setLocoType(0, LocoType::Diesel); // minSpeed = 10
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    EXPECT_TRUE(mc.isDynamicBraking(0));
}

// ============================================================================
// Service braking — loco-type-dependent rates
// ============================================================================

TEST_F(MomentumTest, ElectricBrakesHarderThanDiesel) {
    // Diesel
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);
    mc.setDynamicBraking(0, true);
    int dieselSpeed = runFor(0, 2000);
    mc.setDynamicBraking(0, false);

    // Reset for electric test on throttle 1
    mc.setMomentumLevel(1, MomentumLevel::Medium);
    mc.setLocoType(1, LocoType::Electric);
    mc.setTargetSpeed(1, 80);
    runUntilTarget(1);
    mc.setDynamicBraking(1, true);
    int electricSpeed = runFor(1, 2000);

    // Electric should have shed more speed (lower actual)
    EXPECT_LT(electricSpeed, dieselSpeed);
}

TEST_F(MomentumTest, SteamBrakesGentlerThanDiesel) {
    // Diesel
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);
    mc.setDynamicBraking(0, true);
    int dieselSpeed = runFor(0, 2000);

    // Steam on throttle 1
    mc.setMomentumLevel(1, MomentumLevel::Medium);
    mc.setLocoType(1, LocoType::Steam);
    mc.setTargetSpeed(1, 80);
    runUntilTarget(1);
    mc.setDynamicBraking(1, true);
    int steamSpeed = runFor(1, 2000);

    // Steam should have shed less speed (higher actual = gentler)
    EXPECT_GT(steamSpeed, dieselSpeed);
}

TEST_F(MomentumTest, BrakeProfile_DieselValues) {
    mc.setLocoType(0, LocoType::Diesel);
    auto& profile = mc.getBrakeProfile(0);
    EXPECT_FLOAT_EQ(profile.decelRate, 4.0f);
    EXPECT_EQ(profile.minSpeed, 20);
}

TEST_F(MomentumTest, BrakeProfile_SteamValues) {
    mc.setLocoType(0, LocoType::Steam);
    auto& profile = mc.getBrakeProfile(0);
    EXPECT_FLOAT_EQ(profile.decelRate, 3.0f);
    EXPECT_EQ(profile.minSpeed, 20);
}

TEST_F(MomentumTest, BrakeProfile_ElectricValues) {
    mc.setLocoType(0, LocoType::Electric);
    auto& profile = mc.getBrakeProfile(0);
    EXPECT_FLOAT_EQ(profile.decelRate, 5.0f);
    EXPECT_EQ(profile.minSpeed, 10);
}

// ============================================================================
// Consist power scaling
// ============================================================================

TEST_F(MomentumTest, LargerConsist_AcceleratesFaster) {
    // Single loco
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setConsistSize(0, 1);
    mc.setTargetSpeed(0, 80);
    int singleAfter3s = runFor(0, 3000);

    // Reset
    mc.emergencyStop(0);
    g_fakeTime += 1000;

    // Four-unit consist
    mc.setConsistSize(0, 4);
    mc.setTargetSpeed(0, 80);
    int quadAfter3s = runFor(0, 3000);

    EXPECT_GT(quadAfter3s, singleAfter3s);
}

TEST_F(MomentumTest, ConsistSize_DoesNotExceedTarget) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setConsistSize(0, 4);
    mc.setTargetSpeed(0, 80);

    int speed = runUntilTarget(0);
    EXPECT_EQ(speed, 80); // exact, not over
}

TEST_F(MomentumTest, ConsistSize_DefaultIsOne) {
    EXPECT_EQ(mc.getConsistSize(0), 1);
}

TEST_F(MomentumTest, ConsistSize_ClampsToOne) {
    mc.setConsistSize(0, 0); // invalid
    EXPECT_EQ(mc.getConsistSize(0), 1);
}

// ============================================================================
// Standard braking (throttle=0, hold encoder)
// ============================================================================

TEST_F(MomentumTest, Braking_AcceleratesDeceleration) {
    mc.setMomentumLevel(0, MomentumLevel::High);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    // Coast (no brake)
    mc.setTargetSpeed(0, 0);
    int coastSpeed = runFor(0, 2000);

    // Reset for braking test
    mc.emergencyStop(0);
    g_fakeTime += 1000;
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setTargetSpeed(0, 0);
    mc.setBraking(0, true);
    int brakeSpeed = runFor(0, 2000);

    // With braking, speed should be lower (decelerated faster)
    EXPECT_LT(brakeSpeed, coastSpeed);
}

// ============================================================================
// LocoType persistence
// ============================================================================

TEST_F(MomentumTest, LocoType_DefaultIsDiesel) {
    EXPECT_EQ(mc.getLocoType(0), LocoType::Diesel);
}

TEST_F(MomentumTest, LocoType_SetAndGet) {
    mc.setLocoType(0, LocoType::Steam);
    EXPECT_EQ(mc.getLocoType(0), LocoType::Steam);

    mc.setLocoType(0, LocoType::Electric);
    EXPECT_EQ(mc.getLocoType(0), LocoType::Electric);
}

// ============================================================================
// Sound controller event dispatch
// ============================================================================

TEST_F(MomentumTest, DynamicBrake_NotifiesSound) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setLocoType(0, LocoType::Diesel);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.setDynamicBraking(0, true);
    EXPECT_EQ(stubSound.dynamicBrakeCallCount, 1);
    EXPECT_EQ(stubSound.lastDynamicBrakeThrottle, 0);
    EXPECT_TRUE(stubSound.lastDynamicBrakeState);

    mc.setDynamicBraking(0, false);
    EXPECT_EQ(stubSound.dynamicBrakeCallCount, 2);
    EXPECT_FALSE(stubSound.lastDynamicBrakeState);
}

TEST_F(MomentumTest, StandardBrake_NotifiesSound) {
    mc.setBraking(0, true);
    EXPECT_EQ(stubSound.brakeCallCount, 1);
    EXPECT_TRUE(stubSound.lastBrakeState);

    mc.setBraking(0, false);
    EXPECT_EQ(stubSound.brakeCallCount, 2);
    EXPECT_FALSE(stubSound.lastBrakeState);
}

TEST_F(MomentumTest, SpeedChange_NotifiesSound) {
    mc.setMomentumLevel(0, MomentumLevel::Off);
    mc.setTargetSpeed(0, 50);
    EXPECT_EQ(stubSound.speedChangeCallCount, 1);
    EXPECT_EQ(stubSound.lastOldSpeed, 0);
    EXPECT_EQ(stubSound.lastNewSpeed, 50);
}

// ============================================================================
// Direction change safety
// ============================================================================

TEST_F(MomentumTest, DirectionChange_WhileStopped_NotQueued) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    // Train is stopped
    bool queued = mc.requestDirectionChange(0, Reverse);
    EXPECT_FALSE(queued);
    EXPECT_FALSE(mc.hasPendingDirectionChange(0));
}

TEST_F(MomentumTest, DirectionChange_WhileMoving_Queued) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    bool queued = mc.requestDirectionChange(0, Reverse);
    EXPECT_TRUE(queued);
    EXPECT_TRUE(mc.hasPendingDirectionChange(0));
    EXPECT_EQ(mc.getPendingDirection(0), Reverse);
}

TEST_F(MomentumTest, DirectionChange_DoubleToggle_Cancels) {
    mc.setMomentumLevel(0, MomentumLevel::Medium);
    mc.setTargetSpeed(0, 80);
    runUntilTarget(0);

    mc.requestDirectionChange(0, Reverse);
    EXPECT_TRUE(mc.hasPendingDirectionChange(0));

    // Second toggle cancels
    mc.requestDirectionChange(0, Forward);
    EXPECT_FALSE(mc.hasPendingDirectionChange(0));
}

// ============================================================================
// Boundary conditions
// ============================================================================

TEST_F(MomentumTest, SpeedClampsTo126) {
    mc.setMomentumLevel(0, MomentumLevel::Off);
    mc.setTargetSpeed(0, 200); // over max
    EXPECT_EQ(mc.getTargetSpeed(0), 126);
    EXPECT_EQ(mc.getActualSpeed(0), 126);
}

TEST_F(MomentumTest, SpeedClampsToZero) {
    mc.setMomentumLevel(0, MomentumLevel::Off);
    mc.setTargetSpeed(0, -5); // negative
    EXPECT_EQ(mc.getTargetSpeed(0), 0);
    EXPECT_EQ(mc.getActualSpeed(0), 0);
}

TEST_F(MomentumTest, InvalidThrottle_ReturnsDefaults) {
    EXPECT_EQ(mc.getActualSpeed(-1), 0);
    EXPECT_EQ(mc.getActualSpeed(99), 0);
    EXPECT_EQ(mc.getTargetSpeed(-1), 0);
    EXPECT_FALSE(mc.isDynamicBraking(-1));
    EXPECT_FALSE(mc.isBraking(99));
    EXPECT_EQ(mc.getConsistSize(-1), 1);
    EXPECT_EQ(mc.getLocoType(-1), LocoType::Diesel);
}

// ============================================================================
// Notching blocks momentum updates
// ============================================================================

TEST_F(MomentumTest, Notching_DelaysMomentumUpdate) {
    mc.setMomentumLevel(0, MomentumLevel::Low);
    mc.setTargetSpeed(0, 80);

    stubSound.notching = true;

    // Updates but notching blocks speed change
    runFor(0, 1000);
    // Speed should barely move (or not at all) while notching
    int speedWhileNotching = mc.getActualSpeed(0);

    stubSound.notching = false;
    runFor(0, 1000);
    int speedAfterNotching = mc.getActualSpeed(0);

    // After notching stops, speed should advance
    EXPECT_GT(speedAfterNotching, speedWhileNotching);
}

// ============================================================================
// getBrakeProfile — out of range
// ============================================================================

TEST_F(MomentumTest, BrakeProfile_InvalidThrottle_ReturnsDiesel) {
    auto& profile = mc.getBrakeProfile(-1);
    EXPECT_FLOAT_EQ(profile.decelRate, 4.0f); // Diesel defaults
}

// ============================================================================
// Entry point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
