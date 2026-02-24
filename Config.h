#pragma once

#include <Arduino.h>

// ----------------------------------------------------------------------------
// PIN CONFIGURATION (ESP32-C3)
// ----------------------------------------------------------------------------
constexpr uint8_t PIN_ESC = 5;
constexpr uint8_t PIN_SERVO = 4;
constexpr uint8_t PIN_LIGHTS = 2;
constexpr uint8_t PIN_HORN = 3;

// ----------------------------------------------------------------------------
// PWM CONFIGURATION
// ----------------------------------------------------------------------------
constexpr uint32_t PWM_FREQ = 50;      // 50 Hz for standard ESC and Servo
constexpr uint8_t PWM_RESOLUTION = 14; // 14-bit resolution (0-16383)

// To calculate duty cycle: (Pulse_us / 20000_us) * 2^14
// For 1000us: (1000/20000) * 16384 = 819.2 -> 819
// For 1500us: (1500/20000) * 16384 = 1228.8 -> 1229
// For 2000us: (2000/20000) * 16384 = 1638.4 -> 1638

constexpr uint32_t PWM_DUTY_MIN = 819;      // 1000 us
constexpr uint32_t PWM_DUTY_NEUTRAL = 1229; // 1500 us
constexpr uint32_t PWM_DUTY_MAX = 1638;     // 2000 us

// ----------------------------------------------------------------------------
// SYSTEM TIMERS & DELAYS
// ----------------------------------------------------------------------------
constexpr uint32_t TIME_ESC_STABILIZATION_MS = 2000;
constexpr uint32_t TIME_WATCHDOG_MS = 100;
constexpr uint32_t TIME_HARD_TIMEOUT_MS = 500;
constexpr uint32_t TIME_NEUTRAL_DWELL_MS = 500;
constexpr uint32_t CONTROL_LOOP_INTERVAL_MS = 10;

// ----------------------------------------------------------------------------
// LOGIC CONSTANTS
// ----------------------------------------------------------------------------
constexpr float THROTTLE_DEADBAND = 10.0f; // %
constexpr float SLEW_RATE_MAX_STEP =
    5.0f; // Max 5% change per CONTROL_LOOP_INTERVAL_MS (10ms)
constexpr float REVERSE_LOCKOUT_THRESHOLD_FWD =
    20.0f; // Must be > 20% to trigger lockout check
constexpr float REVERSE_LOCKOUT_THRESHOLD_REV =
    -20.0f; // Must request < -20% reverse to trigger lockout check

// ----------------------------------------------------------------------------
// WIFI & NETWORK CONSTANTS
// ----------------------------------------------------------------------------
constexpr const char *AP_SSID = "RC-Crawler-Pro";
constexpr const char *AP_PASS = "12345687"; // Min 8 chars
constexpr uint16_t WS_PORT = 81;
