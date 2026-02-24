// Last updated: Monday Feb 23, 2026, 23:03
#include <Arduino.h>
#define RUN_UNIT_TESTS
#include "Config.h"
#include "MotionLayer.h"
#include "NetworkLayer.h"
#include "SafetyLayer.h"
#include "StateMachine.h"
#include "Tests.h"
#include "Types.h"

// Initialize components architecture
RCNetworkManager network;
MotionManager motion;
SafetyManager safety;
RCStateMachine stateMachine(&network, &motion, &safety);

void setup() {
  Serial.begin(115200);
  network.setMotionManager(&motion);
  delay(10);
  Serial.println("\n--- RC Crawler Pro Supervisor Booting ---");

  // 1. Initialize Motion FIRST to lock PWM to neutral before WiFi or other
  // logic starts
  Serial.println("Init Motion...");
  motion.begin();

  // 2. Initialize Network (WiFi AP, HTTP Server, WebSocket)
  Serial.println("Init Network...");
  network.begin();

  // 3. Start State Machine Supervisor
  Serial.println("Init State Machine...");
  stateMachine.begin();

#ifdef RUN_UNIT_TESTS
  RCUnitTester tester;
  tester.runAllTests();
#endif

  Serial.println("SYSTEM READY. AP SSID: RC-Crawler-Pro");
}

void loop() {
  // 1. Read Inputs & Maintain Connection (Non-blocking)
  network.loop();

  // 2. Evaluate State, Apply Safety Rules, Issue Hardware Commands (Fixed 100
  // Hz timing inside)
  stateMachine.update();
}
