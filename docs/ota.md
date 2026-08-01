# OTA updates

Firmware **1.1.0+** supports two OTA paths. Flash once over USB with the dual-OTA partition table, then update wirelessly.

## First flash (USB required)

Partition layout changed to dual OTA (`app0` / `app1`). Do a full USB flash once:

```bash
pio run -t upload
pio run -t uploadfs
```

## 1) Web UI upload

1. Open the station dashboard → **Setup** → **OTA update**.
2. Choose **Firmware** or **Filesystem (LittleFS)**.
3. Select the `.bin` and **Upload & flash**.

Build artifacts:

```bash
pio run                    # → .pio/build/esp32-s3-devkitc-1/firmware.bin
pio run -t buildfs         # → .pio/build/esp32-s3-devkitc-1/littlefs.bin
```

## 2) PlatformIO network upload (ArduinoOTA)

With the device on WiFi (hostname `weather-station.local`):

```bash
pio run -e esp32-s3-ota -t upload
```

If mDNS fails, use the IP:

```bash
pio run -e esp32-s3-ota -t upload --upload-port 192.168.x.x
```

## Notes

- OTA writes the inactive app slot, then reboots into it.
- Keep WiFi stable during upload; a failed flash leaves the previous slot bootable.
- Web OTA has no password — use only on a trusted LAN (same as the setup UI).
