"""Build local network settings into firmware; keep credentials out of Git."""
import json
from pathlib import Path

Import("env")

root = Path(env.subst("$PROJECT_DIR"))
config_path = root / "network.local.json"
if not config_path.exists():
    config_path = root / "network.example.json"
config = json.loads(config_path.read_text(encoding="utf-8"))
strings = ("hostname", "wifi_ssid", "wifi_password", "ota_password")
for key in strings:
    if not isinstance(config.get(key), str) or "\0" in config[key]:
        raise ValueError(f"{key} must be a string without NUL characters")
hostname = config["hostname"]
if not hostname or len(hostname) > 63 or any(
    char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-"
    for char in hostname
) or hostname.startswith("-") or hostname.endswith("-"):
    raise ValueError("hostname must be a valid DNS label")
if not isinstance(config.get("ethernet"), bool):
    raise ValueError("ethernet must be true or false")
password = config["ota_password"]
if password and len(password) < 12:
    raise ValueError("Use at least 12 characters for ota_password, or leave it empty to disable OTA")

generated = Path(env.subst("$BUILD_DIR")) / "generated"
generated.mkdir(parents=True, exist_ok=True)
credentials_path = next((path for path in (
    root / "wifi-credentials.h", root.parent / "wifi-credentials.h"
) if path.is_file()), None)
header = "#pragma once\n"
if credentials_path:
    header += '#include "' + credentials_path.as_posix() + '"\n'
header += "namespace NetworkConfig {\n"
header += "constexpr bool ethernet = " + str(config["ethernet"]).lower() + ";\n"
for key in strings:
    if credentials_path and key in ("wifi_ssid", "wifi_password"):
        value = "WIFI_SSID" if key == "wifi_ssid" else "WIFI_PASSWORD"
    else:
        value = json.dumps(config[key], ensure_ascii=False)
    header += f"constexpr char {key}[] = {value};\n"
header += "}\n"
target = generated / "NetworkConfig.h"
if not target.exists() or target.read_text(encoding="utf-8") != header:
    target.write_text(header, encoding="utf-8")
env.Append(CPPPATH=[str(generated)])
if env.subst("$UPLOAD_PROTOCOL") == "espota":
    if not password:
        raise ValueError("Set ota_password in network.local.json before using the OTA target")
