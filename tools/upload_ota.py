"""Invoke the framework's matching OTA client with local credentials in memory."""
import argparse
import importlib.util
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True)
    parser.add_argument("--host", required=True)
    parser.add_argument("--firmware", required=True)
    args = parser.parse_args()
    settings_path = Path(__file__).resolve().parents[1] / "network.local.json"
    try:
        settings = json.loads(settings_path.read_text(encoding="utf-8"))
    except (OSError, ValueError) as error:
        raise SystemExit("Cannot read network.local.json; configure OTA before uploading") from error
    password = settings.get("ota_password")
    if not isinstance(password, str) or len(password) < 12:
        raise SystemExit("Set an OTA password of at least 12 characters in network.local.json")
    spec = importlib.util.spec_from_file_location("espota", args.tool)
    if spec is None or spec.loader is None:
        raise SystemExit("Cannot load the ESP32 framework's OTA client")
    client = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(client)
    return client.main([
        "--progress", "--ip=" + args.host, "--auth=" + password, "--file=" + args.firmware,
    ])


if __name__ == "__main__":
    raise SystemExit(main())
