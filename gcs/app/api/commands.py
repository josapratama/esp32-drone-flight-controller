from typing import List, Optional
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
from app.services.sitl_simulator import sitl_sim
from app.services.mavlink_handler import telemetry_bridge
from app.services.telemetry import telemetry_manager, Waypoint

router = APIRouter(prefix="/api/commands", tags=["Commands"])

class ArmRequest(BaseModel):
    arm: bool = True

class FlightModeRequest(BaseModel):
    mode: str  # STABILIZE, ALT_HOLD, LOITER, AUTO, RTL, LAND

class TakeoffRequest(BaseModel):
    altitude: float = 10.0

class RCStickRequest(BaseModel):
    roll: float = 0.0      # -1.0 to 1.0
    pitch: float = 0.0     # -1.0 to 1.0
    yaw: float = 0.0       # -1.0 to 1.0
    throttle: float = 0.0  # 0.0 to 1.0

class MissionUploadRequest(BaseModel):
    waypoints: List[Waypoint]

@router.post("/arm")
async def command_arm(req: ArmRequest):
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        if req.arm:
            sitl_sim.arm()
        else:
            sitl_sim.disarm()
        return {"status": "success", "armed": sitl_sim.armed}
    else:
        success, msg = await telemetry_bridge.send_command("ARM" if req.arm else "DISARM")
        return {"status": "success" if success else "error", "message": msg}

@router.post("/mode")
async def command_set_mode(req: FlightModeRequest):
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.set_mode(req.mode)
        return {"status": "success", "mode": sitl_sim.flight_mode}
    else:
        success, msg = await telemetry_bridge.send_command("SET_MODE", mode=req.mode)
        return {"status": "success" if success else "error", "message": msg}

@router.post("/takeoff")
async def command_takeoff(req: TakeoffRequest):
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.takeoff(req.altitude)
        return {"status": "success", "altitude": req.altitude}
    else:
        success, msg = await telemetry_bridge.send_command("TAKEOFF", alt=req.altitude)
        return {"status": "success" if success else "error", "message": msg}

@router.post("/rtl")
async def command_rtl():
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.set_mode("RTL")
        return {"status": "success", "mode": "RTL"}
    else:
        success, msg = await telemetry_bridge.send_command("RTL")
        return {"status": "success" if success else "error", "message": msg}

@router.post("/land")
async def command_land():
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.set_mode("LAND")
        return {"status": "success", "mode": "LAND"}
    else:
        success, msg = await telemetry_bridge.send_command("LAND")
        return {"status": "success" if success else "error", "message": msg}

@router.post("/emergency-cut")
async def command_emergency_cut():
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.emergency_stop()
        return {"status": "emergency_activated", "message": "Motors killed instantly"}
    else:
        success, msg = await telemetry_bridge.send_command("EMERGENCY_CUT")
        return {"status": "success" if success else "error", "message": msg}

@router.post("/emergency-reset")
async def command_emergency_reset():
    sitl_sim.reset_emergency()
    return {"status": "success", "message": "Emergency lock reset"}

@router.post("/rc")
async def command_rc_sticks(req: RCStickRequest):
    state = telemetry_manager.get_state()
    if state.connection_type == "sitl":
        sitl_sim.set_rc(req.roll, req.pitch, req.yaw, req.throttle)
        return {"status": "success"}
    else:
        await telemetry_bridge.send_command(
            "SET_RC",
            roll=req.roll,
            pitch=req.pitch,
            yaw=req.yaw,
            throttle=req.throttle
        )
        return {"status": "success"}

@router.post("/mission")
async def upload_mission(req: MissionUploadRequest):
    telemetry_manager.waypoints = req.waypoints
    sitl_sim.load_mission(req.waypoints)
    state = telemetry_manager.get_state()
    if state.connection_type != "sitl":
        await telemetry_bridge.send_command("UPLOAD_MISSION", waypoints=[wp.model_dump() for wp in req.waypoints])
    return {"status": "success", "count": len(req.waypoints)}

@router.delete("/mission")
async def clear_mission():
    telemetry_manager.waypoints = []
    sitl_sim.load_mission([])
    return {"status": "success", "message": "Mission cleared"}

