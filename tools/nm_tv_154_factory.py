"""Add the generated NM-TV-154 LittleFS image to the merged factory binary.

PlatformIO's normal app image remains app-only for OTA. The factory image also
contains the partition's LittleFS data, so its preinstalled clock/picture themes
are available immediately after USB flashing.
"""
from pathlib import Path

Import("env")


def littlefs_partition(partition_file):
    for line in Path(partition_file).read_text(encoding="utf-8").splitlines():
        fields = [field.strip() for field in line.split(",")]
        if len(fields) >= 5 and fields[0] == "spiffs" and fields[1] == "data":
            return int(fields[3], 0), int(fields[4], 0)
    raise ValueError("NM-TV-154 partition table has no data/spiffs LittleFS partition")


project_dir = Path(env.subst("$PROJECT_DIR"))
partition_file = project_dir / "partitions" / "nm_tv_154_4mb_ota.csv"
offset, size = littlefs_partition(partition_file)
data_dir = project_dir / "data" / "nm_tv_154"
if not (data_dir / "themes" / "classic-24.stheme").is_file():
    raise ValueError("Missing generated NM-TV-154 themes; run tools/build_nm_tv_154_assets.py")

# DataToBin is available to post scripts after the platform builder has loaded.
# Making the app depend on this target builds the FS before the platform's own
# post-action merges the factory image; FLASH_EXTRA_IMAGES is read at that time.
fs_image = env.DataToBin("$BUILD_DIR/${ESP32_FS_IMAGE_NAME}", str(data_dir))
env.Depends(fs_image, [env.File(str(path)) for path in data_dir.rglob("*") if path.is_file()])
env.Depends("$BUILD_DIR/${PROGNAME}.bin", fs_image)
env.Append(FLASH_EXTRA_IMAGES=[[hex(offset), "$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin"]])
print(f"NM-TV-154 factory LittleFS: {len(list((data_dir / 'themes').glob('*.stheme')))} themes, "
      f"offset {hex(offset)}, partition {size} bytes")
