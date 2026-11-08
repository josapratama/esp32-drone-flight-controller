/**
 * Web 3D Drone GCS — Lightweight Canvas Real-Time Telemetry & Vibration Charts
 */

class TelemetryOscilloscope {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.maxSamples = 120;
    
    this.rollHistory = [];
    this.pitchHistory = [];
    this.vibXHistory = [];
    this.vibYHistory = [];
    this.vibZHistory = [];

    this.resize();
    window.addEventListener("resize", () => this.resize());
  }

  resize() {
    if (!this.canvas) return;
    this.width = this.canvas.clientWidth;
    this.height = this.canvas.clientHeight;
    this.canvas.width = this.width;
    this.canvas.height = this.height;
  }

  pushData(roll, pitch, vx, vy, vz) {
    this.rollHistory.push(roll);
    this.pitchHistory.push(pitch);
    this.vibXHistory.push(vx * 100);
    this.vibYHistory.push(vy * 100);
    this.vibZHistory.push(vz * 100);

    if (this.rollHistory.length > this.maxSamples) {
      this.rollHistory.shift();
      this.pitchHistory.shift();
      this.vibXHistory.shift();
      this.vibYHistory.shift();
      this.vibZHistory.shift();
    }

    this.render();
  }

  render() {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const midY = h / 2;

    ctx.clearRect(0, 0, w, h);

    // Center Zero line
    ctx.strokeStyle = "rgba(255, 255, 255, 0.1)";
    ctx.lineWidth = 1;
    ctx.beginPath();
    ctx.moveTo(0, midY);
    ctx.lineTo(w, midY);
    ctx.stroke();

    // Upper/Lower grid lines (+20 deg, -20 deg)
    ctx.strokeStyle = "rgba(255, 255, 255, 0.05)";
    ctx.beginPath();
    ctx.moveTo(0, midY - 30);
    ctx.lineTo(w, midY - 30);
    ctx.moveTo(0, midY + 30);
    ctx.lineTo(w, midY + 30);
    ctx.stroke();

    // Plot Roll (Cyan)
    this.plotLine(this.rollHistory, "#00f0ff", midY, 1.5);

    // Plot Pitch (Amber)
    this.plotLine(this.pitchHistory, "#f59e0b", midY, 1.5);
  }

  plotLine(data, color, zeroY, scale = 1.0) {
    if (data.length < 2) return;
    const ctx = this.ctx;
    const stepX = this.width / (this.maxSamples - 1);

    ctx.strokeStyle = color;
    ctx.lineWidth = 1.5;
    ctx.beginPath();

    for (let i = 0; i < data.length; i++) {
      const x = i * stepX;
      const y = zeroY - data[i] * scale;
      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }
}

window.TelemetryOscilloscope = TelemetryOscilloscope;

