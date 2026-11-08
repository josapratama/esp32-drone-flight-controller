import glob
import os
import sys
from fastapi import APIRouter
from pydantic import BaseModel
from app.services.sitl_simulator import sitl_sim
from app.services.mavlink_handler import telemetry_bridge
from app.services.telemetry import telemetry_manager

router = APIRouter(prefix="/api/connection", tags=["Connection"])

class ConnectionRequest(BaseModel):
    type: str  # "sitl", "serial", "udp", "tcp"
    port: str = "/dev/ttyUSB0"
    baudrate: int = 115200
    udp_port: int = 14550
    tcp_host: str = "192.168.4.1"
    tcp_port: int = 8080

def list_serial_ports():
    ports = []
    if sys.platform.startswith("win"):
        import serial.tools.list_ports
        ports = [p.device for p in serial.tools.list_ports.comports()]
    else:
        # Linux / Raspberry Pi
        ports = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*") + glob.glob("/dev/ttyAMA*")
    return ports

@router.get("/ports")
async def get_available_ports():
    return {"ports": list_serial_ports()}

@router.post("/connect")
async def connect_telemetry(req: ConnectionRequest):
    if req.type == "sitl":
        await telemetry_bridge.disconnect()
        sitl_sim.start()
        await telemetry_manager.update_state(
            connected=True,
            connection_type="sitl",
            source_address="Internal SITL 6-DOF Physics"
        )
        return {"status": "connected", "type": "sitl", "target": "SITL Physics Engine"}

    elif req.type == "serial":
        sitl_sim.stop()
        success, msg = await telemetry_bridge.connect_serial(req.port, req.baudrate)
        return {"status": "connected" if success else "error", "type": "serial", "message": msg}

    elif req.type == "udp":
        sitl_sim.stop()
        success, msg = await telemetry_bridge.connect_udp("0.0.0.0", req.udp_port)
        return {"status": "connected" if success else "error", "type": "udp", "message": msg}

    elif req.type == "tcp":
        sitl_sim.stop()
        success, msg = await telemetry_bridge.connect_tcp(req.tcp_host, req.tcp_port)
        return {"status": "connected" if success else "error", "type": "tcp", "message": msg}

    return {"status": "error", "message": "Unknown connection type"}

@router.post("/disconnect")
async def disconnect_telemetry():
    sitl_sim.stop()
    await telemetry_bridge.disconnect()
    await telemetry_manager.update_state(
        connected=False,
        connection_type="none",
        source_address="Disconnected",
        armed=False
    )
    return {"status": "disconnected"}

