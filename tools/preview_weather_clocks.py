import argparse
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw


def rgb(color):
    return ((color >> 11) * 255 // 31, ((color >> 5) & 63) * 255 // 63, (color & 31) * 255 // 31)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("commands", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--animate", action="store_true")
    args = parser.parse_args()
    font_path = Path(".pio/libdeps/smalltv_esp32_8mb/GFX Library for Arduino/src/font/glcdfont.h")
    font = [int(value, 16) for value in re.findall(r"0x([0-9A-Fa-f]{2})", font_path.read_text())]
    sheet = Image.new("RGB", (1008, 288), "#121820")
    sheet_draw = ImageDraw.Draw(sheet)
    names = ["Simple weather", "Detailed weather", "Three-day forecast", "Large two-tone"]
    display = None
    style = None
    frames = []

    def finish():
        if display is not None:
            if args.animate:
                frames.append(display.copy())
                return
            sheet.paste(display, (12 + style * 252, 32))
            sheet_draw.text((12 + style * 252, 10), names[style], fill="white")

    for line in args.commands.read_text().splitlines():
        command = json.loads(line)
        kind, *values = command
        if kind == "style":
            finish()
            style = values[0]
            display = Image.new("RGB", (240, 240), "black")
            draw = ImageDraw.Draw(display)
        elif kind == "rect":
            left, top, width, height, color, fill = values
            if width and height:
                draw.rectangle((left, top, left + width - 1, top + height - 1), **{"fill" if fill else "outline": rgb(color)})
        elif kind == "row":
            left, top, colors = values
            for column, color in enumerate(colors):
                display.putpixel((left+column, top), rgb(color))
        elif kind == "circle":
            center_x, center_y, radius, color, fill = values
            draw.ellipse((center_x - radius, center_y - radius, center_x + radius, center_y + radius), **{"fill" if fill else "outline": rgb(color)})
        elif kind == "line":
            draw.line(values[:4], fill=rgb(values[4]))
        elif kind == "triangle":
            draw.polygon(list(zip(values[:6:2], values[1:6:2])), fill=rgb(values[6]))
        elif kind == "text":
            left, top, size, color, text = values
            for character in text:
                for column in range(5):
                    bitmap = font[ord(character) * 5 + column]
                    for row in range(8):
                        if bitmap & (1 << row):
                            draw.rectangle((left + column * size, top + row * size, left + (column + 1) * size - 1, top + (row + 1) * size - 1), fill=rgb(color))
                left += 6 * size
    finish()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    if args.animate:
        durations = [80] * len(frames)
        for index in (13, 27):
            if index < len(durations):
                durations[index] = 2000
        frames[0].save(args.output, save_all=True, append_images=frames[1:], duration=durations, loop=0, disposal=2)
    else:
        sheet.save(args.output)
    print(args.output)


if __name__ == "__main__":
    main()
