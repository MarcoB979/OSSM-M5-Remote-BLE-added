from __future__ import annotations

import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
FIRMWARE_DIR = ROOT / "build" / "firmware"
WEBFLASHER_DIR = ROOT / "webflasher"

PATTERNS = {
    "core2": re.compile(r"^OSSM-M5-Remote_m5stack-core2_(.+)\\.bin$"),
    "cores3": re.compile(r"^OSSM-M5-Remote_m5stack-cores3_(.+)\\.bin$"),
}


def score_version(tag: str) -> tuple[int, int, str]:
    # Prefer TEST-v0-### style tags, then lexical fallback.
    m = re.search(r"v0-(\\d+)$", tag)
    if m:
        return (2, int(m.group(1)), tag)
    m = re.search(r"(\\d+)$", tag)
    if m:
        return (1, int(m.group(1)), tag)
    return (0, 0, tag)


def latest_firmware(board_key: str) -> tuple[Path, str]:
    pattern = PATTERNS[board_key]
    candidates: list[tuple[tuple[int, int, str], Path, str]] = []

    for path in FIRMWARE_DIR.glob("*.bin"):
        match = pattern.match(path.name)
        if not match:
            continue
        tag = match.group(1)
        candidates.append((score_version(tag), path, tag))

    if not candidates:
        raise FileNotFoundError(f"No firmware found for {board_key} in {FIRMWARE_DIR}")

    candidates.sort(key=lambda x: x[0], reverse=True)
    _, path, tag = candidates[0]
    return path, tag


def write_manifest(file_name: str, display_name: str, chip_family: str, fw_rel_path: str, version_tag: str) -> None:
    manifest = {
        "name": display_name,
        "version": version_tag,
        "new_install_prompt_erase": True,
        "builds": [
            {
                "chipFamily": chip_family,
                "parts": [
                    {
                        "path": fw_rel_path,
                        "offset": 0,
                    }
                ],
            }
        ],
    }

    target = WEBFLASHER_DIR / file_name
    target.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {target}")


def main() -> None:
    core2_path, core2_tag = latest_firmware("core2")
    cores3_path, cores3_tag = latest_firmware("cores3")

    core2_rel = f"../build/firmware/{core2_path.name}"
    cores3_rel = f"../build/firmware/{cores3_path.name}"

    write_manifest(
        "manifest-core2.json",
        "OSSM M5 Remote - Core2",
        "ESP32",
        core2_rel,
        core2_tag,
    )
    write_manifest(
        "manifest-cores3.json",
        "OSSM M5 Remote - CoreS3",
        "ESP32-S3",
        cores3_rel,
        cores3_tag,
    )


if __name__ == "__main__":
    main()
