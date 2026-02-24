#pragma once

#include "Config.h"
#include "MotionLayer.h"
#include "NetworkLayer.h"
#include "SafetyLayer.h"
#include "Types.h"
#include <Arduino.h>

class RCStateMachine {
private:
  RCState currentState;
  RCState previousState;
  uint32_t stateEntryTime;

  RCNetworkManager *network;
  MotionManager *motion;
  SafetyManager *safety;

  uint32_t lastLoopTime;
  bool reverseArmed;

  void changeState(RCState newState) {
    // EXIT Actions
    switch (currentState) {
    case RCState::INIT_ESC:
      break;
    case RCState::IDLE_NO_CLIENT:
      break;
    case RCState::FAILSAFE:
      break;
    default:
      break;
    }

    previousState = currentState;
    currentState = newState;
    stateEntryTime = millis();

    // ENTRY Actions
    switch (currentState) {
    case RCState::INIT_ESC:
      motion->forceNeutral();
      break;
    case RCState::IDLE_NO_CLIENT:
      motion->disableEscOutput();
      safety->forceNeutralImmediate();
      break;
    case RCState::CLIENT_CONNECTED:
      break;
    case RCState::ACTIVE_CONTROL:
      break;
    case RCState::FAILSAFE:
      safety->forceNeutralImmediate();
      motion->forceNeutral();
      break;
    case RCState::BRAKING:
      break;
    case RCState::WAIT_FOR_NEUTRAL_DWELL:
      safety->forceNeutralImmediate();
      motion->forceNeutral();
      break;
    default:
      break;
    }
  }

public:
  RCStateMachine(RCNetworkManager *net, MotionManager *mot, SafetyManager *saf)
      : currentState(RCState::BOOT), previousState(RCState::BOOT),
        stateEntryTime(0), network(net),
        motion(mot), safety(saf), lastLoopTime(0),
        reverseArmed(false) {}

  void begin() { changeState(RCState::INIT_ESC); }

  void update() {
    uint32_t now = millis();
    // Enforce 10ms control loop interval (100 Hz)
    if (now - lastLoopTime < CONTROL_LOOP_INTERVAL_MS) {
      return;
    }
    lastLoopTime = now;

    uint32_t timeInState = now - stateEntryTime;

    switch (currentState) {
    case RCState::BOOT:
      break;

    case RCState::INIT_ESC:
      if (timeInState >= TIME_ESC_STABILIZATION_MS) {
        changeState(RCState::IDLE_NO_CLIENT);
      }
      break;

    case RCState::IDLE_NO_CLIENT:
      if (network->hasClient()) {
        changeState(RCState::CLIENT_CONNECTED);
      }
      break;

    case RCState::CLIENT_CONNECTED:
      if (!network->hasClient()) {
        changeState(RCState::IDLE_NO_CLIENT);
      } else if (network->getTimeSinceLastPacket() < TIME_WATCHDOG_MS) {
        ControlInput raw = network->getLastInput();
        if (abs(raw.throttle) <= THROTTLE_DEADBAND) {
          reverseArmed = true;
          changeState(RCState::ACTIVE_CONTROL);
        }
      }
      break;

    case RCState::ACTIVE_CONTROL:
      if (!network->hasClient()) {
        changeState(RCState::IDLE_NO_CLIENT);
      } else if (network->getTimeSinceLastPacket() >= TIME_WATCHDOG_MS) {
        changeState(RCState::FAILSAFE);
      } else {
        ControlInput raw = network->getLastInput();

        if (raw.throttle > THROTTLE_DEADBAND) {
          reverseArmed = false;
          ControlInput safeOut;
          safety->processInput(raw, safeOut);
          motion->setOutputs(safeOut.throttle, safeOut.steering);
          motion->setAuxiliary(safeOut.lights, safeOut.horn);
        } else if (raw.throttle < -THROTTLE_DEADBAND) {
          if (reverseArmed) {
            ControlInput safeOut;
            safety->processInput(raw, safeOut);
            motion->setOutputs(safeOut.throttle, safeOut.steering);
            motion->setAuxiliary(safeOut.lights, safeOut.horn);
          } else {
            changeState(RCState::BRAKING);
          }
        } else {
          ControlInput cleanRaw = raw;
          cleanRaw.throttle = 0.0f;
          ControlInput safeOut;
          safety->processInput(cleanRaw, safeOut);
          motion->setOutputs(safeOut.throttle, safeOut.steering);
          motion->setAuxiliary(safeOut.lights, safeOut.horn);
        }
      }
      break;

    case RCState::BRAKING:
      if (!network->hasClient()) {
        changeState(RCState::IDLE_NO_CLIENT);
      } else if (network->getTimeSinceLastPacket() >= TIME_WATCHDOG_MS) {
        changeState(RCState::FAILSAFE);
      } else {
        ControlInput raw = network->getLastInput();

        if (raw.throttle < -THROTTLE_DEADBAND) {
          ControlInput safeOut;
          safety->processInput(raw, safeOut);
          motion->setOutputs(safeOut.throttle, safeOut.steering);
          motion->setAuxiliary(safeOut.lights, safeOut.horn);
        } else if (abs(raw.throttle) <= THROTTLE_DEADBAND) {
          ControlInput cleanRaw = raw;
          cleanRaw.throttle = 0.0f;
          ControlInput safeOut;
          safety->processInput(cleanRaw, safeOut);
          motion->setOutputs(safeOut.throttle, safeOut.steering);
          motion->setAuxiliary(safeOut.lights, safeOut.horn);
          changeState(RCState::WAIT_FOR_NEUTRAL_DWELL);
        } else {
          changeState(RCState::ACTIVE_CONTROL);
        }
      }
      break;

    case RCState::WAIT_FOR_NEUTRAL_DWELL:
      if (!network->hasClient()) {
        changeState(RCState::IDLE_NO_CLIENT);
      } else if (network->getTimeSinceLastPacket() >= TIME_WATCHDOG_MS) {
        changeState(RCState::FAILSAFE);
      } else {
        ControlInput raw = network->getLastInput();

        if (raw.throttle > THROTTLE_DEADBAND) {
          changeState(RCState::ACTIVE_CONTROL);
        } else if (raw.throttle < -THROTTLE_DEADBAND) {
          if (timeInState >= TIME_NEUTRAL_DWELL_MS) {
            reverseArmed = true;
            changeState(RCState::ACTIVE_CONTROL);
          } else {
            ControlInput brakedRaw = raw;
            brakedRaw.throttle = 0.0f;
            ControlInput safeOut;
            safety->processInput(brakedRaw, safeOut);
            motion->setOutputs(safeOut.throttle, safeOut.steering);
            motion->setAuxiliary(safeOut.lights, safeOut.horn);
          }
        } else {
          if (timeInState >= TIME_NEUTRAL_DWELL_MS) {
            reverseArmed = true;
            changeState(RCState::ACTIVE_CONTROL);
          } else {
            ControlInput brakedRaw = raw;
            brakedRaw.throttle = 0.0f;
            ControlInput safeOut;
            safety->processInput(brakedRaw, safeOut);
            motion->setOutputs(safeOut.throttle, safeOut.steering);
            motion->setAuxiliary(safeOut.lights, safeOut.horn);
          }
        }
      }
      break;

    case RCState::FAILSAFE:
      if (!network->hasClient()) {
        changeState(RCState::IDLE_NO_CLIENT);
      } else {
        safety->forceNeutralImmediate();
        motion->forceNeutral();

        if (timeInState >= TIME_HARD_TIMEOUT_MS) {
          motion->disableEscOutput();

          if (network->getTimeSinceLastPacket() < TIME_WATCHDOG_MS) {
            changeState(RCState::CLIENT_CONNECTED);
          }
        } else if (network->getTimeSinceLastPacket() < TIME_WATCHDOG_MS) {
          changeState(RCState::ACTIVE_CONTROL);
        }
      }
      break;
    }
  }
};
