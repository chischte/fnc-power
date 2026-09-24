"""Keep the OTA password out of command lines and espota debug logs."""
Import("env")

if env.subst("$UPLOAD_PROTOCOL") == "espota":
    env.Replace(UPLOADCMD=(
        '"$PYTHONEXE" "$PROJECT_DIR/tools/upload_ota.py" '
        '--tool "$UPLOADER" --host "$UPLOAD_PORT" --firmware "$SOURCE"'
    ))
