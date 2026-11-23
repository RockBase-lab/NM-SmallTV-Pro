#!/usr/bin/env python3
"""Build the NM-TV-154 factory themes from the SmallTV-Ultra reference assets.

This desktop tool resizes RGB pictures, samples long animations, and emits
ROM-decodable baseline JPEG pictures inside validated .stheme
packages plus reproducible source directories.
"""
import argparse
import json
from pathlib import Path
import os
import shutil
import sys

from PIL import Image, ImageSequence

sys.path.insert(0, str(Path(__file__).resolve().parent))
import theme_pack

# Local resource preparation can proceed on a machine without a desktop C++
# compiler. Normal/CI runs always use the firmware's native manifest/package
# validator; the explicit escape hatch only skips that duplicate local check.
if os.environ.get("SMALLTV_SKIP_NATIVE_VALIDATION") == "1":
    theme_pack.validate_manifest = lambda path: None
    theme_pack.validate_package = lambda path: None
build = theme_pack.build


PROJECT = Path(__file__).resolve().parents[1]
DEFAULT_INPUT = PROJECT / "docs/product/assets/smalltv-ultra/pictures"
DEFAULT_OUTPUT = PROJECT / "data/nm_tv_154"
DEFAULT_SOURCE = PROJECT / "assets/nm_tv_154/themes"


def manifest(theme_id, name, layers):
    return {
        "spec": 2,
        "theme": {"id": theme_id, "name": name, "author": "SmallTV", "version": "1.0.0" if theme_id.startswith("classic-") else "2.0.0"},
        "display": {"width": 240, "height": 240, "background": "#000000"},
        "layers": layers,
    }


def clock_layers(hour="HH", date="{YYYY}/{MO}/{DD}", suffix=""):
    value = "{" + hour + "}:{MM}" + suffix
    return [
        {"id": "time", "type": "text", "x": 120, "y": 91, "anchor": "center",
         "value": value, "size": 48 if not suffix else 40, "color": "#ffffff"},
        {"id": "date", "type": "text", "x": 120, "y": 145, "anchor": "center",
         "value": date, "size": 16, "color": "#00c8c8"},
    ]


def prepare_image(image, size):
    image = image.convert("RGB")
    image.thumbnail((size, size), Image.Resampling.LANCZOS)
    canvas = Image.new("RGB", (size, size), "black")
    canvas.paste(image, ((size - image.width) // 2, (size - image.height) // 2))
    return canvas


def write_manifest(directory, data):
    (directory / "theme.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def build_classics(source_root, output_root):
    specs = [
        ("classic-24", "Classic 24", manifest("classic-24", "Classic 24", clock_layers())),
        ("classic-12", "Classic 12", manifest("classic-12", "Classic 12",
            clock_layers("hh", "{WD} {DD} {MON}", " {AMPM}"))),
    ]
    results = []
    for theme_id, name, data in specs:
        directory = source_root / theme_id
        directory.mkdir(parents=True, exist_ok=True)
        write_manifest(directory, data)
        package = output_root / "themes" / f"{theme_id}.stheme"
        package.parent.mkdir(parents=True, exist_ok=True)
        package.write_bytes(build(directory))
        results.append({"id": theme_id, "name": name, "bytes": package.stat().st_size, "source": None})
    return results


def build_picture(source, source_root, output_root, sequence, size, max_frames):
    is_gif = source.suffix.lower() == ".gif"
    theme_id = f"ultra-{sequence:02d}"
    name = f"Ultra {'Motion' if is_gif else 'Picture'} {sequence:02d}"
    # Every reference asset is installed once. Pixel Motion is a pure fullscreen
    # animation too: the Pictures screensaver must not overlay a clock or date.
    if source.name == "ezgif-5-acf59aa68c.gif":
        theme_id = "pixel-motion"
        name = "Pixel Motion"
    directory = source_root / theme_id
    directory.mkdir(parents=True, exist_ok=True)
    with Image.open(source) as image:
        frames = [frame.copy() for frame in ImageSequence.Iterator(image)] if is_gif else [image.copy()]
    if is_gif and len(frames) > max_frames:
        indices = [round(i * (len(frames) - 1) / (max_frames - 1)) for i in range(max_frames)]
        frames = [frames[i] for i in indices]
    if is_gif:
        asset_dir = directory / "animations/motion"
        asset_dir.mkdir(parents=True, exist_ok=True)
        for i, frame in enumerate(frames):
            prepare_image(frame, size).save(asset_dir / f"{i:03d}.png", optimize=True)
        layers = [{"id": "motion", "type": "animation", "x": (240 - size) // 2,
                   "y": (240 - size) // 2, "source": "animations/motion", "frames": len(frames),
                   "fps": 4, "loop": True, "width": size, "height": size}]
    else:
        image_dir = directory / "images"
        image_dir.mkdir(parents=True, exist_ok=True)
        prepare_image(frames[0], size).save(image_dir / "picture.png", optimize=True)
        layers = [{"id": "picture", "type": "image", "x": (240 - size) // 2,
                   "y": (240 - size) // 2, "source": "images/picture.png"}]
    write_manifest(directory, manifest(theme_id, name, layers))
    package = output_root / "themes" / f"{theme_id}.stheme"
    package.parent.mkdir(parents=True, exist_ok=True)
    package.write_bytes(build(directory, jpeg=True, jpeg_quality=55))
    return {"id": theme_id, "name": name, "bytes": package.stat().st_size,
            "source": source.name, "sourceFrames": len(frames), "width": size, "height": size,
            "encoding": "JPEG RGB"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--source-output", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--size", type=int, default=240, choices=range(80, 241), metavar="80..240")
    parser.add_argument("--frames", type=int, default=2, choices=range(2, 17), metavar="2..16")
    args = parser.parse_args()
    inputs = sorted(p for p in args.input.iterdir() if p.suffix.lower() in (".jpg", ".gif"))
    if len(inputs) != 10:
        parser.error(f"Expected the 10 reference JPG/GIF files in {args.input}, found {len(inputs)}")
    # These directories are wholly generated by this tool; hand edits belong in
    # the reference assets or this script, not in generated PNG/theme files.
    for directory in (args.output, args.source_output):
        if directory.exists():
            shutil.rmtree(directory)
    results = build_classics(args.source_output, args.output)
    for sequence, source in enumerate(inputs, 1):
        results.append(build_picture(source, args.source_output, args.output, sequence,
                                     args.size, args.frames))
    record = {"imageSize": args.size, "maxFrames": args.frames, "encoding": "JPEG RGB",
              "packageBytes": sum(item["bytes"] for item in results), "themes": results}
    (args.source_output / "builtin-themes.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
    for item in results:
        print(f'{item["id"]}: {item["bytes"]} bytes')
    print(f'Total package payload: {record["packageBytes"]} bytes')


if __name__ == "__main__":
    main()
