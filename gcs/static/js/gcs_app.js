/**
 * Web 3D Drone GCS — Main Application Controller & Audio Telemetry
 */

let drone3D = null;
let flightHUD = null;
let mapPlanner = null;
let oscChart = null;
let ws = null;
let activeTelemetry = null;

// Audio Synthesizer (Zero-dependency Web Audio API)
let audioCtx = null;
function getAudioContext() {
  if (!audioCtx) {
    audioCtx = new (window.AudioContext || window.webkitAudioContext)();
  }
  if (audioCtx.state === "suspended") {
    audioCtx.resume();
  }
  return audioCtx;
}

function playBeep(freq = 880, duration = 0.15, type = "sine") {
  try {
    const ctx = getAudioContext();
    const osc = ctx.createOscillator();
    const gain = ctx.createGain();
    osc.type = type;
    osc.frequency.setValueAtTime(freq, ctx.currentTime);
    gain.gain.setValueAtTime(0.2, ctx.currentTime);
    gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + duration);
    osc.connect(gain);
    gain.connect(ctx.destination);
    osc.start();
    osc.stop(ctx.currentTime + duration);
  } catch (e) {}
}

function speakVoice(text) {
  if ("speechSynthesis" in window) {
    const utter = new SpeechSynthesisUtterance(text);
    utter.rate = 1.1;
    utter.pitch = 1.0;
    window.speechSynthesis.speak(utter);
  }
}

// ─── WebSocket Telemetry Stream ───
function connectWebSocket() {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  const wsUrl = `${protocol}//${window.location.host}/api/telemetry/ws`;
  
  ws = new WebSocket(wsUrl);

  ws.onopen = () => {
    document.getElementById("conn-status-badge").innerHTML = `<span class="pulse-dot" style="background:#10b981;"></span> ONLINE`;
    console.log("Telemetry WebSocket connected");
  };

  ws.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      activeTelemetry = data;
      renderTelemetry(data);
    } catch (e) {
      console.error("WS parse error:", e);
    }
  };

  ws.onclose = () => {
    document.getElementById("conn-status-badge").innerHTML = `<span class="pulse-dot" style="background:#ef4444;"></span> OFFLINE`;
    setTimeout(connectWebSocket, 2000);
  };
}

// ─── Render Incoming Telemetry ───
function renderTelemetry(d) {
  // 1. Update 3D Drone & HUD
  if (drone3D) {
    drone3D.updateAttitude(d.roll, d.pitch, d.yaw, d.motor_m1, d.motor_m2, d.motor_m3, d.motor_m4);
  }
  if (flightHUD) {
    flightHUD.update(d);
  }

  // 2. Update Map GPS
  if (mapPlanner) {
    mapPlanner.updateDronePosition(d.latitude, d.longitude, d.heading);
    mapPlanner.updateHome(d.home_lat, d.home_lon);
  }

  // 3. Update Oscilloscope Chart
  if (oscChart) {
    oscChart.pushData(d.roll, d.pitch, d.vibration_x, d.vibration_y, d.vibration_z);
  }

  // 4. Update Header Badges
  const armBadge = document.getElementById("arm-status-badge");
  if (armBadge) {
    if (d.emergency_cut) {
      armBadge.className = "telemetry-badge badge-arm-status disarmed";
      armBadge.innerHTML = `<span class="pulse-dot"></span> EMERGENCY CUT`;
    } else if (d.armed) {
      armBadge.className = "telemetry-badge badge-arm-status armed";
      armBadge.innerHTML = `<span class="pulse-dot"></span> ARMED`;
    } else {
      armBadge.className = "telemetry-badge badge-arm-status disarmed";
      armBadge.innerHTML = `<span class="pulse-dot"></span> DISARMED`;
    }
  }

  const modeSel = document.getElementById("mode-selector");
  if (modeSel && document.activeElement !== modeSel) {
    modeSel.value = d.flight_mode;
  }

  // Battery Telemetry
  const batVal = document.getElementById("bat-val");
  if (batVal) batVal.innerText = `${d.battery_voltage}V (${d.battery_percent}%)`;
  const batBar = document.getElementById("bat-bar");
  if (batBar) batBar.style.width = `${d.battery_percent}%`;

  // Numeric HUD metrics
  setMetricText("metric-alt", `${d.altitude_agl}m`);
  setMetricText("metric-spd", `${d.groundspeed}m/s`);
  setMetricText("metric-climb", `${d.climb_rate > 0 ? "+" : ""}${d.climb_rate}m/s`);
  setMetricText("metric-dist-home", `${d.dist_to_home}m`);
  setMetricText("metric-current", `${d.battery_current}A`);
  setMetricText("metric-mah", `${d.battery_consumed_mah}mAh`);
  setMetricText("metric-sats", `${d.satellites} (HDOP ${d.hdop})`);
  setMetricText("metric-temp", `${d.baro_temp}°C`);

  // Motors PWM Bars
  setMotorBar("m1-fill", "m1-text", d.motor_m1);
  setMotorBar("m2-fill", "m2-text", d.motor_m2);
  setMotorBar("m3-fill", "m3-text", d.motor_m3);
  setMotorBar("m4-fill", "m4-text", d.motor_m4);

  // Status bottom bar
  const statusBottom = document.getElementById("status-text-bottom");
  if (statusBottom) statusBottom.innerText = d.status_text || "STANDBY";
  const connTypeEl = document.getElementById("conn-type-text");
  if (connTypeEl) connTypeEl.innerText = `${d.connection_type.toUpperCase()}: ${d.source_address}`;
}

function setMetricText(id, text) {
  const el = document.getElementById(id);
  if (el) el.innerText = text;
}

function setMotorBar(fillId, textId, pct) {
  const fill = document.getElementById(fillId);
  const txt = document.getElementById(textId);
  if (fill) fill.style.height = `${pct}%`;
  if (txt) txt.innerText = `${Math.round(pct)}%`;
}

// ─── API Commands ───
async function toggleArm() {
  const isArmed = activeTelemetry?.armed;
  const targetArm = !isArmed;
  playBeep(targetArm ? 800 : 400, 0.2);
  speakVoice(targetArm ? "Motors Armed" : "Motors Disarmed");

  try {
    await fetch("/api/commands/arm", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ arm: targetArm })
    });
  } catch (e) {
    console.error("Arm failed:", e);
  }
}

async function setFlightMode(mode) {
  playBeep(650, 0.1);
  speakVoice(`Mode ${mode}`);
  try {
    await fetch("/api/commands/mode", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ mode })
    });
  } catch (e) {
    console.error("Mode change failed:", e);
  }
}

async function triggerTakeoff() {
  playBeep(900, 0.25);
  speakVoice("Auto Takeoff Initiated");
  try {
    await fetch("/api/commands/takeoff", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ altitude: 10.0 })
    });
  } catch (e) {
    console.error("Takeoff failed:", e);
  }
}

async function triggerRTL() {
  playBeep(750, 0.2);
  speakVoice("Return To Home Activated");
  try {
    await fetch("/api/commands/rtl", { method: "POST" });
  } catch (e) {
    console.error("RTL failed:", e);
  }
}

async function triggerLand() {
  playBeep(500, 0.2);
  speakVoice("Landing Sequence Activated");
  try {
    await fetch("/api/commands/land", { method: "POST" });
  } catch (e) {
    console.error("Land failed:", e);
  }
}

async function triggerEmergencyCut() {
  playBeep(300, 0.4, "sawtooth");
  speakVoice("Emergency Motor Cut Activated!");
  try {
    await fetch("/api/commands/emergency-cut", { method: "POST" });
  } catch (e) {
    console.error("Emergency cut failed:", e);
  }
}

async function uploadMission() {
  if (!mapPlanner || mapPlanner.waypoints.length === 0) {
    alert("Tambahkan waypoint terlebih dahulu pada peta!");
    return;
  }
  playBeep(880, 0.2);
  try {
    const res = await fetch("/api/commands/mission", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ waypoints: mapPlanner.waypoints })
    });
    const j = await res.json();
    alert(`Berhasil mengunggah ${j.count} Waypoint ke Drone!`);
  } catch (e) {
    alert("Gagal mengunggah misi!");
  }
}

// ─── Keyboard Control Mode ───
let stickState = { roll: 0, pitch: 0, yaw: 0, throttle: 0.5 };
let keyboardActive = false;

window.addEventListener("keydown", (e) => {
  if (e.target.tagName === "INPUT" || e.target.tagName === "SELECT") return;

  let changed = false;
  if (e.key === "w" || e.key === "W") { stickState.pitch = 0.5; changed = true; }
  else if (e.key === "s" || e.key === "S") { stickState.pitch = -0.5; changed = true; }
  else if (e.key === "a" || e.key === "A") { stickState.roll = -0.5; changed = true; }
  else if (e.key === "d" || e.key === "D") { stickState.roll = 0.5; changed = true; }
  else if (e.key === "ArrowUp") { stickState.throttle = Math.min(1.0, stickState.throttle + 0.05); changed = true; }
  else if (e.key === "ArrowDown") { stickState.throttle = Math.max(0.0, stickState.throttle - 0.05); changed = true; }
  else if (e.key === "ArrowLeft") { stickState.yaw = -0.6; changed = true; }
  else if (e.key === "ArrowRight") { stickState.yaw = 0.6; changed = true; }
  else if (e.key === " ") { triggerEmergencyCut(); }
  else if (e.key === "t" || e.key === "T") { triggerTakeoff(); }
  else if (e.key === "r" || e.key === "R") { triggerRTL(); }

  if (changed) {
    sendSticks();
  }
});

window.addEventListener("keyup", (e) => {
  if (e.target.tagName === "INPUT" || e.target.tagName === "SELECT") return;

  let changed = false;
  if (["w", "W", "s", "S"].includes(e.key)) { stickState.pitch = 0; changed = true; }
  if (["a", "A", "d", "D"].includes(e.key)) { stickState.roll = 0; changed = true; }
  if (["ArrowLeft", "ArrowRight"].includes(e.key)) { stickState.yaw = 0; changed = true; }

  if (changed) {
    sendSticks();
  }
});

async function sendSticks() {
  try {
    await fetch("/api/commands/rc", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(stickState)
    });
  } catch (e) {}
}

// ─── Modal Management ───
function openConnectionModal() {
  document.getElementById("conn-modal").classList.add("active");
  loadAvailablePorts();
}

function closeConnectionModal() {
  document.getElementById("conn-modal").classList.remove("active");
}

async function loadAvailablePorts() {
  try {
    const res = await fetch("/api/connection/ports");
    const j = await res.json();
    const sel = document.getElementById("serial-port-select");
    if (sel && j.ports) {
      sel.innerHTML = j.ports.length
        ? j.ports.map((p) => `<option value="${p}">${p}</option>`).join("")
        : `<option value="/dev/ttyUSB0">/dev/ttyUSB0</option><option value="COM3">COM3</option>`;
    }
  } catch (e) {}
}

async function applyConnection() {
  const type = document.getElementById("conn-type-select").value;
  const port = document.getElementById("serial-port-select").value;
  const baudrate = parseInt(document.getElementById("serial-baud-select").value, 10);
  const udpPort = parseInt(document.getElementById("udp-port-input").value, 10);
  const tcpHost = document.getElementById("tcp-host-input").value;

  try {
    const res = await fetch("/api/connection/connect", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ type, port, baudrate, udp_port: udpPort, tcp_host: tcpHost })
    });
    const j = await res.json();
    closeConnectionModal();
    alert(`Status Koneksi: ${j.status.toUpperCase()} (${j.type})`);
  } catch (e) {
    alert("Gagal menghubungkan telemetri!");
  }
}

// ─── App Bootstrap ───
window.addEventListener("DOMContentLoaded", () => {
  // 1. Init Three.js 3D View
  drone3D = new Drone3DView("three-canvas-container");

  // 2. Init Canvas PFD HUD
  flightHUD = new FlightHUD("pfd-canvas");

  // 3. Init Leaflet Map
  mapPlanner = new MapMissionPlanner("leaflet-map");

  // 4. Init Oscilloscope
  oscChart = new TelemetryOscilloscope("telemetry-chart-canvas");

  // 5. Connect WebSocket
  connectWebSocket();
});

