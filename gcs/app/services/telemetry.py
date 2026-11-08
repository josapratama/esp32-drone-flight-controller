import asyncio
import time
from typing import Dict, List, Optional, Set
from pydantic import BaseModel, Field

class DroneState(BaseModel):
    timestamp: float = Field(default_factory=time.time)
    connected: bool = False
    connection_type: str = "none"  # sitl, serial, udp, tcp
    source_address: str = "disconnected"
    
    # Flight status
    armed: bool = False
    flight_mode: str = "STABILIZE"
    emergency_cut: bool = False
    failsafe: bool = False
    status_text: str = "STANDBY"
    
    # Attitude (deg)
    roll: float = 0.0      # -180 to +180 deg (tilt right +)
    pitch: float = 0.0     # -90 to +90 deg (pitch up +)
    yaw: float = 0.0       # 0 to 360 deg heading
    
    # Desired Attitude (from controller/RC)
    target_roll: float = 0.0
    target_pitch: float = 0.0
    target_yaw: float = 0.0
    
    # Angular rates (deg/s)
    roll_rate: float = 0.0
    pitch_rate: float = 0.0
    yaw_rate: float = 0.0
    
    # Velocity (m/s)
    vx: float = 0.0  # North
    vy: float = 0.0  # East
    vz: float = 0.0  # Down (climb rate = -vz)
    groundspeed: float = 0.0
    airspeed: float = 0.0
    climb_rate: float = 0.0
    
    # Position & GPS
    latitude: float = -6.175392
    longitude: float = 106.827153
    altitude_msl: float = 15.0      # Above sea level (m)
    altitude_agl: float = 0.0       # Relative to takeoff/home (m)
    heading: float = 0.0
    gps_fix: int = 3                # 0=No, 1=2D, 2=3D, 3=DGPS
    satellites: int = 14
    hdop: float = 0.85
    
    # Home location
    home_lat: float = -6.175392
    home_lon: float = 106.827153
    home_alt: float = 15.0
    dist_to_home: float = 0.0
    
    # Power & Battery
    battery_voltage: float = 12.4   # 3S LiPo (~12.6V full, 10.5V empty)
    battery_current: float = 0.0    # Amps
    battery_percent: int = 95       # %
    battery_consumed_mah: float = 0.0
    estimated_flight_time_sec: int = 900
    
    # Motors (0-100%)
    motor_m1: float = 0.0
    motor_m2: float = 0.0
    motor_m3: float = 0.0
    motor_m4: float = 0.0
    throttle: float = 0.0
    
    # Sensors
    baro_altitude: float = 0.0
    baro_temp: float = 29.5         # Celcius
    tof_distance_m: float = 0.0
    vibration_x: float = 0.02
    vibration_y: float = 0.03
    vibration_z: float = 0.04
    
    # Mission Navigation
    current_wp: int = 0
    total_wp: int = 0
    dist_to_wp: float = 0.0
    mission_running: bool = False

class Waypoint(BaseModel):
    id: int
    lat: float
    lon: float
    alt: float = 20.0
    speed: float = 5.0
    action: str = "WAYPOINT"  # WAYPOINT, TAKEOFF, LAND, RTL, LOITER_TIME
    param1: float = 0.0       # e.g. loiter time in seconds

class TelemetryManager:
    """Singleton telemetry hub managing state and broadcasting to WebSockets."""
    
    def __init__(self):
        self.state = DroneState()
        self._subscribers: Set[asyncio.Queue] = set()
        self._lock = asyncio.Lock()
        self.waypoints: List[Waypoint] = []
        self.flight_logs: List[dict] = []
        
    async def update_state(self, **kwargs):
        """Update drone state fields and trigger broadcast."""
        async with self._lock:
            for k, v in kwargs.items():
                if hasattr(self.state, k):
                    setattr(self.state, k, v)
            self.state.timestamp = time.time()
            data = self.state.model_dump()
            
        # Push to all active websocket queues
        for q in list(self._subscribers):
            try:
                if q.qsize() < 10:
                    q.put_nowait(data)
            except Exception:
                pass
                
    def get_state(self) -> DroneState:
        return self.state
        
    def subscribe(self) -> asyncio.Queue:
        q = asyncio.Queue(maxsize=20)
        self._subscribers.add(q)
        return q
        
    def unsubscribe(self, q: asyncio.Queue):
        self._subscribers.discard(q)

telemetry_manager = TelemetryManager()

