import shutil
import re
import os
import hashlib

Import("env")

OUTPUT_DIR = "build{}".format(os.path.sep)

APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
MERGED_BIN = "$BUILD_DIR/merged-flash.bin"


def readFlag(flag):
    buildFlags = env.ParseFlags(env["BUILD_FLAGS"])

    for define in buildFlags.get("CPPDEFINES"):
        if define == flag:
            return None

        if isinstance(define, list) and define[0] == flag:
            cleanedFlag = re.sub(r'^"|"$', '', str(define[1]))
            return cleanedFlag

    return None


def bin_copy(source, target, env):
    app_version = readFlag("APP_VERSION") or "unknown"
    app_name = readFlag("APP_NAME") or "firmware"
    build_target = env.get("PIOENV") or "unknown"

    print("App Version: " + app_version)
    print("App Name: " + app_name)
    print("Build Target: " + build_target)

    variant = app_name + "_" + build_target + "_" + app_version.replace(".", "-")

    firmware_dir = "{}firmware".format(OUTPUT_DIR)

    os.makedirs(OUTPUT_DIR, exist_ok=True)
    os.makedirs(firmware_dir, exist_ok=True)

    root_bin_file = "{}{}{}.bin".format(OUTPUT_DIR, os.path.sep, variant)
    root_md5_file = "{}{}{}.md5".format(OUTPUT_DIR, os.path.sep, variant)
    bin_file = "{}{}{}.bin".format(firmware_dir, os.path.sep, variant)
    md5_file = "{}{}{}.md5".format(firmware_dir, os.path.sep, variant)

    for f in [root_bin_file, root_md5_file, bin_file, md5_file]:
        if os.path.isfile(f):
            os.remove(f)

    merged_bin = env.subst(MERGED_BIN)

    if not os.path.isfile(merged_bin):
        print("ERROR: merged firmware not found: " + merged_bin)
        print("Make sure mergeb_bin.py runs before rename_fw.py")
        env.Exit(1)

    print("Copying merged firmware to " + bin_file)

    shutil.copy(merged_bin, bin_file)
    print("Copying merged firmware to " + root_bin_file)
    shutil.copy(merged_bin, root_bin_file)

    with open(bin_file, "rb") as f:
        result = hashlib.md5(f.read()).hexdigest()

    print("Calculating MD5: " + result)

    with open(md5_file, "w") as file1:
        file1.write(result)

    with open(root_md5_file, "w") as file1:
        file1.write(result)


env.AddPostAction(MERGED_BIN, bin_copy)
env.AddPostAction("upload", bin_copy)