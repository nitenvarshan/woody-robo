#pragma once

#include <Arduino.h>

// ----------------------------------------------------------------------------
//  WOODY ROBOT — EMBEDDED WEB DASHBOARD (HTML5 / CSS / JAVASCRIPT)
//  Features:
//    - Modern App UI with Fixed Bottom Navigation Bar
//    - Tab 1: Robot Controls (Arm sliders, presets, faces, calibration)
//    - Tab 2: Live Chip Diagnostics & Telemetry (Temp, RAM, CPU, Flash, Uptime)
//    - Auto-polling REST API (/api/telemetry)
// ----------------------------------------------------------------------------

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Woody Robot Control & Telemetry</title>
  <style>
    :root {
      --bg: #0d1117;
      --card-bg: #161b22;
      --card-border: #30363d;
      --text: #f0f6fc;
      --text-muted: #8b949e;
      --accent-cyan: #58a6ff;
      --accent-green: #3fb950;
      --accent-orange: #d29922;
      --accent-red: #f85149;
      --accent-purple: #bc8cff;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; -webkit-tap-highlight-color: transparent; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
      background: var(--bg);
      color: var(--text);
      padding-bottom: 75px;
      min-height: 100vh;
    }
    header {
      background: #161b22;
      border-bottom: 1px solid var(--card-border);
      padding: 12px 16px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      position: sticky;
      top: 0;
      z-index: 100;
    }
    .brand { font-size: 1.1rem; font-weight: 700; color: var(--accent-cyan); display: flex; align-items: center; gap: 8px; }
    .status-pill {
      font-size: 0.75rem;
      padding: 4px 10px;
      border-radius: 12px;
      background: rgba(63, 185, 80, 0.15);
      color: var(--accent-green);
      font-weight: 600;
      border: 1px solid rgba(63, 185, 80, 0.3);
    }
    .content-tab { display: none; padding: 16px; max-width: 600px; margin: 0 auto; }
    .content-tab.active { display: block; animation: fadeIn 0.2s ease-in-out; }
    @keyframes fadeIn { from { opacity: 0; transform: translateY(4px); } to { opacity: 1; transform: translateY(0); } }

    .card {
      background: var(--card-bg);
      border: 1px solid var(--card-border);
      border-radius: 10px;
      padding: 16px;
      margin-bottom: 16px;
    }
    .card-title {
      font-size: 0.85rem;
      text-transform: uppercase;
      letter-spacing: 0.5px;
      color: var(--text-muted);
      margin-bottom: 12px;
      display: flex;
      justify-content: space-between;
      align-items: center;
    }

    /* Sliders */
    .slider-group { margin-bottom: 14px; }
    .slider-header { display: flex; justify-content: space-between; font-size: 0.9rem; margin-bottom: 6px; }
    input[type=range] {
      width: 100%;
      height: 6px;
      border-radius: 3px;
      background: var(--card-border);
      outline: none;
      -webkit-appearance: none;
    }
    input[type=range]::-webkit-slider-thumb {
      -webkit-appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--accent-cyan);
      cursor: pointer;
    }

    /* Action Buttons Grid */
    .btn-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 10px; }
    .btn-grid-3 { display: grid; grid-template-columns: repeat(3, 1fr); gap: 8px; }
    button {
      background: #21262d;
      border: 1px solid var(--card-border);
      color: var(--text);
      padding: 12px 10px;
      font-size: 0.9rem;
      font-weight: 600;
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.15s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      gap: 6px;
    }
    button:active { transform: scale(0.97); background: #30363d; }
    button.btn-primary { background: rgba(88, 166, 255, 0.15); border-color: rgba(88, 166, 255, 0.4); color: var(--accent-cyan); }
    button.btn-orange { background: rgba(210, 153, 34, 0.15); border-color: rgba(210, 153, 34, 0.4); color: var(--accent-orange); }
    button.btn-red { background: rgba(248, 81, 73, 0.15); border-color: rgba(248, 81, 73, 0.4); color: var(--accent-red); }

    /* Telemetry Gauges */
    .telemetry-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 12px; margin-bottom: 16px; }
    .stat-card {
      background: #21262d;
      border: 1px solid var(--card-border);
      border-radius: 8px;
      padding: 12px;
    }
    .stat-label { font-size: 0.75rem; color: var(--text-muted); margin-bottom: 4px; }
    .stat-value { font-size: 1.3rem; font-weight: 700; color: var(--accent-cyan); }
    .stat-sub { font-size: 0.7rem; color: var(--text-muted); margin-top: 4px; }

    /* Progress bar */
    .bar-bg { width: 100%; height: 8px; background: #30363d; border-radius: 4px; overflow: hidden; margin-top: 6px; }
    .bar-fill { height: 100%; background: var(--accent-cyan); border-radius: 4px; transition: width 0.3s ease; }
    .bar-fill.warning { background: var(--accent-orange); }
    .bar-fill.danger { background: var(--accent-red); }

    /* Key-Value List */
    .kv-row { display: flex; justify-content: space-between; padding: 8px 0; border-bottom: 1px solid #21262d; font-size: 0.85rem; }
    .kv-row:last-child { border-bottom: none; }
    .kv-key { color: var(--text-muted); }
    .kv-val { font-weight: 600; font-family: monospace; }

    /* Bottom Navigation Bar */
    nav.bottom-bar {
      position: fixed;
      bottom: 0;
      left: 0;
      right: 0;
      height: 60px;
      background: #161b22;
      border-top: 1px solid var(--card-border);
      display: flex;
      justify-content: space-around;
      align-items: center;
      z-index: 1000;
    }
    .nav-btn {
      flex: 1;
      height: 100%;
      background: none;
      border: none;
      color: var(--text-muted);
      font-size: 0.75rem;
      flex-direction: column;
      gap: 3px;
      border-radius: 0;
    }
    .nav-btn.active { color: var(--accent-cyan); font-weight: 700; }
    .nav-icon { font-size: 1.2rem; }
  </style>
</head>
<body>

  <!-- Header -->
  <header>
    <div class="brand">🤖 Woody Companion</div>
    <div class="status-pill" id="live-indicator">LIVE</div>
  </header>

  <!-- TAB 1: CONTROLS -->
  <div id="tab-controls" class="content-tab active">
    
    <!-- Arm Sliders -->
    <div class="card">
      <div class="card-title">Arm Angle Controls</div>
      
      <div class="slider-group">
        <div class="slider-header">
          <span>Both Arms (Sync)</span>
          <span id="sync-val" style="color:var(--accent-cyan);font-weight:700;">25°</span>
        </div>
        <input type="range" id="sync-slider" min="0" max="180" value="25" oninput="onSyncSlide(this.value)">
      </div>

      <div class="slider-group">
        <div class="slider-header">
          <span>Left Arm Only</span>
          <span id="left-val">25°</span>
        </div>
        <input type="range" id="left-slider" min="0" max="180" value="25" oninput="onArmSlide('L', this.value)">
      </div>

      <div class="slider-group" style="margin-bottom:0;">
        <div class="slider-header">
          <span>Right Arm Only</span>
          <span id="right-val">25°</span>
        </div>
        <input type="range" id="right-slider" min="0" max="180" value="25" oninput="onArmSlide('R', this.value)">
      </div>
    </div>

    <!-- Quick Gestures -->
    <div class="card">
      <div class="card-title">Preset Gestures</div>
      <div class="btn-grid">
        <button class="btn-primary" onclick="sendCmd('U')">🙌 Arms UP</button>
        <button class="btn-primary" onclick="sendCmd('D')">👇 Arms DOWN</button>
        <button onclick="sendCmd('W')">👋 Wave</button>
        <button onclick="sendCmd('S')">🛑 Emergency STOP</button>
      </div>
    </div>

    <!-- Moods & Faces -->
    <div class="card">
      <div class="card-title">Faces & Expressions</div>
      <div class="btn-grid-3">
        <button onclick="sendCmd('G')">😎 Chad</button>
        <button onclick="sendCmd('H')">❤️❤️ Hearts</button>
        <button onclick="sendCmd('K')">😉 Wink</button>
        <button onclick="sendCmd('1')">🟩 Creeper</button>
        <button onclick="sendCmd('2')">👻 Hero</button>
        <button class="btn-primary" onclick="sendCmd('3')">🎬 Story Loop</button>
      </div>
    </div>

    <!-- Calibration & Alignment -->
    <div class="card">
      <div class="card-title">Servo Hardware Setup</div>
      <div class="btn-grid">
        <button class="btn-orange" onclick="sendCmd('C')">🎯 90° Calibrate</button>
        <button class="btn-orange" onclick="sendCmd('M')">🔄 Toggle Mirror</button>
      </div>
      <button style="width:100%;margin-top:10px;" onclick="sendCmd('TEST')">🔬 Run Diagnostic Sweep</button>
    </div>

  </div>

  <!-- TAB 2: CHIP CONDITION & TELEMETRY -->
  <div id="tab-telemetry" class="content-tab">
    
    <!-- 4 High-Level Stat Cards -->
    <div class="telemetry-grid">
      <div class="stat-card">
        <div class="stat-label">🌡️ Chip Core Temp</div>
        <div class="stat-value" id="t-temp">--.- °C</div>
        <div class="stat-sub" id="t-temp-sub">--.- °F</div>
      </div>

      <div class="stat-card">
        <div class="stat-label">⏱️ Running Time</div>
        <div class="stat-value" id="t-uptime" style="font-size:1.05rem;">00:00:00</div>
        <div class="stat-sub">Uptime since boot</div>
      </div>

      <div class="stat-card">
        <div class="stat-label">🧠 Free RAM (Heap)</div>
        <div class="stat-value" id="t-ram">-- KB</div>
        <div class="bar-bg"><div class="bar-fill" id="t-ram-bar" style="width: 0%;"></div></div>
        <div class="stat-sub" id="t-ram-sub">--% used</div>
      </div>

      <div class="stat-card">
        <div class="stat-label">💾 Flash Storage</div>
        <div class="stat-value" id="t-flash">-- MB</div>
        <div class="bar-bg"><div class="bar-fill" id="t-flash-bar" style="width: 0%;"></div></div>
        <div class="stat-sub" id="t-flash-sub">--% sketch</div>
      </div>
    </div>

    <!-- Processor & Silicon Card -->
    <div class="card">
      <div class="card-title">Processor Silicon</div>
      <div class="kv-row">
        <span class="kv-key">CPU Frequency</span>
        <span class="kv-val" id="t-cpu">240 MHz</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">CPU Cores</span>
        <span class="kv-val" id="t-cores">2 Cores (Xtensa LX6)</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Silicon Model & Rev</span>
        <span class="kv-val" id="t-chip">ESP32 DevKit V1</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Min Free RAM Ever</span>
        <span class="kv-val" id="t-min-heap">-- KB</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Max Allocatable Block</span>
        <span class="kv-val" id="t-max-block">-- KB</span>
      </div>
    </div>

    <!-- Robot Subsystems State -->
    <div class="card">
      <div class="card-title">Robot Subsystems State</div>
      <div class="kv-row">
        <span class="kv-key">Left Arm Coordinate</span>
        <span class="kv-val" id="t-sub-left">25°</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Right Arm Coordinate</span>
        <span class="kv-val" id="t-sub-right">155°</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Servo Mirroring</span>
        <span class="kv-val" id="t-sub-mirror" style="color:var(--accent-green);">ENABLED</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Active Face / Mood</span>
        <span class="kv-val" id="t-sub-mood">ORGANIC / NORMAL</span>
      </div>
      <div class="kv-row">
        <span class="kv-key">Shoulder IR Sensor</span>
        <span class="kv-val" id="t-sub-ir">CLEAR (0 hits)</span>
      </div>
    </div>

    <div style="text-align:center;font-size:0.75rem;color:var(--text-muted);padding:8px 0;">
      Telemetry updates live every 1.5 seconds.
    </div>

  </div>

  <!-- Bottom App Navigation Bar -->
  <nav class="bottom-bar">
    <button class="nav-btn active" id="btn-tab-controls" onclick="switchTab('controls')">
      <div class="nav-icon">🎮</div>
      <div>Controls</div>
    </button>
    <button class="nav-btn" id="btn-tab-telemetry" onclick="switchTab('telemetry')">
      <div class="nav-icon">📊</div>
      <div>Chip & System</div>
    </button>
  </nav>

  <script>
    // Tab switching
    function switchTab(tabName) {
      document.querySelectorAll('.content-tab').forEach(el => el.classList.remove('active'));
      document.querySelectorAll('.nav-btn').forEach(el => el.classList.remove('active'));

      if (tabName === 'controls') {
        document.getElementById('tab-controls').classList.add('active');
        document.getElementById('btn-tab-controls').classList.add('active');
      } else {
        document.getElementById('tab-telemetry').classList.add('active');
        document.getElementById('btn-tab-telemetry').classList.add('active');
        fetchTelemetry(); // Instant refresh on tab view
      }
    }

    // Send Single Action Command
    function sendCmd(cmd) {
      fetch('/action?cmd=' + encodeURIComponent(cmd))
        .catch(err => console.error(err));
    }

    // Sliders
    let slideTimer = null;
    function onSyncSlide(val) {
      document.getElementById('sync-val').innerText = val + '°';
      document.getElementById('left-val').innerText = val + '°';
      document.getElementById('right-val').innerText = val + '°';
      clearTimeout(slideTimer);
      slideTimer = setTimeout(() => sendCmd(val), 40);
    }

    function onArmSlide(arm, val) {
      document.getElementById(arm === 'L' ? 'left-val' : 'right-val').innerText = val + '°';
      clearTimeout(slideTimer);
      slideTimer = setTimeout(() => sendCmd(arm + ' ' + val), 40);
    }

    // Live Telemetry Poller
    function fetchTelemetry() {
      fetch('/api/telemetry')
        .then(res => res.json())
        .then(data => {
          // Temperature
          const tempEl = document.getElementById('t-temp');
          tempEl.innerText = data.temp_c.toFixed(1) + ' °C';
          document.getElementById('t-temp-sub').innerText = data.temp_f.toFixed(1) + ' °F';
          if (data.temp_c > 65) tempEl.style.color = 'var(--accent-red)';
          else if (data.temp_c > 52) tempEl.style.color = 'var(--accent-orange)';
          else tempEl.style.color = 'var(--accent-green)';

          // Uptime
          document.getElementById('t-uptime').innerText = data.uptime;

          // RAM
          const freeKb = (data.free_ram / 1024).toFixed(0);
          const totalKb = (data.total_ram / 1024).toFixed(0);
          document.getElementById('t-ram').innerText = freeKb + ' KB';
          document.getElementById('t-ram-sub').innerText = data.ram_pct.toFixed(0) + '% used of ' + totalKb + ' KB';
          const ramBar = document.getElementById('t-ram-bar');
          ramBar.style.width = data.ram_pct + '%';
          ramBar.className = 'bar-fill' + (data.ram_pct > 80 ? ' danger' : (data.ram_pct > 60 ? ' warning' : ''));

          // Flash
          const flashMb = (data.flash_size / (1024 * 1024)).toFixed(1);
          const sketchMb = (data.sketch_size / (1024 * 1024)).toFixed(2);
          document.getElementById('t-flash').innerText = flashMb + ' MB';
          document.getElementById('t-flash-sub').innerText = sketchMb + ' MB used (' + data.flash_pct.toFixed(0) + '%)';
          document.getElementById('t-flash-bar').style.width = data.flash_pct + '%';

          // Silicon
          document.getElementById('t-cpu').innerText = data.cpu_freq + ' MHz';
          document.getElementById('t-cores').innerText = data.chip_cores + ' Cores (' + data.chip_model + ')';
          document.getElementById('t-chip').innerText = data.chip_model + ' (Rev ' + data.chip_rev + ')';
          document.getElementById('t-min-heap').innerText = (data.min_free_ram / 1024).toFixed(1) + ' KB';
          document.getElementById('t-max-block').innerText = (data.max_alloc_ram / 1024).toFixed(1) + ' KB';

          // Subsystems
          document.getElementById('t-sub-left').innerText = data.left_angle + '°';
          document.getElementById('t-sub-right').innerText = data.right_angle + '°';
          const mirrorEl = document.getElementById('t-sub-mirror');
          mirrorEl.innerText = data.mirrored ? 'ENABLED (Back-to-Back)' : 'DISABLED (Direct)';
          mirrorEl.style.color = data.mirrored ? 'var(--accent-green)' : 'var(--accent-orange)';

          document.getElementById('t-sub-mood').innerText = data.face + ' / ' + data.mood;
          document.getElementById('t-sub-ir').innerText = (data.ir_detected ? '🚨 DETECTED' : 'CLEAR') + ' (' + data.ir_count + ' triggers)';
        })
        .catch(err => console.warn('Telemetry poll error', err));
    }

    // Auto-poll telemetry every 1.5s
    setInterval(fetchTelemetry, 1500);
  </script>
</body>
</html>
)rawliteral";

