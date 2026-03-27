# Build Troubleshooting (ESP32 Toolchain)

If you see this error:

`'xtensa-esp32-elf-g++' is not recognized as an internal or external command`
`or having problems building/uploading your firmware`

follow this checklist.

## Checklist

1. Confirm you are building the correct environment:
   - `pio run -e m5stack-core2`

2. Update PlatformIO Core:
   - `pio upgrade`

3. Reinstall ESP32 platform packages:
   - `pio platform uninstall espressif32`
   - `pio platform install espressif32`

4. If still broken, remove cached toolchain/platform and reinstall:
   - Delete `%USERPROFILE%\.platformio\packages\toolchain-xtensa-esp32*`
   - Delete `%USERPROFILE%\.platformio\platforms\espressif32`
   - Run: `pio platform install espressif32`

5. Check antivirus/Defender quarantine:
   - Whitelist `%USERPROFILE%\.platformio\`
   - Reinstall platform after whitelisting

6. Verify compiler file exists:
   - `%USERPROFILE%\.platformio\packages\toolchain-xtensa-esp32\bin\xtensa-esp32-elf-g++.exe`

7. Retry a clean build:
   - `pio run -e m5stack-core2 -t clean`
   - `pio run -e m5stack-core2`

## If It Still Fails

Please share:

- Full build log
- `pio --version`
- `pio platform list`
- `pio pkg list -g`


## Possible workaround:

In platformio.ini:

- Replace text
- `platform = espressif32`
- `to:`
- `platform = espressif32@6.5.0`
