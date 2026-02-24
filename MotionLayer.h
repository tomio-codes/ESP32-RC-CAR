#pragma once

#include "Config.h"
#include "driver/ledc.h"
#include <Arduino.h>
#include <Preferences.h>

class MotionManager {
private:
  uint32_t lastEscDuty = 0;
  uint32_t lastServoDuty = 0;
  float steeringTrimmer = 0.0f; // -20.0 to 20.0
  float throttleTrimmer = 0.0f; // -10.0 to 10.0 (compensate ESC drift)
  Preferences prefs;

  // Exponential curve transformation for better crawler control at low speeds
  // Factor < 1.0 makes curve convex (low values amplified), > 1.0 concave (low values attenuated)
  float applyExpo(float input, float factor = 2.0f) {
    // Keeps sign, applies power curve mapping
    float sign = (input < 0.0f) ? -1.0f : 1.0f;
    float norm = abs(input) / 100.0f;
    float curved = pow(norm, factor) * 100.0f;
    return curved * sign;
  }

  uint32_t percentToDuty(float percent) {
    // Percent is -100 to 100
    // -100 -> PWM_DUTY_MIN (1000us)
    // 0    -> PWM_DUTY_NEUTRAL (1500us)
    // 100  -> PWM_DUTY_MAX (2000us)

    // MANDATORY SAFETY: Absolute neutral gating
    if (abs(percent) < 0.01f) {
      return PWM_DUTY_NEUTRAL;
    }

    percent = constrain(percent, -100.0f, 100.0f);

    // Apply Throttle Trimmer to compensate for hardware center drift
    float adjustedPercent = percent + throttleTrimmer;
    adjustedPercent = constrain(adjustedPercent, -100.0f, 100.0f);

    if (adjustedPercent >= 0.0f) {
      float range = PWM_DUTY_MAX - PWM_DUTY_NEUTRAL;
      return PWM_DUTY_NEUTRAL + (uint32_t)((adjustedPercent / 100.0f) * range);
    } else {
      float range = PWM_DUTY_NEUTRAL - PWM_DUTY_MIN;
      return PWM_DUTY_NEUTRAL -
             (uint32_t)((abs(adjustedPercent) / 100.0f) * range);
    }
  }

public:
  void begin() {
    // Configure Timer (Must be zero-initialized to prevent stack garbage bugs!)
    ledc_timer_config_t ledc_timer = {};
    ledc_timer.speed_mode = LEDC_LOW_SPEED_MODE;
    ledc_timer.timer_num = LEDC_TIMER_0;
    ledc_timer.duty_resolution = (ledc_timer_bit_t)PWM_RESOLUTION;
    ledc_timer.freq_hz = PWM_FREQ;
    // Use APB_CLK for higher stability on C3 at 50Hz
    ledc_timer.clk_cfg = LEDC_USE_APB_CLK;
    ledc_timer_config(&ledc_timer);

    // Configure ESC Channel
    ledc_channel_config_t esc_channel = {};
    esc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    esc_channel.channel = LEDC_CHANNEL_0;
    esc_channel.timer_sel = LEDC_TIMER_0;
    esc_channel.intr_type = LEDC_INTR_DISABLE;
    esc_channel.gpio_num = PIN_ESC;
    esc_channel.duty = PWM_DUTY_NEUTRAL;
    esc_channel.hpoint = 0;
    ledc_channel_config(&esc_channel);

    // Lock duty immediately!
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, PWM_DUTY_NEUTRAL);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    // Configure Servo Channel
    ledc_channel_config_t servo_channel = {};
    servo_channel.speed_mode = LEDC_LOW_SPEED_MODE;
    servo_channel.channel = LEDC_CHANNEL_1;
    servo_channel.timer_sel = LEDC_TIMER_0;
    servo_channel.intr_type = LEDC_INTR_DISABLE;
    servo_channel.gpio_num = PIN_SERVO;
    servo_channel.duty = PWM_DUTY_NEUTRAL;
    servo_channel.hpoint = 0;
    ledc_channel_config(&servo_channel);

    // Lock duty immediately!
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, PWM_DUTY_NEUTRAL);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);

    // Lights and Horn as simple GPIO
    pinMode(PIN_LIGHTS, OUTPUT);
    pinMode(PIN_HORN, OUTPUT);
    digitalWrite(PIN_LIGHTS, LOW);
    digitalWrite(PIN_HORN, LOW);

    loadSettings();
  }

  void loadSettings() {
    prefs.begin("rc-crawler", true); // Open in read-only mode first
    steeringTrimmer = prefs.getFloat("trim", 0.0f);
    throttleTrimmer = prefs.getFloat("ttrim", 0.0f);
    prefs.end();
    Serial.printf("Loaded saved trimmer: %.1f\n", steeringTrimmer);
  }

  void saveTrimmer(float val) {
    steeringTrimmer = constrain(val, -20.0f, 20.0f);
    prefs.begin("rc-crawler", false); // Open in read-write mode
    prefs.putFloat("trim", steeringTrimmer);
    prefs.end();
    Serial.printf("Saved new trimmer: %.1f\n", steeringTrimmer);
  }

  void setTrimmer(float val) {
    steeringTrimmer = constrain(val, -20.0f, 20.0f);
  }
  float getTrimmer() { return steeringTrimmer; }

  void saveThrottleTrimmer(float val) {
    throttleTrimmer = constrain(val, -10.0f, 10.0f);
    prefs.begin("rc-crawler", false);
    prefs.putFloat("ttrim", throttleTrimmer);
    prefs.end();
  }
  void setThrottleTrimmer(float val) {
    throttleTrimmer = constrain(val, -10.0f, 10.0f);
  }
  float getThrottleTrimmer() { return throttleTrimmer; }

  void disableEscOutput() { forceNeutral(); }

  void forceNeutral() {
    uint32_t dutyNeutral = PWM_DUTY_NEUTRAL;
    if (dutyNeutral != lastEscDuty) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, dutyNeutral);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      lastEscDuty = dutyNeutral;
    }

    if (dutyNeutral != lastServoDuty) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, dutyNeutral);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
      lastServoDuty = dutyNeutral;
    }
  }

  void setOutputs(float throttle, float steering) {
    // Apply Expo to throttle for better low speed control, but skip for
    // braking! Use factor < 1.0 to amplify low values (motor starts earlier)
    float outThrottle = throttle;
    if (throttle > 0.0f) {
      outThrottle = applyExpo(throttle, 0.8f); // 0.8 power factor - amplifies low speeds
    }

    uint32_t escDuty = percentToDuty(outThrottle);

    // Apply Trimmer to steering
    float adjustedSteering = steering + steeringTrimmer;
    uint32_t servoDuty = percentToDuty(adjustedSteering);

    if (escDuty != lastEscDuty) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, escDuty);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
      lastEscDuty = escDuty;
    }

    if (servoDuty != lastServoDuty) {
      ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, servoDuty);
      ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
      lastServoDuty = servoDuty;
    }
  }

  void setAuxiliary(bool lights, bool horn) {
    digitalWrite(PIN_LIGHTS, lights ? HIGH : LOW);
    digitalWrite(PIN_HORN, horn ? HIGH : LOW);
  }
};
