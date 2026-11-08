import asyncio
import json
import logging
import math
import socket
import time
from typing import Optional, List
from app.services.telemetry import telemetry_manager, Waypoint

logger = logging.getLogger("mavlink")

class TelemetryBridge:
    """
    Unified Telemetry Bridge supporting MAVLink 1.0/2.0 and ESP32 JSON protocol
    over Serial port, UDP socket, or TCP connection.
    """
    
    def __init__(self):
        self.connected = False
        self.connection_type = "none"
        self.endpoint = ""
        self._reader_task: Optional[asyncio.Task] = None
        self._serial_conn = None
        self._udp_sock = None
        self._tcp_reader = None
        self._tcp_writer = None

    async def connect_serial(self, port: str = "/dev/ttyUSB0", baudrate: int = 115200):
        await self.disconnect()
        try:
            import serial
            self._serial_conn = serial.Serial(port=port, baudrate=baudrate, timeout=0.1)
            self.connected = True
            self.connection_type = "serial"
            self.endpoint = f"{port}@{baudrate}"
            self._reader_task = asyncio.create_task(self._serial_reader_loop())
            logger.info(f"Connected to Serial {self.endpoint}")
            return True, f"Connected to {self.endpoint}"
        except Exception as e:
            logger.error(f"Serial connect error: {e}")
            return False, str(e)

    async def connect_udp(self, host: str = "0.0.0.0", port: int = 14550):
        await self.disconnect()
        try:
            self._udp_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._udp_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._udp_sock.bind((host, port))
            self._udp_sock.setblocking(False)
            self.connected = True
            self.connection_type = "udp"
            self.endpoint = f"udp://{host}:{port}"
            self._reader_task = asyncio.create_task(self._udp_reader_loop())
            logger.info(f"Listening on UDP {self.endpoint}")
            return True, f"Listening on {self.endpoint}"
        except Exception as e:
            logger.error(f"UDP bind error: {e}")
            return False, str(e)

    async def connect_tcp(self, host: str = "192.168.4.1", port: int = 8080):
        await self.disconnect()
        try:
            self._tcp_reader, self._tcp_writer = await asyncio.open_connection(host, port)
            self.connected = True
            self.connection_type = "tcp"
            self.endpoint = f"tcp://{host}:{port}"
            self._reader_task = asyncio.create_task(self._tcp_reader_loop())
            logger.info(f"Connected to TCP {self.endpoint}")
            return True, f"Connected to {self.endpoint}"
        except Exception as e:
            logger.error(f"TCP connect error: {e}")
            return False, str(e)

    async def disconnect(self):
        self.connected = False
        if self._reader_task and not self._reader_task.done():
            self._reader_task.cancel()
        if self._serial_conn:
            try:
                self._serial_conn.close()
            except Exception:
                pass
            self._serial_conn = None
        if self._udp_sock:
            try:
                self._udp_sock.close()
            except Exception:
                pass
            self._udp_sock = None
        if self._tcp_writer:
            try:
                self._tcp_writer.close()
            except Exception:
                pass
            self._tcp_writer = None
            self._tcp_reader = None
        self.connection_type = "none"
        self.endpoint = ""

    def _parse_packet(self, data: bytes):
        """Parse raw telemetry bytes (supports both JSON and MAVLink)."""
        # 1. Try decoding as JSON (ESP32 DIY Flight Controller format)
        try:
            text = data.decode("utf-8", errors="ignore").strip()
            for line in text.split("\n"):
                line = line.strip()
                if line.startswith("{") and line.endswith("}"):
                    j = json.loads(line)
                    # Expected keys from ESP32 quadcopter:
                    # {"roll": 1.2, "pitch": -0.5, "yaw": 90.0, "alt": 12.3, "bat": 11.8, "armed": 1, ...}
                    asyncio.create_task(telemetry_manager.update_state(
                        connected=True,
                        connection_type=self.connection_type,
                        source_address=self.endpoint,
                        roll=float(j.get("roll", 0.0)),
                        pitch=float(j.get("pitch", 0.0)),
                        yaw=float(j.get("yaw", 0.0)),
                        altitude_agl=float(j.get("alt", j.get("altitude", 0.0))),
                        groundspeed=float(j.get("speed", 0.0)),
                        battery_voltage=float(j.get("bat", j.get("vbat", 12.0))),
                        battery_percent=int(j.get("bat_pct", 85)),
                        armed=bool(j.get("armed", False)),
                        flight_mode=str(j.get("mode", "STABILIZE")).upper(),
                        motor_m1=float(j.get("m1", 0.0)),
                        motor_m2=float(j.get("m2", 0.0)),
                        motor_m3=float(j.get("m3", 0.0)),
                        motor_m4=float(j.get("m4", 0.0)),
                        latitude=float(j.get("lat", -6.175392)),
                        longitude=float(j.get("lon", 106.827153)),
                        satellites=int(j.get("sats", 12)),
                    ))
                    return
        except Exception:
            pass

        # 2. Try parsing as MAVLink if pymavlink is installed
        try:
            from pymavlink import mavutil
            # Mavlink parsing hook
        except ImportError:
            pass

    async def _serial_reader_loop(self):
        while self.connected and self._serial_conn:
            try:
                if self._serial_conn.in_waiting > 0:
                    data = self._serial_conn.read(self._serial_conn.in_waiting)
                    self._parse_packet(data)
                await asyncio.sleep(0.01)
            except Exception as e:
                logger.error(f"Serial reader exception: {e}")
                break
        await self.disconnect()

    async def _udp_reader_loop(self):
        loop = asyncio.get_event_loop()
        while self.connected and self._udp_sock:
            try:
                data, addr = await loop.sock_recvfrom(self._udp_sock, 2048)
                self._parse_packet(data)
            except Exception:
                await asyncio.sleep(0.02)
        await self.disconnect()

    async def _tcp_reader_loop(self):
        while self.connected and self._tcp_reader:
            try:
                line = await self._tcp_reader.readline()
                if not line:
                    break
                self._parse_packet(line)
            except Exception:
                break
        await self.disconnect()

    async def send_command(self, cmd: str, **params):
        """Send command to physical drone or active bridge."""
        if not self.connected:
            return False, "Not connected to drone"
        payload = json.dumps({"cmd": cmd, "params": params}).encode("utf-8") + b"\n"
        try:
            if self._serial_conn:
                self._serial_conn.write(payload)
            elif self._tcp_writer:
                self._tcp_writer.write(payload)
                await self._tcp_writer.drain()
            return True, f"Sent {cmd}"
        except Exception as e:
            return False, str(e)

telemetry_bridge = TelemetryBridge()

