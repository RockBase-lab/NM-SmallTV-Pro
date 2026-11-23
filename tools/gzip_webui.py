#!/usr/bin/env python3
"""Regenerate src/webui.h from src/webui.html.

The web UI ships as a gzip-compressed byte array in PROGMEM and is served with
`Content-Encoding: gzip` (see handleRoot in src/WebPortal.cpp). src/webui.html
is the source of truth — edit it, then rerun:

    python tools/gzip_webui.py

Install the pinned Node minifier once with `npm ci --prefix tools`.
The build bundles src/webui-search.js and minifies JavaScript without
renaming globals used by inline controls. Output has gzip mtime=0.
`--check` compares decompressed output with a fresh canonical bundle,
independently of the gzip implementation.

For the compact NM-TV-154 release, install zopfli and use --small to produce
a smaller gzip stream. The firmware needs no extra decoder.
"""
import gzip
import pathlib
import re
import sys
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
SRC = ROOT / "src" / "webui.html"
OUT = ROOT / "src" / "webui.h"

HEADER = """\
// GENERATED FILE -- do not edit by hand.
// Produced from src/webui.html and src/webui-search.js using pinned Terser.
// Regenerate after editing the HTML:  python tools/gzip_webui.py
//
// Gzip-compressed single-page config UI, served from PROGMEM with
// `Content-Encoding: gzip` by handleRoot in WebPortal.cpp.
#pragma once
#include <Arduino.h>

static const uint8_t WEBUI_HTML_GZ[] PROGMEM = {
"""


def html() -> bytes:
    # The repo stores LF; a Windows checkout with core.autocrlf has CRLF.
    raw = SRC.read_bytes().replace(b"\r\n", b"\n")
    search = (ROOT / "src" / "webui-search.js").read_bytes().replace(b"\r\n", b"\n")
    raw = raw.replace(b'<script src="webui-search.js"></script>', b'<script>' + search + b'</script>')
    return subprocess.run(["node", str(ROOT / "tools" / "minify_webui.cjs")], input=raw,
                          stdout=subprocess.PIPE, check=True).stdout


def check() -> int:
    body = OUT.read_text(encoding="utf-8").split("PROGMEM = {", 1)[1]
    blob = bytes(int(b, 16) for b in re.findall(r"0x([0-9a-f]{2})", body))
    if gzip.decompress(blob) == html():
        print(f"{OUT.name} matches {SRC.name}")
        return 0
    print(f"{OUT.name} is out of date: run python tools/gzip_webui.py")
    return 1


def main() -> None:
    raw = html()
    if "--small" in sys.argv:
        import zopfli.gzip
        blob = zopfli.gzip.compress(raw, numiterations=30)
    else:
        blob = gzip.compress(raw, compresslevel=9, mtime=0)

    lines = [HEADER]
    for i in range(0, len(blob), 16):
        chunk = blob[i:i + 16]
        lines.append("  " + ", ".join(f"0x{b:02x}" for b in chunk) + ",\n")
    lines.append("};\n")
    lines.append("static const size_t WEBUI_HTML_GZ_LEN = sizeof(WEBUI_HTML_GZ);\n")

    OUT.write_text("".join(lines), encoding="utf-8", newline="\n")
    print(f"{SRC.name}: {len(raw)} B -> gzip {len(blob)} B "
          f"({100 * len(blob) / len(raw):.1f}%) -> {OUT.name}")


if __name__ == "__main__":
    if sys.argv[1:] == ["--check"]:
        sys.exit(check())
    main()
