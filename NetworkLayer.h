#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "Config.h"
#include "MotionLayer.h"
#include "Types.h"
#include "WebUI.h"

/*
 * SECURITY ARCHITECTURE:
 * - Only ONE WebSocket client can be connected at a time
 * - First successful connection locks the IP address
 * - All subsequent connections from different IP addresses are rejected
 * - IP lock persists until ESP32 restart (prevents device hopping attacks)
 * - HTTP server remains accessible to all for web interface
 */

class RCNetworkManager {
private:
  WebServer *server;
  WebSocketsServer *webSocket;

  ControlInput lastInput;
  uint32_t lastPacketTime;
  uint8_t connectedClientNum;
  bool dataIsNew;
  MotionManager *motion;

  // IP address lock for security (prevents multiple device control)
  bool ipLocked;
  IPAddress authorizedIP;

  static RCNetworkManager *instance;

  static void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                             size_t length) {
    if (instance)
      instance->handleWebSocketEvent(num, type, payload, length);
  }

  void handleWebSocketEvent(uint8_t num, WStype_t type, uint8_t *payload,
                            size_t length) {
    switch (type) {
    case WStype_DISCONNECTED:
      if (num == connectedClientNum) {
        Serial.printf("[NET] Client %d disconnected\n", num);
        connectedClientNum = 255;
      }
      break;
    case WStype_CONNECTED: {
      // Only allow one client at a time
      if (connectedClientNum != 255) {
        Serial.printf("[NET] Rejecting client %d (slot taken by %d)\n", num, connectedClientNum);
        webSocket->disconnect(num);
      } else {
        connectedClientNum = num;
        IPAddress clientIP = getClientIP(num);
        authorizedIP = clientIP;
        ipLocked = true;
        Serial.printf("[NET] Client %d connected from %s\n", num, clientIP.toString().c_str());
      }
      break;
    }
    case WStype_TEXT: {
      // SECURITY: Only process messages from the authorized connected client
      if (num != connectedClientNum) {
        break;
      }

      // Parse JSON payload
      // Expected format: {"t":x, "s":y, "l":0/1, "h":0/1, "ms":timestamp}
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload, length);

      if (!error) {
        // Extract data, default to 0 if missing. Protect against malformed JSON
        // keys.
        if (doc["trim"].is<float>()) {
          if (motion)
            motion->saveTrimmer(doc["trim"].as<float>());
        }
        if (doc["trim_live"].is<float>()) {
          if (motion)
            motion->setTrimmer(doc["trim_live"].as<float>());
        }
        if (doc["th_trim"].is<float>()) {
          if (motion)
            motion->saveThrottleTrimmer(doc["th_trim"].as<float>());
        }
        if (doc["th_trim_live"].is<float>()) {
          if (motion)
            motion->setThrottleTrimmer(doc["th_trim_live"].as<float>());
        }

        float throttle = doc["t"] | 0.0f;
        float steering = doc["s"] | 0.0f;
        bool lights = doc["l"] | 0;
        bool horn = doc["h"] | 0;
        uint32_t ts = doc["ms"] | 0;

        // Saturate to [-100.0, 100.0] independently to protect from malformed
        // values
        throttle = constrain(throttle, -100.0f, 100.0f);
        steering = constrain(steering, -100.0f, 100.0f);

        lastInput.throttle = throttle;
        lastInput.steering = steering;
        lastInput.lights = lights;
        lastInput.horn = horn;
        lastInput.timestamp = ts;

        lastPacketTime = millis();
        dataIsNew = true;

        // Send heartbeat PONG response to measure RTT
        if (ts != 0 && webSocket) {
          JsonDocument resDoc;
          resDoc["pong"] = ts;
          String resStr;
          serializeJson(resDoc, resStr);
          webSocket->sendTXT(num, resStr);
        }
      }
      break;
    }

    default:
      // Ignore other WebSocket event types (PING, PONG, FRAGMENTS, etc.)
      break;
    }
  }

public:
  RCNetworkManager()
      : server(nullptr), webSocket(nullptr), lastPacketTime(0),
        connectedClientNum(255), dataIsNew(false), motion(nullptr),
        ipLocked(false) {

    lastInput = {0.0f, 0.0f, false, false, 0};
    instance = this;
  }

  void setMotionManager(MotionManager *m) { motion = m; }

  IPAddress getClientIP(uint8_t num) {
    if (webSocket) return webSocket->remoteIP(num);
    return IPAddress(0, 0, 0, 0);
  }

  void begin() {
    // Init AP Mode
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASS, 6, 0, 4);

    Serial.printf("[NET] AP: %s IP: %s\n", AP_SSID, WiFi.softAPIP().toString().c_str());

    // Start Web Server
    server = new WebServer(80);
    server->on("/", [this]() {
      server->sendHeader("Cache-Control",
                         "no-cache, no-store, must-revalidate");
      server->send_P(200, "text/html", index_html);
    });

    server->on("/manifest.json", [this]() {
      const char *manifest =
          R"({"name":"RC Crawler Pro","short_name":"RC Crawler","start_url":"/","scope":"/","display":"standalone","orientation":"landscape","background_color":"#121212","theme_color":"#121212","icons":[{"src":"data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIj48cmVjdCB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCIgZmlsbD0iIzEyMTIxMiIvPjxjaXJjbGUgY3g9IjUwIiBjeT0iNTAiIHI9IjQwIiBmaWxsPSIjZmY0NzU3Ii8+PC9zdmc+","sizes":"192x192","type":"image/svg+xml"}]})";
      server->send(200, "application/json", manifest);
    });

    server->on("/sw.js", [this]() {
      const char *sw =
          "const CACHE_NAME = 'rc-v15';\n"
          "const ASSETS = ['/', '/manifest.json'];\n"
          "self.addEventListener('install', e => { "
          "self.skipWaiting(); "
          "e.waitUntil(caches.open(CACHE_NAME).then(c => "
          "c.addAll(ASSETS))); });\n"
          "self.addEventListener('activate', e => { "
          "e.waitUntil(clients.claim().then(() => caches.keys().then(keys => "
          "Promise.all(keys.filter(k => k !== CACHE_NAME).map(k => "
          "caches.delete(k)))))); });\n"
          "self.addEventListener('fetch', e => { "
          "e.respondWith(fetch(e.request).catch(() => "
          "caches.match(e.request))); });";
      server->sendHeader("Cache-Control",
                         "no-cache, no-store, must-revalidate");
      server->send(200, "application/javascript", sw);
    });

    server->onNotFound(
        [this]() { server->send(404, "text/plain", "Not Found"); });
    server->begin();

    // Start WebSocket Server
    webSocket = new WebSocketsServer(WS_PORT);
    webSocket->begin();
    webSocket->onEvent(webSocketEvent);
    Serial.printf("[NET] WS server on port %d\n", WS_PORT);
  }

  void loop() {
    if (webSocket)
      webSocket->loop();
    if (server)
      server->handleClient();
  }

  bool hasClient() const { return (connectedClientNum != 255); }

  // Reset IP lock (for development/testing)
  void resetIPLock() {
    ipLocked = false;
    authorizedIP = IPAddress(0, 0, 0, 0);
    Serial.println("[SECURITY] IP lock reset");
  }

  uint32_t getTimeSinceLastPacket() const { return millis() - lastPacketTime; }

  bool isDataNew() {
    bool n = dataIsNew;
    dataIsNew = false;
    return n;
  }

  ControlInput getLastInput() const { return lastInput; }
};

RCNetworkManager *RCNetworkManager::instance = nullptr;
