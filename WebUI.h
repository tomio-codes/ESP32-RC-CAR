#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsServer.h>
#include <WiFi.h>

#include "Config.h"
#include "Types.h"

// HTML content generated from web_ui.html
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="cs">

<head>
    <meta charset="UTF-8">
    <meta name="viewport"
        content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <meta name="theme-color" content="#121212">
    <meta name="apple-mobile-web-app-capable" content="yes">
    <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
    <meta name="apple-mobile-web-app-title" content="RC Crawler">
    <link rel="apple-touch-icon"
        href="data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMDAgMTAwIj48cmVjdCB3aWR0aD0iMTAwIiBoZWlnaHQ9IjEwMCIgZmlsbD0iIzEyMTIxMiIvPjxjaXJjbGUgY3g9IjUwIiBjeT0iNTAiIHI9IjQwIiBmaWxsPSIjZmY0NzU3Ii8+PC9zdmc+">
    <link rel="manifest" href="/manifest.json">
    <title>RC Crawler Pro</title>
    <style>
        :root {
            --bg-color: #0f0f12;
            --panel-bg: rgba(30, 30, 35, 0.7);
            --accent: #ff4757;
            --joy-bg: #1a1a1e;
            --text-light: #f1f2f6;
            --text-dim: #747d8c;
            --glass-border: rgba(255, 255, 255, 0.1);
            --safe-top: env(safe-area-inset-top, 0px);
            --safe-bottom: env(safe-area-inset-bottom, 0px);
            --safe-left: env(safe-area-inset-left, 0px);
            --safe-right: env(safe-area-inset-right, 0px);
            --app-pad: clamp(6px, 1.5vw, 20px);
            --app-gap: clamp(8px, 2vw, 20px);
            --panel-w: clamp(90px, 18vw, 180px);
            --joy-size: min(
                calc(100dvh - var(--safe-top) - var(--safe-bottom) - var(--app-pad) * 2 - 30px),
                calc((100vw - var(--safe-left) - var(--safe-right) - var(--panel-w) - var(--app-gap) * 2 - var(--app-pad) * 2) / 2)
            );
        }

        * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; -webkit-user-select: none; }

        html, body {
            height: 100%; width: 100%; overflow: hidden;
            overscroll-behavior: none;
        }

        body {
            background-color: var(--bg-color);
            background-image: radial-gradient(circle at 50% 50%, #1e1e24 0%, #0f0f12 100%);
            color: var(--text-light);
            font-family: 'Inter', 'Segoe UI', system-ui, sans-serif;
            display: flex; flex-direction: column;
        }

        #app {
            display: flex;
            flex: 1;
            justify-content: center;
            align-items: center;
            padding: var(--app-pad);
            padding-top: calc(var(--app-pad) + var(--safe-top) + 28px);
            padding-left: calc(var(--app-pad) + var(--safe-left));
            padding-right: calc(var(--app-pad) + var(--safe-right));
            padding-bottom: calc(var(--app-pad) + var(--safe-bottom));
            gap: var(--app-gap);
        }

        .joystick-container {
            width: var(--joy-size);
            height: var(--joy-size);
            min-width: 100px; min-height: 100px;
            flex-shrink: 0;
            aspect-ratio: 1/1;
            background: var(--joy-bg);
            border-radius: 50%;
            position: relative;
            box-shadow:
                inset 0 0 30px rgba(0, 0, 0, 0.8),
                0 0 0 3px rgba(255, 255, 255, 0.03),
                0 8px 16px rgba(0, 0, 0, 0.5);
            display: flex; justify-content: center; align-items: center;
            touch-action: none; overflow: hidden;
        }

        .joystick-container::after {
            content: ''; position: absolute; inset: 0; border-radius: 50%;
            pointer-events: none;
            box-shadow: inset 0 2px 10px rgba(255, 255, 255, 0.05);
        }

        canvas {
            border-radius: 50%; position: absolute; inset: 0;
            width: 100%; height: 100%;
            cursor: pointer; touch-action: none; z-index: 2;
        }

        .center-panel {
            flex: 0 0 auto;
            width: var(--panel-w);
            display: flex; flex-direction: column;
            align-items: center; justify-content: center;
            gap: clamp(4px, 1vh, 8px);
            background: var(--panel-bg);
            backdrop-filter: blur(15px); -webkit-backdrop-filter: blur(15px);
            padding: clamp(6px, 1.5vh, 12px);
            border-radius: clamp(12px, 2vw, 20px);
            border: 1px solid var(--glass-border);
            box-shadow: 0 10px 25px rgba(0, 0, 0, 0.4);
        }

        .status-bar {
            display: flex; flex-direction: column; align-items: center;
            gap: 3px; width: 100%;
            font-size: clamp(0.55rem, 1.8vw, 0.85rem);
            color: var(--text-dim);
            margin-bottom: clamp(2px, 0.5vh, 5px);
            font-weight: 600; text-transform: uppercase; letter-spacing: 0.5px;
        }

        .status-val {
            color: var(--text-light);
            text-shadow: 0 0 8px rgba(255, 255, 255, 0.2);
        }

        .btn-group {
            display: flex; flex-direction: column;
            gap: clamp(4px, 1vh, 8px);
            width: 100%;
        }

        button {
            background: rgba(255, 255, 255, 0.07);
            color: var(--text-light);
            border: 1px solid var(--glass-border);
            border-radius: clamp(8px, 1.5vw, 12px);
            padding: clamp(4px, 1vh, 8px) 0;
            font-size: clamp(0.55rem, 1.5vw, 0.75rem);
            font-weight: 700; cursor: pointer;
            transition: all 0.2s cubic-bezier(0.4, 0, 0.2, 1);
            width: 100%; text-transform: uppercase;
            letter-spacing: 0.5px; touch-action: manipulation;
        }

        button:active { background: rgba(255, 255, 255, 0.15); transform: translateY(1px); }

        button.active {
            background: #2ed573; border-color: #7bed9f; color: #000;
            box-shadow: 0 0 15px rgba(46, 213, 115, 0.4);
        }

        .conn-status {
            position: fixed;
            top: calc(var(--safe-top) + 6px);
            left: 50%; transform: translateX(-50%);
            padding: clamp(3px, 0.8vh, 6px) clamp(10px, 2vw, 18px);
            border-radius: 30px;
            font-size: clamp(0.55rem, 1.3vw, 0.75rem);
            background: #ff4757; color: white;
            font-weight: 800; letter-spacing: 1px; text-transform: uppercase;
            box-shadow: 0 3px 12px rgba(255, 71, 87, 0.3);
            z-index: 100; animation: pulse-red 2s infinite;
            white-space: nowrap;
        }

        .conn-status.connected {
            background: #2ed573;
            box-shadow: 0 3px 12px rgba(46, 213, 115, 0.3);
            animation: none;
        }

        @keyframes pulse-red {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.6; }
        }

        .label {
            position: absolute; top: 15%; left: 50%; transform: translateX(-50%);
            color: rgba(255, 255, 255, 0.15);
            font-weight: 900;
            font-size: clamp(0.45rem, 1.2vw, 0.7rem);
            letter-spacing: clamp(1px, 0.4vw, 3px);
            text-transform: uppercase; pointer-events: none;
        }

        .modal-overlay {
            position: fixed; inset: 0;
            background: rgba(0, 0, 0, 0.8); backdrop-filter: blur(8px);
            z-index: 1000; display: none;
            align-items: center; justify-content: center;
        }

        .modal-content {
            background: #1e1e24; border: 1px solid var(--glass-border);
            padding: clamp(16px, 3vw, 24px);
            border-radius: clamp(16px, 3vw, 24px);
            width: min(90%, 320px);
            box-shadow: 0 20px 50px rgba(0, 0, 0, 0.6);
        }

        .modal-header {
            font-size: clamp(0.85rem, 2.5vw, 1.1rem);
            font-weight: 800; margin-bottom: clamp(14px, 3vh, 24px);
            text-align: center; color: var(--accent);
            letter-spacing: 2px; text-transform: uppercase;
        }

        .settings-item { margin-bottom: clamp(14px, 3vh, 24px); }

        .settings-label {
            display: flex; justify-content: space-between;
            margin-bottom: 10px; font-weight: 700;
            font-size: clamp(0.65rem, 1.8vw, 0.8rem);
            color: var(--text-dim); text-transform: uppercase; letter-spacing: 1px;
        }

        input[type="range"] {
            width: 100%; height: 8px;
            background: rgba(255, 255, 255, 0.05);
            border-radius: 10px; appearance: none; outline: none;
        }

        input[type="range"]::-webkit-slider-thumb {
            appearance: none; width: 28px; height: 28px;
            background: var(--accent); border-radius: 50%; cursor: pointer;
            box-shadow: 0 0 15px rgba(255, 71, 87, 0.5);
            border: 3px solid #1e1e24;
        }

        #btnSave { background: #2ed573; color: #000; margin-top: 8px; }
        #btnCancel { background: rgba(255, 255, 255, 0.05); color: var(--text-dim); margin-top: 6px; }

        .app-version {
            font-size: clamp(0.4rem, 1vw, 0.6rem);
            color: #444; margin-top: clamp(2px, 0.5vh, 4px);
        }
    </style>
</head>

<body>

    <div class="conn-status" id="connIndicator">DISCONNECTED</div>
    <div id="app">
        <div class="joystick-container" id="leftJoy">
            <div class="label">THROTTLE</div>
            <canvas id="canvasL" width="320" height="320"></canvas>
        </div>

        <div class="center-panel">
            <div class="status-bar">
                <div>THR: <span class="status-val" id="valThr">0</span> %</div>
                <div>STR: <span class="status-val" id="valStr">0</span> %</div>
            </div>
            <div class="btn-group">
                <button id="btnLights" type="button">LIGHTS</button>
                <button id="btnHorn" type="button">HORN</button>
                <button id="btnSettings" type="button">SETTINGS</button>
            </div>
            <div class="app-version">App: v15 | host: <span id="debugHost">--</span></div>
        </div>

        <div class="joystick-container" id="rightJoy">
            <div class="label">STEERING</div>
            <canvas id="canvasR" width="320" height="320"></canvas>
        </div>
    </div>

    <!-- Settings Modal -->
    <div class="modal-overlay" id="settingsModal">
        <div class="modal-content">
            <div class="modal-header">Settings</div>
            <div class="settings-item">
                <label style="color:var(--text-dim); font-size:0.8rem; margin-top:10px; display:block;">Throttle Center (Trimmer)</label>
                <input type="range" id="trimThr" min="-10" max="10" step="0.5" value="0" style="width:100%; margin:10px 0;">
                <div style="display:flex; justify-content:space-between; font-size:0.7rem; color:var(--text-dim);">
                    <span>-10</span><span id="trimThrVal">0</span><span>10</span>
                </div>

                <div style="margin-top:20px; display:flex; gap:10px;">
                    <span>Steering Trim</span>
                    <span id="trimVal">0</span>
                </div>
                <input type="range" id="trimSlider" min="-20" max="20" step="1" value="0">
            </div>
            <button id="btnSave" type="button">SAVE SETTINGS</button>
            <button id="btnCancel" type="button">BACK</button>
        </div>
    </div>

    <script>
        // Simple WS URL for ESP32 AP mode (always 192.168.4.1)
        function getWsUrl() {
            let host = window.location.hostname;
            document.getElementById('debugHost').textContent = host || 'null';
            // ESP32 AP mode always uses 192.168.4.1
            return `ws://192.168.4.1:81/`;
        }

        let ws = null;
        let isConnected = false;
        let sendInterval = null; // Store interval ID to control send loop

        let state = { t: 0, s: 0, l: 0, h: 0, ms: 0 };
        let lastPing = 0;

        // UI Elements
        const valThr = document.getElementById('valThr');
        const valStr = document.getElementById('valStr');
        const valPing = document.getElementById('valPing');
        const connInd = document.getElementById('connIndicator');
        const btnLights = document.getElementById('btnLights'), btnHorn = document.getElementById('btnHorn');
        const btnSettings = document.getElementById('btnSettings'), settingsModal = document.getElementById('settingsModal');
        const btnSave = document.getElementById('btnSave'), btnCancel = document.getElementById('btnCancel');
        const trimSlider = document.getElementById('trimSlider'), trimVal = document.getElementById('trimVal');
        const trimThr = document.getElementById('trimThr'), trimThrVal = document.getElementById('trimThrVal');

        // Settings Modal Logic
        function openSettings() {
            settingsModal.style.display = 'flex';
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ get_settings: true }));
            }
        }

        function closeSettings() {
            settingsModal.style.display = 'none';
        }

        btnSettings.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            openSettings();
        });

        btnCancel.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            closeSettings();
        });

        trimSlider.oninput = () => { trimVal.textContent = trimSlider.value; if (isConnected) ws.send(JSON.stringify({ trim_live: parseFloat(trimSlider.value) })); };
        trimThr.oninput = () => { trimThrVal.textContent = trimThr.value; if (isConnected) ws.send(JSON.stringify({ th_trim_live: parseFloat(trimThr.value) })); };

        function saveSettings() {
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ trim: parseFloat(trimSlider.value), th_trim: parseFloat(trimThr.value) }));
            }
            closeSettings();
        }

        btnSave.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            saveSettings();
        });

        // Robust Toggle Latch for Lights
        btnLights.addEventListener('pointerdown', (e) => {
            e.preventDefault();
            state.l = state.l ? 0 : 1;
            btnLights.classList.toggle('active', state.l === 1);
        });

        const startH = (e) => { e.preventDefault(); state.h = 1; btnHorn.classList.add('active'); };
        const stopHorn = (e) => { e.preventDefault(); state.h = 0; btnHorn.classList.remove('active'); };
        btnHorn.addEventListener('mousedown', startH);
        btnHorn.addEventListener('touchstart', startH, { passive: false });
        window.addEventListener('mouseup', stopHorn);
        window.addEventListener('touchend', stopHorn);

        function initWebSocket() {
            const url = getWsUrl();

            // Close previous connection if any
            if (ws) {
                ws.onopen = null;
                ws.onclose = null;
                ws.onerror = null;
                ws.onmessage = null;
                if (ws.readyState === WebSocket.OPEN || ws.readyState === WebSocket.CONNECTING) {
                    ws.close();
                }
                ws = null;
            }

            ws = new WebSocket(url);

            ws.onopen = () => {
                isConnected = true;
                connInd.textContent = 'CONNECTED';
                connInd.classList.add('connected');
                window.dcCount = 0;
            };

            ws.onclose = (e) => {
                isConnected = false;
                connInd.textContent = 'DISCONNECTED';
                connInd.classList.remove('connected');

                if (!window.dcCount) window.dcCount = 0;
                window.dcCount++;

                if (window.dcCount > 10) {
                    window.location.reload(true);
                } else {
                    setTimeout(initWebSocket, 1500);
                }
            };

            ws.onerror = () => {};

            ws.onmessage = (e) => {
                try {
                    const data = JSON.parse(e.data);
                    if (data.pong) {
                        const rtt = Date.now() - data.pong;
                        if (valPing) valPing.textContent = rtt;
                    }
                } catch (err) { }
            };
        }

        // Single send loop - always running, checks connection before sending
        setInterval(() => {
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                state.ms = Date.now();
                ws.send(JSON.stringify(state));
            }
        }, 50);

        // Safety: Handle app backgrounding/focus loss
        const forceNeutral = () => {
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ t: 0, s: 0, l: state.l, h: 0, ms: Date.now() }));
            }
            state.t = 0; state.s = 0;
            valThr.textContent = "0"; valStr.textContent = "0";
            if (typeof joyL !== 'undefined') joyL.rst();
            if (typeof joyR !== 'undefined') joyR.rst();
        };

        window.addEventListener('blur', forceNeutral);
        document.addEventListener('visibilitychange', () => {
            if (document.hidden) forceNeutral();
        });

        class Joystick {
            constructor(id, type) {
                this.c = document.getElementById(id); this.ctx = this.c.getContext('2d');
                this.type = type; this.act = false; this.id = null;
                this.resize();
                window.addEventListener('resize', () => this.resize());
                this.bind(); this.draw();
            }
            resize() {
                const rect = this.c.parentElement.getBoundingClientRect();
                this.c.width = rect.width; this.c.height = rect.height;
                this.r = this.c.width / 2; this.cx = this.r; this.cy = this.r;
                this.kr = Math.max(18, Math.min(40, this.r * 0.22));
                this.draw();
            }
            bind() {
                const start = (e) => {
                    e.preventDefault(); this.act = true; this.upd(e.changedTouches ? e.changedTouches[0] : e);
                    if (e.changedTouches) this.id = e.changedTouches[0].identifier;
                };
                const move = (e) => {
                    if (!this.act) return; e.preventDefault();
                    if (e.changedTouches) {
                        for (let i = 0; i < e.changedTouches.length; i++) {
                            if (e.changedTouches[i].identifier === this.id) { this.upd(e.changedTouches[i]); break; }
                        }
                    } else { this.upd(e); }
                };
                const end = (e) => {
                    if (!this.act) return; if (e.type !== 'touchcancel') e.preventDefault();
                    if (e.changedTouches) {
                        let f = false; for (let i = 0; i < e.changedTouches.length; i++) { if (e.changedTouches[i].identifier === this.id) f = true; }
                        if (!f) return;
                    }
                    this.act = false; this.id = null; this.rst();
                };
                this.c.addEventListener('mousedown', start); window.addEventListener('mousemove', move); window.addEventListener('mouseup', end);
                this.c.addEventListener('touchstart', start, { passive: false }); window.addEventListener('touchmove', move, { passive: false });
                window.addEventListener('touchend', end, { passive: false }); window.addEventListener('touchcancel', end, { passive: false });
            }
            upd(e) {
                const rect = this.c.getBoundingClientRect();
                let mx = e.clientX - rect.left - this.cx, my = e.clientY - rect.top - this.cy;
                if (this.type === 'v') mx = 0; if (this.type === 'h') my = 0;

                // Absolute maximum travel: 2px margin from edge
                const md = this.r - this.kr - 2;

                if (this.type === 'v') { if (my < -md) my = -md; if (my > md) my = md; }
                else if (this.type === 'h') { if (mx < -md) mx = -md; if (mx > md) mx = md; }
                if (this.type === 'v') { let p = Math.round(-(my / md) * 100); state.t = p; valThr.textContent = p; }
                if (this.type === 'h') { let p = Math.round((mx / md) * 100); state.s = p; valStr.textContent = p; }
                this.draw(mx, my);
            }
            rst() { if (this.type === 'v') { state.t = 0; valThr.textContent = "0"; } if (this.type === 'h') { state.s = 0; valStr.textContent = "0"; } this.draw(0, 0); }
            draw(ox = 0, oy = 0) {
                this.ctx.clearRect(0, 0, this.c.width, this.c.height);

                // Track path: use 2px margin for maximum visual travel guide
                this.ctx.beginPath();
                this.ctx.lineWidth = 12;
                this.ctx.lineCap = "round";
                this.ctx.strokeStyle = "rgba(0,0,0,0.3)";
                if (this.type === 'v') {
                    this.ctx.moveTo(this.cx, this.kr + 2);
                    this.ctx.lineTo(this.cx, this.c.height - (this.kr + 2));
                } else {
                    this.ctx.moveTo(this.kr + 2, this.cy);
                    this.ctx.lineTo(this.c.width - (this.kr + 2), this.cy);
                }
                this.ctx.stroke();

                // 3D Knob
                const kx = this.cx + ox;
                const ky = this.cy + oy;

                this.ctx.save();
                this.ctx.beginPath();
                this.ctx.arc(kx, ky, this.kr, 0, Math.PI * 2);

                // Shadow for depth
                this.ctx.shadowBlur = 15;
                this.ctx.shadowColor = "rgba(0,0,0,0.6)";
                this.ctx.shadowOffsetY = 5;

                // Spherical Gradient
                const grad = this.ctx.createRadialGradient(
                    kx - this.kr * 0.3, ky - this.kr * 0.3, this.kr * 0.1,
                    kx, ky, this.kr
                );
                const baseColor = this.act ? "#ff6b81" : "#ff4757";
                grad.addColorStop(0, "#ff9f9f");
                grad.addColorStop(0.5, baseColor);
                grad.addColorStop(1, "#8b0000");

                this.ctx.fillStyle = grad;
                this.ctx.fill();

                // Glossy reflection
                this.ctx.beginPath();
                this.ctx.ellipse(kx - this.kr * 0.3, ky - this.kr * 0.4, this.kr * 0.4, this.kr * 0.2, Math.PI * 0.8, 0, Math.PI * 2);
                this.ctx.fillStyle = "rgba(255,255,255,0.2)";
                this.ctx.fill();

                this.ctx.restore();
            }
        }
        const joyL = new Joystick('canvasL', 'v'); const joyR = new Joystick('canvasR', 'h');

        initWebSocket();

        // Register Service Worker
        if ('serviceWorker' in navigator) {
            window.addEventListener('load', () => {
                navigator.serviceWorker.register('/sw.js').catch(err => console.log('SW registration failed:', err));
            });
        }
    </script>
</body>

</html>
)rawliteral";
