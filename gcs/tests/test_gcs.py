import pytest
import asyncio
from app.services.sitl_simulator import SITLQuadcopterSimulator
from app.services.telemetry import TelemetryManager, Waypoint

@pytest.mark.asyncio
async def test_sitl_quadcopter_physics():
    sim = SITLQuadcopterSimulator()
    
    # Initial state should be grounded and disarmed
    assert sim.armed is False
    assert sim.z == 0.0
    
    # Test arming
    sim.arm()
    assert sim.armed is True
    
    # Test takeoff
    sim.takeoff(alt=10.0)
    assert sim.flight_mode == "ALT_HOLD"
    assert sim.target_alt == 10.0
    
    # Step physics simulation for 1 second (25 steps at dt=0.04s)
    dt = 0.04
    for _ in range(25):
        sim._step_physics(dt)
        
    # Drone should have gained altitude and positive motor thrust
    assert sim.z > 0.0
    assert sim.m1 > 0.0 and sim.m2 > 0.0
    
    # Test Emergency Cut
    sim.emergency_stop()
    assert sim.armed is False
    assert sim.emergency_cut is True
    assert sim.m1 == 0.0

@pytest.mark.asyncio
async def test_telemetry_manager_broadcast():
    tm = TelemetryManager()
    q = tm.subscribe()
    
    # Initial queue empty
    assert q.empty()
    
    # Update telemetry state
    await tm.update_state(roll=12.5, pitch=-5.2, altitude_agl=8.4)
    
    # Queue should receive updated state
    data = await asyncio.wait_for(q.get(), timeout=1.0)
    assert data["roll"] == 12.5
    assert data["pitch"] == -5.2
    assert data["altitude_agl"] == 8.4
    
    tm.unsubscribe(q)

def test_waypoint_math():
    sim = SITLQuadcopterSimulator()
    wp1 = Waypoint(id=1, lat=-6.176000, lon=106.828000, alt=20.0, speed=5.0)
    sim.load_mission([wp1])
    
    assert len(sim.mission_waypoints) == 1
    assert sim.mission_waypoints[0].id == 1

