import json
from pathlib import Path
import struct
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT/'tools'))
import theme_native


def entries(data):
    position = 8
    result = {}
    for _ in range(struct.unpack_from('<H', data, 4)[0]):
        name_size, size = struct.unpack_from('<HI', data, position)
        position += 6
        name = data[position:position+name_size].decode()
        position += name_size
        result[name] = data[position:position+size]
        position += size
    return result


def package(items):
    data = bytearray(b'STH1'+struct.pack('<HH',len(items),0))
    for name, payload in items.items():
        encoded = name.encode()
        data.extend(struct.pack('<HI',len(encoded),len(payload))+encoded+payload)
    return data


class JpegPackageTests(unittest.TestCase):
    def validate(self, data):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)/'picture.stheme'
            path.write_bytes(data)
            return theme_native.validate_package(path)

    def test_rgb_picture_and_animation_packages_load(self):
        for path in (ROOT/'data/nm_tv_154/themes').glob('*.stheme'):
            self.assertEqual(theme_native.validate_package(path)['id'],path.stem)

    def test_jpeg_rejects_bad_size_progressive_and_truncation(self):
        original = entries((ROOT/'data/nm_tv_154/themes/ultra-01.stheme').read_bytes())
        name = next(name for name in original if name.endswith('.sti'))
        for change in ('width','reserved','progressive','missing-end','height'):
            items = dict(original)
            image = bytearray(items[name])
            if change == 'width': image[4] = 0
            if change == 'height': image[6] = 0
            if change == 'reserved': image[8] = 1
            if change == 'progressive':
                marker = image.index(b'\xff\xc0')
                image[marker+1] = 0xc2
            if change == 'missing-end': image[-1] = 0
            items[name] = image
            with self.subTest(change=change),self.assertRaises(ValueError):
                self.validate(package(items))

    def test_jpeg_rejects_compositing_that_cannot_be_rendered(self):
        original = entries((ROOT/'data/nm_tv_154/themes/ultra-01.stheme').read_bytes())
        for change in ('x','spec','overlay','binding'):
            items = dict(original)
            manifest = json.loads(items['theme.json'])
            if change == 'x': manifest['layers'][0]['x'] = 1
            if change == 'spec': manifest['spec'] = 1
            if change == 'overlay': manifest['layers'].append({'id':'overlay','type':'text','x':0,'y':0,'value':'test','size':8,'color':'#ffffff'})
            if change == 'binding': manifest['layers'][0]['bind']={'x':{'source':'time.second'}}
            items['theme.json'] = json.dumps(manifest).encode()
            with self.subTest(change=change),self.assertRaises(ValueError):
                self.validate(package(items))


if __name__ == '__main__':
    unittest.main()
