import asyncio
import math
import time
import random
from typing import Optional, List
from app.services.telemetry import telemetry_manager, Waypoint

class SITLQuadcopterSimulator:
    """
    High-fidelity 6-DOF Quadcopter physics simulation.
    Allows real-time testing of GCS 3D attitude, waypoint navigation,
    altimeter, speedometer, and battery telemetry.
    """
    
    def __init__(self):
        self.running = False
        self._task: Optional[asyncio.Task] = None
        
        # Physical constants
        self.mass_kg = 1.25  # 1.25 kg 5-inch / 7-inch quad
        self.g = 9.80665
        self.max_thrust_n = 28.0  # Max total thrust (4 motors) ~2.3:1 TWR
        
        # State variables
        self.x = 0.0  # meters relative to home (North)
        self.y = 0.0  # meters relative to home (East)
        self.z = 0.0  # meters altitude AGL (Up)
        
        self.vx = 0.0
        self.vy = 0.0
        self.vz = 0.0
        
        self.roll = 0.0      # deg (-180 to 180)
        self.pitch = 0.0     # deg (-90 to 90)
        self.yaw = 0.0       # deg (0 to 360)
        
        self.roll_rate = 0.0
        self.pitch_rate = 0.0
        self.yaw_rate = 0.0
        
        # Desired control inputs (-1.0 to 1.0)
        self.rc_roll = 0.0
        self.rc_pitch = 0.0
        self.rc_yaw = 0.0
        self.rc_throttle = 0.0  # 0.0 to 1.0
        
        # Flight states
        self.armed = False
        self.flight_mode = "STABILIZE"
        self.emergency_cut = False
        
        # Target setpoints for automated modes
        self.target_alt = 15.0
        self.target_speed = 6.0
        self.current_wp_index = 0
        self.mission_waypoints: List[Waypoint] = []
        
        # Home GPS (Monas Jakarta)
        self.home_lat = -6.175392
        self.home_lon = 106.827153
        self.home_alt = 15.0
        
        # Battery state
        self.battery_mah = 2200.0
        self.consumed_mah = 0.0
        self.battery_v_full = 12.6  # 3S LiPo
        self.battery_v_empty = 10.5
        
        # Motors PWM (0-100%)
        self.m1 = 0.0
        self.m2 = 0.0
        self.m3 = 0.0
        self.m4 = 0.0

    def arm(self):
        if not self.emergency_cut:
            self.armed = True

    def disarm(self):
        self.armed = False
        self.m1 = self.m2 = self.m3 = self.m4 = 0.0

    def emergency_stop(self):
        self.emergency_cut = True
        self.armed = False
        self.m1 = self.m2 = self.m3 = self.m4 = 0.0

    def reset_emergency(self):
        self.emergency_cut = False

    def set_mode(self, mode: str):
        valid = ["MANUAL", "STABILIZE", "ALT_HOLD", "LOITER", "AUTO", "RTL", "LAND"]
        mode = mode.upper()
        if mode in valid:
            self.flight_mode = mode
            if mode == "ALT_HOLD":
                self.target_alt = max(self.z, 2.0)
            elif mode == "RTL":
                self.target_alt = max(self.z, 30.0)

    def takeoff(self, alt: float = 10.0):
        if not self.armed:
            self.arm()
        self.set_mode("ALT_HOLD")
        self.target_alt = max(alt, 2.0)

    def set_rc(self, roll: float, pitch: float, yaw: float, throttle: float):
        self.rc_roll = max(-1.0, min(1.0, roll))
        self.rc_pitch = max(-1.0, min(1.0, pitch))
        self.rc_yaw = max(-1.0, min(1.0, yaw))
        self.rc_throttle = max(0.0, min(1.0, throttle))

    def load_mission(self, waypoints: List[Waypoint]):
        self.mission_waypoints = waypoints
        self.current_wp_index = 0

    def _meters_to_lat_lon(self, north: float, east: float):
        # WGS84 approximation: 1 deg lat ~ 111132 m, 1 deg lon ~ 111319 * cos(lat)
        d_lat = north / 111132.0
        lat_rad = math.radians(self.home_lat)
        d_lon = east / (111319.0 * math.cos(lat_rad))
        return self.home_lat + d_lat, self.home_lon + d_lon

    def _step_physics(self, dt: float):
        """Perform 6-DOF numerical integration."""
        if not self.armed or self.emergency_cut:
            # Motors are off
            self.m1 = self.m2 = self.m3 = self.m4 = 0.0
            
            # Ground contact constraint
            if self.z <= 0.0:
                self.z = 0.0
                self.vz = 0.0
                self.vx *= 0.8
                self.vy *= 0.8
                # Dampen tilt towards level on ground
                self.roll *= 0.8
                self.pitch *= 0.8
                return
            else:
                # Free fall if in air and cut
                self.vz -= self.g * dt
                self.z += self.vz * dt
                if self.z < 0.0:
                    self.z = 0.0
                    self.vz = 0.0
                return

        # 1. Mode controller to determine desired roll, pitch, yaw, thrust
        target_roll_deg = 0.0
        target_pitch_deg = 0.0
        target_thrust_ratio = 0.0

        if self.flight_mode == "MANUAL":
            target_roll_deg = self.rc_roll * 45.0
            target_pitch_deg = -self.rc_pitch * 45.0
            target_thrust_ratio = self.rc_throttle

        elif self.flight_mode == "STABILIZE":
            target_roll_deg = self.rc_roll * 35.0
            target_pitch_deg = -self.rc_pitch * 35.0
            # Hover is around 45% throttle
            target_thrust_ratio = self.rc_throttle

        elif self.flight_mode in ["ALT_HOLD", "LOITER", "AUTO", "RTL", "LAND"]:
            # Altitude PID loop
            if self.flight_mode == "LAND":
                target_climb_rate = -0.6  # descend smoothly at 0.6 m/s
                if self.z < 0.5:
                    target_climb_rate = -0.3
                if self.z <= 0.05:
                    self.disarm()
                    self.z = 0.0
                    return
            else:
                # Target alt control
                alt_err = self.target_alt - self.z
                target_climb_rate = max(-3.0, min(3.0, alt_err * 0.8))

            climb_err = target_climb_rate - self.vz
            hover_thrust = (self.mass_kg * self.g) / self.max_thrust_n
            target_thrust_ratio = max(0.1, min(0.95, hover_thrust + climb_err * 0.15))

            # Horizontal Navigation Loop
            if self.flight_mode == "LOITER":
                # Resists drift using position hold
                target_roll_deg = max(-25.0, min(25.0, -self.vy * 4.0))
                target_pitch_deg = max(-25.0, min(25.0, self.vx * 4.0))
                # Add manual stick nudge
                target_roll_deg += self.rc_roll * 15.0
                target_pitch_deg -= self.rc_pitch * 15.0

            elif self.flight_mode == "AUTO" and self.mission_waypoints:
                if self.current_wp_index < len(self.mission_waypoints):
                    wp = self.mission_waypoints[self.current_wp_index]
                    self.target_alt = wp.alt
                    
                    # Convert WP lat/lon to local meters
                    d_lat = wp.lat - self.home_lat
                    d_lon = wp.lon - self.home_lon
                    wp_n = d_lat * 111132.0
                    wp_e = d_lon * (111319.0 * math.cos(math.radians(self.home_lat)))
                    
                    dist_to_wp = math.hypot(wp_n - self.x, wp_e - self.y)
                    if dist_to_wp < 3.0:
                        # Reached waypoint
                        self.current_wp_index += 1
                        if self.current_wp_index >= len(self.mission_waypoints):
                            self.set_mode("LOITER")
                    else:
                        # Fly towards WP
                        angle = math.atan2(wp_e - self.y, wp_n - self.x)
                        des_vx = math.cos(angle) * wp.speed
                        des_vy = math.sin(angle) * wp.speed
                        
                        target_pitch_deg = max(-30.0, min(30.0, (des_vx - self.vx) * 5.0))
                        target_roll_deg = max(-30.0, min(30.0, -(des_vy - self.vy) * 5.0))
                        
                        # Point nose to WP
                        des_yaw = math.degrees(angle) % 360.0
                        yaw_err = (des_yaw - self.yaw + 180) % 360 - 180
                        self.yaw += yaw_err * 0.05

            elif self.flight_mode == "RTL":
                # Return to Home (0,0) at safe altitude
                dist_to_home = math.hypot(-self.x, -self.y)
                if self.z < self.target_alt - 1.0:
                    # Climb first
                    target_roll_deg = 0.0
                    target_pitch_deg = 0.0
                elif dist_to_home > 3.0:
                    angle = math.atan2(-self.y, -self.x)
                    des_vx = math.cos(angle) * 5.0
                    des_vy = math.sin(angle) * 5.0
                    target_pitch_deg = max(-25.0, min(25.0, (des_vx - self.vx) * 4.0))
                    target_roll_deg = max(-25.0, min(25.0, -(des_vy - self.vy) * 4.0))
                else:
                    # Over home, auto land
                    self.set_mode("LAND")

        # 2. Attitude dynamics with PID response
        roll_err = target_roll_deg - self.roll
        pitch_err = target_pitch_deg - self.pitch
        self.roll += roll_err * min(1.0, dt * 12.0)
        self.pitch += pitch_err * min(1.0, dt * 12.0)
        self.yaw = (self.yaw + self.rc_yaw * 120.0 * dt) % 360.0

        # Small turbulence / motor vibration noise
        self.roll += (random.random() - 0.5) * 0.15
        self.pitch += (random.random() - 0.5) * 0.15

        # 3. Thrust vector & accelerations
        total_thrust_n = target_thrust_ratio * self.max_thrust_n
        
        # Rotations in radians
        r_rad = math.radians(self.roll)
        p_rad = math.radians(self.pitch)
        y_rad = math.radians(self.yaw)

        # Acceleration in body frame rotated to world frame
        # Pitch down = positive forward thrust (North), Roll right = positive right thrust (East)
        ax = (total_thrust_n / self.mass_kg) * (math.sin(p_rad) * math.cos(y_rad) + math.sin(r_rad) * math.sin(y_rad))
        ay = (total_thrust_n / self.mass_kg) * (math.sin(p_rad) * math.sin(y_rad) - math.sin(r_rad) * math.cos(y_rad))
        az = (total_thrust_n / self.mass_kg) * (math.cos(r_rad) * math.cos(p_rad)) - self.g

        # Aerodynamic drag
        drag_coeff = 0.25
        ax -= drag_coeff * self.vx * abs(self.vx)
        ay -= drag_coeff * self.vy * abs(self.vy)

        # Integrate velocity & position
        self.vx += ax * dt
        self.vy += ay * dt
        self.vz += az * dt

        self.x += self.vx * dt
        self.y += self.vy * dt
        self.z += self.vz * dt

        if self.z <= 0.0:
            self.z = 0.0
            self.vz = 0.0

        # 4. Motor individual mixer (Quad-X configuration)
        # M1 (front-left CCW), M2 (front-right CW), M3 (rear-left CW), M4 (rear-right CCW)
        m_base = target_thrust_ratio * 100.0
        d_roll = self.roll * 0.5
        d_pitch = self.pitch * 0.5
        
        self.m1 = max(0.0, min(100.0, m_base - d_roll + d_pitch))
        self.m2 = max(0.0, min(100.0, m_base + d_roll + d_pitch))
        self.m3 = max(0.0, min(100.0, m_base - d_roll - d_pitch))
        self.m4 = max(0.0, min(100.0, m_base + d_roll - d_pitch))

        # 5. Battery discharge
        current_a = (target_thrust_ratio * 25.0) + (1.2 if self.armed else 0.4)
        mah_drain = (current_a * (dt / 3600.0)) * 1000.0
        self.consumed_mah += mah_drain
        rem_mah = max(0.0, self.battery_mah - self.consumed_mah)
        self.battery_pct = int((rem_mah / self.battery_mah) * 100.0)

    async def _loop(self):
        hz = 25
        dt = 1.0 / hz
        while self.running:
            t0 = time.time()
            self._step_physics(dt)
            
            # Convert meters to GPS coordinates
            curr_lat, curr_lon = self._meters_to_lat_lon(self.x, self.y)
            groundspeed = math.hypot(self.vx, self.vy)
            dist_home = math.hypot(self.x, self.y)
            
            # Dist to current waypoint
            dist_wp = 0.0
            if self.mission_waypoints and self.current_wp_index < len(self.mission_waypoints):
                wp = self.mission_waypoints[self.current_wp_index]
                wp_n = (wp.lat - self.home_lat) * 111132.0
                wp_e = (wp.lon - self.home_lon) * (111319.0 * math.cos(math.radians(self.home_lat)))
                dist_wp = math.hypot(wp_n - self.x, wp_e - self.y)

            # Battery voltage calculation under load
            pct_ratio = self.battery_pct / 100.0
            v_open = self.battery_v_empty + (self.battery_v_full - self.battery_v_empty) * pct_ratio
            current_a = (self.m1 + self.m2 + self.m3 + self.m4) / 400.0 * 25.0
            v_sag = v_open - (current_a * 0.04)

            # Update central telemetry state
            await telemetry_manager.update_state(
                connected=True,
                connection_type="sitl",
                source_address="Internal SITL 6-DOF Engine",
                armed=self.armed,
                flight_mode=self.flight_mode,
                emergency_cut=self.emergency_cut,
                status_text=f"FLYING [{self.flight_mode}]" if (self.armed and self.z > 0.3) else ("ARMED (GROUND)" if self.armed else "DISARMED"),
                roll=round(self.roll, 2),
                pitch=round(self.pitch, 2),
                yaw=round(self.yaw, 1),
                target_roll=round(self.rc_roll * 35.0, 1),
                target_pitch=round(-self.rc_pitch * 35.0, 1),
                target_yaw=round(self.yaw, 1),
                roll_rate=round(self.roll_rate, 2),
                pitch_rate=round(self.pitch_rate, 2),
                yaw_rate=round(self.yaw_rate, 2),
                vx=round(self.vx, 2),
                vy=round(self.vy, 2),
                vz=round(self.vz, 2),
                groundspeed=round(groundspeed, 2),
                airspeed=round(groundspeed + random.uniform(-0.3, 0.3), 2),
                climb_rate=round(self.vz, 2),
                latitude=curr_lat,
                longitude=curr_lon,
                altitude_msl=round(self.home_alt + self.z, 2),
                altitude_agl=round(self.z, 2),
                heading=round(self.yaw, 1),
                home_lat=self.home_lat,
                home_lon=self.home_lon,
                home_alt=self.home_alt,
                dist_to_home=round(dist_home, 1),
                battery_voltage=round(max(10.0, v_sag), 2),
                battery_current=round(current_a, 2),
                battery_percent=self.battery_pct,
                battery_consumed_mah=round(self.consumed_mah, 1),
                motor_m1=round(self.m1, 1),
                motor_m2=round(self.m2, 1),
                motor_m3=round(self.m3, 1),
                motor_m4=round(self.m4, 1),
                throttle=round((self.m1 + self.m2 + self.m3 + self.m4) / 4.0, 1),
                baro_altitude=round(self.z, 2),
                tof_distance_m=round(self.z, 2) if self.z < 4.0 else 0.0,
                vibration_x=round(0.02 + (self.m1 / 100.0) * 0.1 + random.uniform(0, 0.02), 3),
                vibration_y=round(0.02 + (self.m2 / 100.0) * 0.1 + random.uniform(0, 0.02), 3),
                vibration_z=round(0.03 + (self.m3 / 100.0) * 0.15 + random.uniform(0, 0.03), 3),
                current_wp=self.current_wp_index + 1 if self.mission_waypoints else 0,
                total_wp=len(self.mission_waypoints),
                dist_to_wp=round(dist_wp, 1),
                mission_running=(self.flight_mode == "AUTO")
            )
            
            elapsed = time.time() - t0
            sleep_t = max(0.002, dt - elapsed)
            await asyncio.sleep(sleep_t)

    def start(self):
        if not self.running:
            self.running = True
            self._task = asyncio.create_task(self._loop())

    def stop(self):
        self.running = False
        if self._task and not self._task.done():
            self._task.cancel()

sitl_sim = SITLQuadcopterSimulator()

