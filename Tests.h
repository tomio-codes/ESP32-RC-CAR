#pragma once

#include "Config.h"
#include "SafetyLayer.h"
#include <Arduino.h>

class RCUnitTester {
private:
  int testsPassed = 0;
  int testsFailed = 0;

  void assertFloat(const char *testName, float expected, float actual,
                   float epsilon = 0.01f) {
    if (abs(expected - actual) <= epsilon) {
      Serial.printf("[PASS] %s\n", testName);
      testsPassed++;
    } else {
      Serial.printf("[FAIL] %s - Expected: %.2f, Got: %.2f\n", testName,
                    expected, actual);
      testsFailed++;
    }
  }

  void assertBool(const char *testName, bool expected, bool actual) {
    if (expected == actual) {
      Serial.printf("[PASS] %s\n", testName);
      testsPassed++;
    } else {
      Serial.printf("[FAIL] %s - Expected: %s, Got: %s\n", testName,
                    expected ? "TRUE" : "FALSE", actual ? "TRUE" : "FALSE");
      testsFailed++;
    }
  }

public:
  void runAllTests() {
    Serial.println("\n==================================");
    Serial.println("  STARTING NATIVE UNIT TESTS ");
    Serial.println("==================================");

    SafetyManager sm;

    // 1. Deadband Logic Test
    assertFloat("Deadband: Neutral Input (0)", 0.0f,
                sm.applyDeadband(0.0f, 5.0f));
    delay(15);
    assertFloat("Deadband: Under Threshold (4.9)", 0.0f,
                sm.applyDeadband(4.9f, 5.0f));
    delay(15);
    assertFloat("Deadband: Over Threshold (5.1)", 5.1f,
                sm.applyDeadband(5.1f, 5.0f));
    delay(15);
    assertFloat("Deadband: Under Negative (-4.9)", 0.0f,
                sm.applyDeadband(-4.9f, 5.0f));
    delay(15);
    assertFloat("Deadband: Over Negative (-5.1)", -5.1f,
                sm.applyDeadband(-5.1f, 5.0f));
    delay(15);

    // 2. Slew Rate Limiter Test (Max step 5.0)
    assertFloat("SlewRate: Acceleration (+15 from 0)", 5.0f,
                sm.applySlewRate(15.0f, 0.0f, 5.0f));
    delay(15);
    assertFloat("SlewRate: Acceleration (+18 from 5)", 10.0f,
                sm.applySlewRate(18.0f, 5.0f, 5.0f));
    delay(15);
    assertFloat("SlewRate: Deceleration (0 from 20)", 15.0f,
                sm.applySlewRate(0.0f, 20.0f, 5.0f));
    delay(15);
    assertFloat("SlewRate: Small Step (+2 from 0)", 2.0f,
                sm.applySlewRate(2.0f, 0.0f, 5.0f));
    delay(15);

    // Additional test cases
    assertFloat("SlewRate: Deceleration (-5 from 10)", 5.0f,
                sm.applySlewRate(-5.0f, 10.0f, 5.0f));
    delay(15);
    assertFloat("SlewRate: No change (5 from 5)", 5.0f,
                sm.applySlewRate(5.0f, 5.0f, 5.0f));
    delay(15);

    Serial.println("----------------------------------");
    Serial.printf("TESTS COMPLETED: %d PASSED | %d FAILED\n", testsPassed,
                  testsFailed);
    Serial.println("==================================\n");
  }
};
