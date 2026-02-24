#pragma once

#include "Config.h"
#include "Types.h"
#include <Arduino.h>

class SafetyManager {
private:
  float currentThrottle;
  float currentSteering;
  float filteredThrottle; // Low-pass filtered throttle to reduce joystick noise

public:
  SafetyManager() : currentThrottle(0.0f), currentSteering(0.0f), filteredThrottle(0.0f) {}

  // Applies deadband to raw input
  float applyDeadband(float value, float deadband) {
    if (abs(value) < deadband) {
      return 0.0f;
    }
    // Shift the output so it starts from 0 at the deadband edge
    if (value > 0)
      return (value - deadband) * (100.0f / (100.0f - deadband));
    else
      return (value + deadband) * (100.0f / (100.0f - deadband));
  }

  // Applies low-pass filter to reduce joystick noise and stabilize throttle
  float applyLowPassFilter(float rawValue, float filteredValue, float alpha = 0.3f) {
    // Alpha = 0.3 means 30% new value, 70% old filtered value
    // Lower alpha = more filtering = more stable but slower response
    return filteredValue + alpha * (rawValue - filteredValue);
  }

  // Applies slew rate limiter to prevent sudden changes
  // Limit both acceleration and deceleration to prevent jerky movements
  float applySlewRate(float target, float current, float maxStep) {
    float delta = target - current;

    // If change is larger than maxStep, limit it
    if (fabs(delta) > maxStep) {
      return current + (delta > 0.0f ? maxStep : -maxStep);
    }

    // Small changes are applied immediately
    return target;
  }

  // Process the input according to safety rules
  void processInput(const ControlInput &rawInput, ControlInput &safeOutput) {
    // 1. Apply low-pass filter to throttle to reduce joystick noise
    filteredThrottle = applyLowPassFilter(rawInput.throttle, filteredThrottle, 0.3f);

    // 2. Deadband (applied to filtered value)
    float targetThrottle = applyDeadband(filteredThrottle, THROTTLE_DEADBAND);
    float targetSteering = applyDeadband(rawInput.steering, THROTTLE_DEADBAND);

    // 2. Slew Rate Limiter (only on acceleration to protect gearbox, braking
    // must be instant!)
    if (targetThrottle >= 0.0f) {
      currentThrottle =
          applySlewRate(targetThrottle, currentThrottle, SLEW_RATE_MAX_STEP);
    } else {
      // Instant braking! Crawler ESCs need the sudden drop to negative duty to
      // engage drag-brake!
      currentThrottle = targetThrottle;
    }
    currentSteering = targetSteering;

    // Ensure exact 0 if very close due to float precision
    if (abs(currentThrottle) < 0.01f)
      currentThrottle = 0.0f;

    // Output assignment
    safeOutput.throttle = currentThrottle;
    safeOutput.steering = currentSteering;
    safeOutput.lights = rawInput.lights;
    safeOutput.horn = rawInput.horn;
    safeOutput.timestamp = rawInput.timestamp;
  }

  void forceNeutralImmediate() {
    currentThrottle = 0.0f;
    currentSteering = 0.0f;
    filteredThrottle = 0.0f; // Also reset filter state for clean start
  }

  float getCurrentThrottle() const { return currentThrottle; }
};
