"""Set the DM4 RTC to the PC's local time over the local network."""
import argparse
from datetime import datetime
import http.client
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("host", help="DM4 IP address or hostname")
    args = parser.parse_args()
    path = Path(__file__).resolve().parents[1] / "network.local.json"
    try:
        password = json.loads(path.read_text(encoding="utf-8"))["ota_password"]
    except (OSError, ValueError, KeyError):
        raise SystemExit("Cannot read OTA password from network.local.json")
    if not isinstance(password, str) or not password or any(c in password for c in "\r\n"):
        raise SystemExit("Invalid OTA password in network.local.json")
    connection = http.client.HTTPConnection(args.host, timeout=5)
    try:
        connection.connect()
        timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        connection.request("POST", "/time", body=b"", headers={
            "X-OTA-Password": password,
            "X-Local-Time": timestamp,
        })
        response = connection.getresponse()
        body = response.read(1024).decode("utf-8", errors="replace")
        if response.status != 200 or body != "OK TIME":
            raise SystemExit(f"Clock was not confirmed (HTTP {response.status})")
        print(f"OK TIME {timestamp} (PC local time; RTC write verified)")
    except (OSError, http.client.HTTPException):
        raise SystemExit("Cannot reach the DM4 time-setting endpoint")
    finally:
        connection.close()


if __name__ == "__main__":
    main()
