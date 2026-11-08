import asyncio
import json
from fastapi import APIRouter, WebSocket, WebSocketDisconnect
from app.services.telemetry import telemetry_manager

router = APIRouter(prefix="/api/telemetry", tags=["Telemetry"])

@router.get("/state")
async def get_current_state():
    return telemetry_manager.get_state().model_dump()

@router.get("/logs")
async def get_flight_logs():
    return telemetry_manager.flight_logs

@router.websocket("/ws")
async def websocket_telemetry_stream(websocket: WebSocket):
    await websocket.accept()
    q = telemetry_manager.subscribe()
    try:
        # Send initial full state immediately
        initial = telemetry_manager.get_state().model_dump()
        await websocket.send_text(json.dumps(initial))
        
        while True:
            # Wait for next state update
            data = await q.get()
            await websocket.send_text(json.dumps(data))
    except (WebSocketDisconnect, asyncio.CancelledError):
        pass
    finally:
        telemetry_manager.unsubscribe(q)

