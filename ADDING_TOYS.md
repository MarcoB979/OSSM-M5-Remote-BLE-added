# Adding a toy to the OSSM M5 Remote

Two ways to add a toy, depending on how comfortable you are with technology:

- **Beginner (no programming)** — add a toy from a web page in your browser,
  no code, no rebuild. The toy is saved on the M5 and survives restarts and
  updates. **Most people should use this.**
- **Developer** — add a new toy *family* directly in the firmware source.

---

## Part 1 — For beginners (no programming)

You need: the M5 connected to WiFi, and a phone with a free BLE scanner app
(**nRF Connect** is recommended).

### Step 1 — Find your toy's name

1. Turn the toy on.
2. Open nRF Connect and scan for BLE devices.
3. Note the toy's **name** (or just its first few letters). Examples:
   - Lovense toys start with `LVS-` or `LOVE-`
   - We-Vibe toys start with `WP-`

### Step 2 — Open the M5's toy page

1. Make sure the M5 is on your WiFi (Settings → WiFi on the M5, if you haven't
   set it up yet).
2. In a browser on the same WiFi, go to `http://m5-remote.local/toys`.
   (If your device doesn't resolve `.local` names, use the M5's IP address —
   shown in Settings → WiFi and printed in the serial log — instead.)

### Step 3 — Add the toy

1. Type the name prefix from Step 1 into **"Advertised name starts with"**.
2. Pick what the toy does from the **"What it does"** dropdown. For a simple
   vibrator, leave it on **Vibrate** — the min/max and command text fill in
   automatically.
3. Press **Add toy**.

### Step 4 — Connect

1. Turn the toy on.
2. Press **Scan now** on the page (or wait ~20 seconds — it auto-scans).
3. On the M5, open **Toys** (in the Addons menu) to control it.

> **Finding the command text for unusual toys:** look your toy up in the
> buttplug.io device list, copy its command template (e.g. `Vibrate:%d;`), and
> paste it into the **Command text** box. The `%d` is where the number goes.

---

## Part 2 — For developers

A "toy family" is a BLE name prefix plus a feature list. The runtime profile
system (Part 1) already covers most one-feature toys without any code. Add code
when you need a toy family with **multiple features** or **special connection
logic**.

### How a toy becomes compatible

The M5 connects to the toy over BLE and writes command strings (e.g.
`Vibrate:12;`) to its command characteristic. Requirements:

1. The toy must be **BLE** (a phone can discover it).
2. It must use a **documented command protocol**.

> The M5 is the Buttplug *device server* — anything it controls itself must go
> through this direct-BLE path. Intiface handles other toys on the PC side.

### Where the code lives

Everything is in `connectToy()` in `src/network/ToyHub.cpp`.

1. **Name match** — add a prefix branch next to Lovense:
   ```cpp
   if (name.startsWith("LVS-") || name.startsWith("LOVE-")) {
       features = { /* Lovense vibrate + rotate */ };
   } else if (name.startsWith("WP-")) {
       features = { /* We-Vibe feature list */ };
   } else { /* fall through to runtime profiles */ }
   ```
2. **Feature list** — the only toy-specific part. Each entry:
   ```cpp
   { ToyFeatureType, minValue, maxValue, "command-template", initialValue, useRotateCmd, bpIndex }
   ```
   The on-device screen and the Buttplug registration build themselves from
   this list. Available types: `Vibrate, Rotate, Oscillate, Constrict, Spray,
   Temperature, Led, Position`.
3. **Characteristic selection** — Lovense uses the first `canWrite()`
   characteristic. If a toy needs a specific UUID, match that instead.

### Runtime profiles (the data model behind Part 1)

- `src/network/ToyProfiles.h/.cpp` — persists toy profiles in NVS (namespace
  `toys`), survives reboots and OTA. One feature per profile.
- `src/network/ToyConfigWeb.h/.cpp` — the `/toys` web page (add/delete/scan).
- `src/network/ToyHub.cpp` `connectToy()` — checks built-in families first,
  then `toyProfilesMatch()` for runtime profiles.

### Build

```powershell
platformio run -e m5stack-core2 -e m5stack-cores3
```

Only build — never upload/monitor from the build tooling.

---

## What to report back for a new toy

- The advertised **name prefix**.
- If non-obvious: the **GATT service/characteristic UUIDs** and **command
  strings** seen in nRF Connect.

> Key check: does the toy show up with a **name** at all in a BLE scan? Toys
> that require a proprietary app and don't expose a plain BLE GATT interface
> won't work with direct BLE control.
