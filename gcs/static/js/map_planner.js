/**
 * Web 3D Drone GCS — Leaflet GPS Tracker & Interactive Waypoint Mission Planner
 */

class MapMissionPlanner {
  constructor(mapContainerId) {
    this.containerId = mapContainerId;
    this.map = null;
    this.droneMarker = null;
    this.homeMarker = null;
    this.flightTrail = null;
    this.missionPolyline = null;
    this.trailCoords = [];
    this.waypoints = [];
    this.wpMarkers = [];
    this.planningMode = false;
    this.homeLat = -6.175392;
    this.homeLon = 106.827153;

    this.initMap();
  }

  initMap() {
    // Center at Monas Jakarta by default
    this.map = L.map(this.containerId, {
      zoomControl: false,
      attributionControl: false
    }).setView([this.homeLat, this.homeLon], 17);

    // Zoom controls at bottom right
    L.control.zoom({ position: "bottomright" }).addTo(this.map);

    // Dark Cyberpunk / Satellite Tile layer (CartoDB Dark Matter)
    L.tileLayer("https://{s}.basemaps.cartocdn.com/dark_all/{z}/{x}/{y}{r}.png", {
      maxZoom: 19,
      subdomains: "abcd"
    }).addTo(this.map);

    // Flight Trail Polyline (Glowing Cyan)
    this.flightTrail = L.polyline([], {
      color: "#00f0ff",
      weight: 3,
      opacity: 0.8,
      dashArray: "4, 6"
    }).addTo(this.map);

    // Mission Trajectory Polyline (Orange)
    this.missionPolyline = L.polyline([], {
      color: "#f59e0b",
      weight: 2.5,
      opacity: 0.9
    }).addTo(this.map);

    // Home Marker
    const homeIcon = L.divIcon({
      className: "custom-home-marker",
      html: `<div style="background:#10b981; color:#fff; border-radius:50%; width:24px; height:24px; display:flex; align-items:center; justify-content:center; font-size:12px; font-weight:bold; border:2px solid #fff; box-shadow:0 0 10px #10b981;">H</div>`,
      iconSize: [24, 24],
      iconAnchor: [12, 12]
    });
    this.homeMarker = L.marker([this.homeLat, this.homeLon], { icon: homeIcon }).addTo(this.map);

    // Drone Marker with rotating SVG
    const droneIcon = L.divIcon({
      className: "custom-drone-marker",
      html: `<div id="drone-map-icon" style="width:36px; height:36px; transform-origin:center; transition:transform 0.1s linear;">
        <svg viewBox="0 0 40 40" width="36" height="36">
          <circle cx="20" cy="20" r="14" fill="rgba(0, 240, 255, 0.25)" stroke="#00f0ff" stroke-width="2"/>
          <path d="M20 6 L27 28 L20 23 L13 28 Z" fill="#00f0ff" stroke="#fff" stroke-width="1.5"/>
        </svg>
      </div>`,
      iconSize: [36, 36],
      iconAnchor: [18, 18]
    });
    this.droneMarker = L.marker([this.homeLat, this.homeLon], { icon: droneIcon }).addTo(this.map);

    // Map click listener for Waypoint placement
    this.map.on("click", (e) => {
      if (this.planningMode) {
        this.addWaypoint(e.latlng.lat, e.latlng.lng);
      }
    });
  }

  updateDronePosition(lat, lon, heading) {
    if (!this.map || isNaN(lat) || isNaN(lon)) return;

    const latlng = [lat, lon];
    this.droneMarker.setLatLng(latlng);

    // Rotate heading SVG
    const iconEl = document.getElementById("drone-map-icon");
    if (iconEl) {
      iconEl.style.transform = `rotate(${heading}deg)`;
    }

    // Append to flight trail
    this.trailCoords.push(latlng);
    if (this.trailCoords.length > 500) this.trailCoords.shift();
    this.flightTrail.setLatLngs(this.trailCoords);
  }

  updateHome(lat, lon) {
    this.homeLat = lat;
    this.homeLon = lon;
    if (this.homeMarker) {
      this.homeMarker.setLatLng([lat, lon]);
    }
  }

  centerOnDrone() {
    const pos = this.droneMarker.getLatLng();
    this.map.panTo(pos);
  }

  togglePlanningMode(active) {
    this.planningMode = active !== undefined ? active : !this.planningMode;
    return this.planningMode;
  }

  addWaypoint(lat, lon, alt = 15.0, speed = 6.0) {
    const id = this.waypoints.length + 1;
    const wp = { id, lat, lon, alt, speed, action: "WAYPOINT" };
    this.waypoints.push(wp);

    // Create Map Marker
    const wpIcon = L.divIcon({
      className: "custom-wp-marker",
      html: `<div style="background:#f59e0b; color:#000; border-radius:50%; width:22px; height:22px; display:flex; align-items:center; justify-content:center; font-size:11px; font-weight:bold; border:2px solid #fff; box-shadow:0 0 8px #f59e0b;">${id}</div>`,
      iconSize: [22, 22],
      iconAnchor: [11, 11]
    });

    const marker = L.marker([lat, lon], { icon: wpIcon, draggable: true }).addTo(this.map);

    marker.on("dragend", (e) => {
      const pos = e.target.getLatLng();
      wp.lat = pos.lat;
      wp.lon = pos.lng;
      this.renderMissionPolyline();
      this.renderWaypointTable();
    });

    this.wpMarkers.push(marker);
    this.renderMissionPolyline();
    this.renderWaypointTable();
  }

  renderMissionPolyline() {
    const latlngs = this.waypoints.map((w) => [w.lat, w.lon]);
    this.missionPolyline.setLatLngs(latlngs);
  }

  renderWaypointTable() {
    const container = document.getElementById("wp-table-body");
    const countEl = document.getElementById("wp-count-badge");
    if (countEl) countEl.innerText = `${this.waypoints.length} WP`;
    if (!container) return;

    if (this.waypoints.length === 0) {
      container.innerHTML = `<div style="padding: 12px; text-align: center; color: var(--text-muted);">Klik peta untuk menambah Waypoint baru</div>`;
      return;
    }

    container.innerHTML = this.waypoints
      .map(
        (w) => `
      <div class="wp-row">
        <span><b>#${w.id}</b> (${w.lat.toFixed(5)}, ${w.lon.toFixed(5)})</span>
        <span style="color:var(--cyan-neon);">${w.alt}m @ ${w.speed}m/s</span>
      </div>`
      )
      .join("");
  }

  clearWaypoints() {
    this.wpMarkers.forEach((m) => this.map.removeLayer(m));
    this.wpMarkers = [];
    this.waypoints = [];
    this.missionPolyline.setLatLngs([]);
    this.renderWaypointTable();
  }
}

window.MapMissionPlanner = MapMissionPlanner;

