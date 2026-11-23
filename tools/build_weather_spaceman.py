from pathlib import Path
from PIL import Image, ImageSequence
from build_weather_font import encode_glyph

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT/'docs/product/assets/smalltv-ultra/pictures/spaceman.gif'
OUTPUT = ROOT/'src/features/weather/WeatherSpaceman.h'
SIZE = 56


def encode(values):
    result = []
    previous, count = None, 0
    for value in values:
        if value != previous or count == 128:
            if count:
                result.append((previous << 7) | (count - 1))
            previous, count = value, 1
        else:
            count += 1
    if count:
        result.append((previous << 7) | (count - 1))
    return result


def generate():
    data, offsets, deadlines = [], [0], []
    with Image.open(SOURCE) as animation:
        for frame in ImageSequence.Iterator(animation):
            image = frame.convert('L').resize((SIZE, SIZE), Image.Resampling.LANCZOS)
            values = [int(value >= 100) for value in image.tobytes()]
            packed = [sum(values[index+bit] << (7-bit) for bit in range(8)) for index in range(0,len(values),8)]
            data.extend(encode_glyph(packed))
            offsets.append(len(data))
            deadlines.append((deadlines[-1] if deadlines else 0) + max(20, frame.info.get('duration', 80)))
    lines = ['#pragma once', '#include <cstdint>', '#ifdef ARDUINO', '#include <pgmspace.h>',
             '#define WEATHER_SPRITE_FLASH PROGMEM', '#else', '#define WEATHER_SPRITE_FLASH', '#endif',
             f'constexpr unsigned WeatherSpacemanSize = {SIZE};',
             f'constexpr unsigned WeatherSpacemanFrames = {len(deadlines)};',
             f'constexpr uint32_t WeatherSpacemanDuration = {deadlines[-1]};',
             'static const uint16_t weatherSpacemanOffsets[] WEATHER_SPRITE_FLASH = {' + ','.join(map(str, offsets)) + '};',
             'static const uint16_t weatherSpacemanDeadlines[] WEATHER_SPRITE_FLASH = {' + ','.join(map(str, deadlines)) + '};',
             'static const uint8_t weatherSpacemanPixels[] WEATHER_SPRITE_FLASH = {']
    for index in range(0, len(data), 24):
        lines.append('  '+','.join(map(str, data[index:index+24]))+',')
    lines.extend(['};', '#undef WEATHER_SPRITE_FLASH', ''])
    return '\n'.join(lines)


if __name__ == '__main__':
    OUTPUT.write_text(generate(), encoding='utf-8')
    print(OUTPUT)
