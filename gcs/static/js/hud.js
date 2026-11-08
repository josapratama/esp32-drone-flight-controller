/**
 * Web 3D Drone GCS — Primary Flight Display (PFD) HUD Canvas Overlay
 */

class FlightHUD {
  constructor(canvasId) {
    this.canvas = document.getElementById(canvasId);
    this.ctx = this.canvas.getContext("2d");
    this.roll = 0;
    this.pitch = 0;
    this.yaw = 0;
    this.altitude = 0;
    this.speed = 0;
    this.climbRate = 0;

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

  update(telemetry) {
    this.roll = telemetry.roll || 0;
    this.pitch = telemetry.pitch || 0;
    this.yaw = telemetry.heading || telemetry.yaw || 0;
    this.altitude = telemetry.altitude_agl || 0;
    this.speed = telemetry.groundspeed || 0;
    this.climbRate = telemetry.climb_rate || 0;
    this.draw();
  }

  draw() {
    const ctx = this.ctx;
    const w = this.width;
    const h = this.height;
    const cx = w / 2;
    const cy = h / 2;

    ctx.clearRect(0, 0, w, h);

    // 1. Draw Pitch Ladder (Artificial Horizon lines)
    ctx.save();
    ctx.translate(cx, cy);
    // Rotate with Roll
    ctx.rotate((-this.roll * Math.PI) / 180);

    const pitchPixelPerDeg = 4.0;
    const pitchOffset = this.pitch * pitchPixelPerDeg;

    ctx.strokeStyle = "rgba(0, 240, 255, 0.4)";
    ctx.fillStyle = "rgba(0, 240, 255, 0.8)";
    ctx.lineWidth = 1.5;
    ctx.font = "10px monospace";
    ctx.textAlign = "center";

    // Pitch rungs (-40 to +40 deg every 10 deg)
    for (let p = -40; p <= 40; p += 10) {
      if (p === 0) continue;
      const y = -p * pitchPixelPerDeg + pitchOffset;
      if (Math.abs(y) > cy - 30) continue;

      const rungW = p % 20 === 0 ? 60 : 35;
      
      // Left bar
      ctx.beginPath();
      ctx.moveTo(-rungW, y);
      ctx.lineTo(-15, y);
      // downward/upward notch
      ctx.lineTo(-15, y + (p > 0 ? 5 : -5));
      ctx.stroke();

      // Right bar
      ctx.beginPath();
      ctx.moveTo(15, y + (p > 0 ? 5 : -5));
      ctx.lineTo(15, y);
      ctx.lineTo(rungW, y);
      ctx.stroke();

      // Degree labels
      ctx.fillText(`${Math.abs(p)}`, -rungW - 12, y + 3);
      ctx.fillText(`${Math.abs(p)}`, rungW + 12, y + 3);
    }

    // Horizon line (0 deg)
    const zeroY = pitchOffset;
    if (Math.abs(zeroY) < cy) {
      ctx.strokeStyle = "rgba(56, 189, 248, 0.75)";
      ctx.lineWidth = 2;
      ctx.beginPath();
      ctx.moveTo(-110, zeroY);
      ctx.lineTo(-25, zeroY);
      ctx.moveTo(25, zeroY);
      ctx.lineTo(110, zeroY);
      ctx.stroke();
    }
    ctx.restore();

    // 2. Fixed Center Aircraft Reticle (Boresight)
    ctx.strokeStyle = "#00f0ff";
    ctx.fillStyle = "#00f0ff";
    ctx.lineWidth = 2.5;
    ctx.beginPath();
    // Center pip
    ctx.arc(cx, cy, 3, 0, Math.PI * 2);
    ctx.fill();
    // Left wing
    ctx.beginPath();
    ctx.moveTo(cx - 35, cy);
    ctx.lineTo(cx - 12, cy);
    ctx.lineTo(cx - 12, cy + 6);
    // Right wing
    ctx.moveTo(cx + 35, cy);
    ctx.lineTo(cx + 12, cy);
    ctx.lineTo(cx + 12, cy + 6);
    ctx.stroke();

    // 3. Top Compass Heading Tape
    this.drawCompassTape(ctx, w);

    // 4. Left Airspeed Tape
    this.drawSpeedTape(ctx, cy);

    // 5. Right Altimeter Tape
    this.drawAltitudeTape(ctx, w, cy);
  }

  drawCompassTape(ctx, w) {
    const cx = w / 2;
    const tapeY = 18;
    const pixelsPerDeg = 2.5;

    ctx.save();
    // Background ribbon
    ctx.fillStyle = "rgba(10, 14, 23, 0.75)";
    ctx.strokeStyle = "rgba(56, 189, 248, 0.25)";
    ctx.lineWidth = 1;
    ctx.fillRect(cx - 120, 2, 240, 26);
    ctx.strokeRect(cx - 120, 2, 240, 26);

    ctx.font = "9px monospace";
    ctx.textAlign = "center";

    // Center indicator pointer
    ctx.fillStyle = "#00f0ff";
    ctx.beginPath();
    ctx.moveTo(cx, tapeY + 9);
    ctx.lineTo(cx - 5, tapeY + 14);
    ctx.lineTo(cx + 5, tapeY + 14);
    ctx.closePath();
    ctx.fill();

    // Heading labels
    const currentHeading = this.yaw;
    for (let h = 0; h < 360; h += 15) {
      let diff = h - currentHeading;
      while (diff < -180) diff += 360;
      while (diff > 180) diff -= 360;

      const x = cx + diff * pixelsPerDeg;
      if (x < cx - 110 || x > cx + 110) continue;

      ctx.strokeStyle = "rgba(255, 255, 255, 0.3)";
      ctx.beginPath();
      ctx.moveTo(x, tapeY - 6);
      ctx.lineTo(x, tapeY);
      ctx.stroke();

      let label = `${h}`;
      if (h === 0) label = "N";
      else if (h === 90) label = "E";
      else if (h === 180) label = "S";
      else if (h === 270) label = "W";

      ctx.fillStyle = h % 90 === 0 ? "#00f0ff" : "rgba(255,255,255,0.7)";
      ctx.fillText(label, x, tapeY - 8);
    }
    ctx.restore();
  }

  drawSpeedTape(ctx, cy) {
    const tapeX = 35;
    const pixelsPerUnit = 4.0;

    ctx.save();
    // Background tape
    ctx.fillStyle = "rgba(10, 14, 23, 0.75)";
    ctx.fillRect(2, cy - 80, 50, 160);
    ctx.strokeStyle = "rgba(56, 189, 248, 0.25)";
    ctx.strokeRect(2, cy - 80, 50, 160);

    ctx.font = "10px monospace";
    ctx.textAlign = "right";

    const currentSpd = this.speed;
    const startSpd = Math.max(0, Math.floor(currentSpd - 15));
    const endSpd = Math.ceil(currentSpd + 15);

    for (let s = startSpd; s <= endSpd; s += 2) {
      const y = cy - (s - currentSpd) * pixelsPerUnit;
      if (y < cy - 75 || y > cy + 75) continue;

      ctx.strokeStyle = "rgba(255, 255, 255, 0.3)";
      ctx.beginPath();
      ctx.moveTo(tapeX, y);
      ctx.lineTo(tapeX + (s % 10 === 0 ? 10 : 5), y);
      ctx.stroke();

      if (s % 10 === 0) {
        ctx.fillStyle = "rgba(255, 255, 255, 0.75)";
        ctx.fillText(`${s}`, tapeX - 4, y + 3);
      }
    }

    // Current Speed Box
    ctx.fillStyle = "#00f0ff";
    ctx.fillRect(1, cy - 10, 48, 20);
    ctx.fillStyle = "#0a0e17";
    ctx.font = "bold 11px monospace";
    ctx.textAlign = "center";
    ctx.fillText(`${this.speed.toFixed(1)}`, 24, cy + 4);
    ctx.restore();
  }

  drawAltitudeTape(ctx, w, cy) {
    const tapeX = w - 45;
    const pixelsPerUnit = 3.5;

    ctx.save();
    // Background tape
    ctx.fillStyle = "rgba(10, 14, 23, 0.75)";
    ctx.fillRect(w - 52, cy - 80, 50, 160);
    ctx.strokeStyle = "rgba(56, 189, 248, 0.25)";
    ctx.strokeRect(w - 52, cy - 80, 50, 160);

    ctx.font = "10px monospace";
    ctx.textAlign = "left";

    const currentAlt = this.altitude;
    const startAlt = Math.max(0, Math.floor(currentAlt - 20));
    const endAlt = Math.ceil(currentAlt + 20);

    for (let a = startAlt; a <= endAlt; a += 5) {
      const y = cy - (a - currentAlt) * pixelsPerUnit;
      if (y < cy - 75 || y > cy + 75) continue;

      ctx.strokeStyle = "rgba(255, 255, 255, 0.3)";
      ctx.beginPath();
      ctx.moveTo(tapeX - (a % 10 === 0 ? 10 : 5), y);
      ctx.lineTo(tapeX, y);
      ctx.stroke();

      if (a % 10 === 0) {
        ctx.fillStyle = "rgba(255, 255, 255, 0.75)";
        ctx.fillText(`${a}`, tapeX + 4, y + 3);
      }
    }

    // Current Altitude Box
    ctx.fillStyle = "#00f0ff";
    ctx.fillRect(w - 50, cy - 10, 48, 20);
    ctx.fillStyle = "#0a0e17";
    ctx.font = "bold 11px monospace";
    ctx.textAlign = "center";
    ctx.fillText(`${this.altitude.toFixed(1)}m`, w - 26, cy + 4);
    ctx.restore();
  }
}

window.FlightHUD = FlightHUD;

