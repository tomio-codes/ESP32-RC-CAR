#pragma once

#include <Arduino.h>

// ----------------------------------------------------------------------------
// STATE MACHINE STATES
// ----------------------------------------------------------------------------
enum class RCState {
  BOOT,             // Initial power up
  INIT_ESC,         // ESC stabilization period (2 seconds neutral)
  IDLE_NO_CLIENT,   // Waiting for WiFi client to connect
  CLIENT_CONNECTED, // Client connected, waiting for first control inputs
  ACTIVE_CONTROL,   // Normal operation
  FAILSAFE, // Connection lost -> neutral, waiting to reconnect or hard disable
  BRAKING,  // Active braking
  WAIT_FOR_NEUTRAL_DWELL // Requires 500ms neutral dwell before reversing
};

// ----------------------------------------------------------------------------
// CONTROL DATA STRUCTURES
// ----------------------------------------------------------------------------
struct ControlInput {
  float throttle; // -100.0 to 100.0
  float steering; // -100.0 to 100.0
  bool lights;
  bool horn;
  uint32_t timestamp; // Time when packet was received
};
