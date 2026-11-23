from pathlib import Path
from io import BytesIO
import struct
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]


def main():
    sheet = Image.new('RGB', (1000, 452), '#121820')
    draw = ImageDraw.Draw(sheet)
    index = 0
    for path in sorted((ROOT/'data/nm_tv_154/themes').glob('*.stheme')):
        data = path.read_bytes()
        position = 8
        for _ in range(struct.unpack_from('<H', data, 4)[0]):
            name_size, size = struct.unpack_from('<HI', data, position)
            position += 6 + name_size
            payload = data[position:position+size]
            position += size
            if payload[:4] != b'STJ1':
                continue
            picture = Image.open(BytesIO(payload[12:])).resize((192, 192), Image.Resampling.LANCZOS)
            x, y = 4 + index % 5 * 200, 26 + index // 5 * 224
            draw.text((x,y-19),path.stem,fill='white')
            sheet.paste(picture,(x,y))
            index += 1
            break
    output = ROOT/'docs/product/nm-tv-154-color-pictures-preview.png'
    sheet.save(output)
    print(output)


if __name__ == '__main__':
    main()
