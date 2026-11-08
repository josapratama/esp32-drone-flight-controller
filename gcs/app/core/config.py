import os
from pydantic import BaseModel

class Settings(BaseModel):
    APP_NAME: str = "Web 3D Drone GCS"
    VERSION: str = "1.0.0"
    HOST: str = os.getenv("GCS_HOST", "0.0.0.0")
    PORT: int = int(os.getenv("GCS_PORT", "8090"))
    
    # Telemetry connection defaults
    DEFAULT_CONNECTION_TYPE: str = os.getenv("DEFAULT_CONN_TYPE", "sitl")  # sitl | serial | udp | tcp
    DEFAULT_SERIAL_PORT: str = os.getenv("SERIAL_PORT", "/dev/ttyUSB0")
    DEFAULT_BAUDRATE: int = int(os.getenv("SERIAL_BAUD", "115200"))
    DEFAULT_UDP_PORT: int = int(os.getenv("UDP_PORT", "14550"))
    DEFAULT_TCP_HOST: str = os.getenv("TCP_HOST", "192.168.4.1")
    DEFAULT_TCP_PORT: int = int(os.getenv("TCP_PORT", "8080"))
    
    # Default Home GPS (Monas Jakarta)
    DEFAULT_HOME_LAT: float = float(os.getenv("HOME_LAT", "-6.175392"))
    DEFAULT_HOME_LON: float = float(os.getenv("HOME_LON", "106.827153"))
    DEFAULT_HOME_ALT: float = float(os.getenv("HOME_ALT", "15.0"))
    
    # Telemetry streaming rate (Hz)
    TELEMETRY_HZ: int = int(os.getenv("TELEMETRY_HZ", "20"))

settings = Settings()

