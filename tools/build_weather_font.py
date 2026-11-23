from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT = ROOT / 'examples/themes/bebas-clock/fonts/BebasNeue-Regular.ttf'


def glyph(character, width, height):
    font = ImageFont.truetype(str(FONT), 160)
    bounds = font.getbbox(character)
    image = Image.new('L', (max(1, bounds[2]-bounds[0]), max(1, bounds[3]-bounds[1])))
    ImageDraw.Draw(image).text((-bounds[0], -bounds[1]), character, fill=255, font=font)
    advance = round(font.getlength('0'))
    canvas = Image.new('L', (advance, image.height))
    canvas.paste(image, ((advance-image.width)//2, 0))
    return canvas.resize((width, height), Image.Resampling.LANCZOS)


def pack(image):
    values = [min(3, (value+42)//85) for value in image.tobytes()]
    return [(values[index]<<6)|(values[index+1]<<4)|(values[index+2]<<2)|values[index+3]
            for index in range(0, len(values), 4)]


def encode_glyph(values):
    encoded,position=[],0
    while position<len(values):
        flag_at=len(encoded)
        encoded.append(0)
        for bit in range(8):
            if position>=len(values):
                break
            best_length,best_distance=0,0
            for distance in range(1,min(position,1024)+1):
                length=0
                while length<66 and position+length<len(values) and values[position+length]==values[position+length-distance]:
                    length+=1
                if length>best_length:
                    best_length,best_distance=length,distance
                if length==66:
                    break
            if best_length>=3:
                token=((best_distance-1)<<6)|(best_length-3)
                encoded.extend([token>>8,token&255])
                position+=best_length
            else:
                encoded[flag_at]|=1<<bit
                encoded.append(values[position])
                position+=1
    return encoded


def letter_glyph(code):
    font=ImageFont.truetype(str(FONT),28)
    character=chr(code)
    bounds=font.getbbox(character)
    image=Image.new('L',(max(1,bounds[2]-bounds[0]),24))
    ImageDraw.Draw(image).text((-bounds[0],-7),character,fill=255,font=font)
    padded=Image.new('L',((image.width+3)//4*4,24))
    padded.paste(image)
    return padded,round(font.getlength(character))+1


def main():
    values, offsets = [], []
    for character in '0123456789':
        offsets.append(len(values))
        values.extend(encode_glyph(pack(glyph(character, 40, 80))))
    lines = ['#pragma once', '#include <cstdint>',
             '#ifdef ARDUINO', '#include <pgmspace.h>', '#define WEATHER_FONT_FLASH PROGMEM',
             '#else', '#define WEATHER_FONT_FLASH', '#endif',
             'static const uint16_t weatherDigitOffsets[] WEATHER_FONT_FLASH = {'+','.join(map(str,offsets))+'};',
             'static const uint8_t weatherDigits[] WEATHER_FONT_FLASH = {']
    for index in range(0, len(values), 24):
        lines.append('  '+','.join(str(value) for value in values[index:index+24])+',')
    lines.extend(['};', '#undef WEATHER_FONT_FLASH', ''])
    records, coverage = [], []
    for code in range(32, 127):
        padded,advance=letter_glyph(code)
        records.append((len(coverage), padded.width, advance))
        assert padded.width<=40
        coverage.extend(encode_glyph(pack(padded)))
    lines[-2:-2] = ['struct WeatherLetter { uint16_t offset; uint8_t width, advance; };',
                    'static const WeatherLetter weatherLetters[] WEATHER_FONT_FLASH = {',
                    *['  {'+','.join(map(str, record))+'},' for record in records], '};',
                    'static const uint8_t weatherLetterPixels[] WEATHER_FONT_FLASH = {',
                    *['  '+','.join(map(str,coverage[index:index+24]))+',' for index in range(0,len(coverage),24)], '};']
    (ROOT/'src/features/weather/WeatherDigits.h').write_text('\n'.join(lines), encoding='utf-8')


if __name__ == '__main__':
    main()
