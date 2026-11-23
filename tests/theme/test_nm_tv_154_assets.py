"""The NM-TV-154 factory image must contain every reference asset exactly once."""
import json
from pathlib import Path
import struct
import unittest
from io import BytesIO
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
EXPECTED = {
    'classic-24', 'classic-12', 'pixel-motion',
    'ultra-01', 'ultra-02', 'ultra-03', 'ultra-04', 'ultra-05', 'ultra-06',
    'ultra-08', 'ultra-09', 'ultra-10',
}


class NmTv154AssetTests(unittest.TestCase):
    def test_all_packages_and_reference_assets_are_present(self):
        record = json.loads((ROOT/'assets/nm_tv_154/themes/builtin-themes.json').read_text(encoding='utf-8'))
        themes = {item['id']: item for item in record['themes']}
        self.assertEqual(set(themes), EXPECTED)
        self.assertEqual(record['imageSize'], 240)
        self.assertEqual(record['maxFrames'], 2)
        self.assertEqual(record['encoding'], 'JPEG RGB')
        # 168,460 B of packages already failed LittleFS packing. Keep at least
        # 48 KiB of the raw 192 KiB partition for filesystem metadata/config.
        self.assertLessEqual(record['packageBytes'], 147456)
        self.assertLessEqual(sum((item['bytes'] + 4095) // 4096 * 4096 for item in themes.values() if item['source']), 143360)
        sources = [item['source'] for item in themes.values() if item['source']]
        self.assertEqual(len(sources), 10)
        self.assertEqual(len(set(sources)), 10)
        references = {p.name for p in (ROOT/'docs/product/assets/smalltv-ultra/pictures').iterdir()
                      if p.suffix.lower() in ('.jpg', '.gif')}
        self.assertEqual(set(sources), references)
        for theme_id, item in themes.items():
            package = ROOT/'data/nm_tv_154/themes'/f'{theme_id}.stheme'
            data = package.read_bytes()
            self.assertEqual(data[:4], b'STH1', theme_id)
            self.assertGreater(struct.unpack_from('<H', data, 4)[0], 0, theme_id)  # entry count
            self.assertEqual(struct.unpack_from('<H', data, 6)[0], 0, theme_id)     # reserved
            self.assertEqual(len(data), item['bytes'], theme_id)
            manifest = json.loads((ROOT/'assets/nm_tv_154/themes'/theme_id/'theme.json').read_text(encoding='utf-8'))
            self.assertEqual(manifest['theme']['id'], theme_id)
            self.assertEqual(manifest['spec'], 2)
            for layer in manifest['layers']:
                if layer['type'] not in ('image', 'animation'):
                    continue
                self.assertEqual((layer['x'], layer['y']), (0, 0), theme_id)
                if layer['type'] == 'animation':
                    self.assertEqual((layer['width'], layer['height']), (240, 240), theme_id)
                else:
                    # Image dimensions come from the STI2 entry at load time;
                    # the manifest deliberately omits redundant width/height.
                    png = (ROOT/'assets/nm_tv_154/themes'/theme_id/layer['source']).read_bytes()
                    self.assertEqual(png[:8], b'\x89PNG\r\n\x1a\n', theme_id)
                    self.assertEqual(struct.unpack_from('>II', png, 16), (240, 240), theme_id)
        pixel_motion = json.loads((ROOT/'assets/nm_tv_154/themes/pixel-motion/theme.json').read_text(encoding='utf-8'))
        self.assertEqual([layer['type'] for layer in pixel_motion['layers']], ['animation'])
        self.assertEqual(record['packageBytes'], sum(p.stat().st_size for p in
                         (ROOT/'data/nm_tv_154/themes').glob('*.stheme')))

    def test_motion_assets_are_sampled_to_two_fullscreen_frames(self):
        for theme_id in ('pixel-motion', 'ultra-04', 'ultra-10'):
            manifest = json.loads((ROOT/'assets/nm_tv_154/themes'/theme_id/'theme.json').read_text(encoding='utf-8'))
            animation = next(layer for layer in manifest['layers'] if layer['type'] == 'animation')
            self.assertEqual(animation['frames'], 2)
            self.assertIs(animation['loop'], True)
            self.assertEqual(manifest['theme']['version'], '2.0.0')
            self.assertEqual(animation['fps'], 4)
            self.assertEqual((animation['width'], animation['height']), (240, 240))

    def test_factory_photos_keep_full_color(self):
        for package in (ROOT/'data/nm_tv_154/themes').glob('*.stheme'):
            data = package.read_bytes()
            position = 8
            for _ in range(struct.unpack_from('<H', data, 4)[0]):
                name_size, length = struct.unpack_from('<HI', data, position)
                position += 6
                name = data[position:position+name_size].decode('ascii')
                position += name_size
                payload = data[position:position+length]
                position += length
                if not name.endswith('.sti'):
                    continue
                self.assertEqual(payload[:4], b'STJ1', package.name)
                decoded = Image.open(BytesIO(payload[12:])).convert('RGB')
                self.assertEqual(decoded.size, (240, 240))
                self.assertGreater(len(decoded.getcolors(57600)), 256, package.name)
                source = ROOT/'assets/nm_tv_154/themes'/package.stem/name[:-4]
                original = Image.open(source).convert('RGB')
                error = sum(abs(a-b) for a,b in zip(original.tobytes(), decoded.tobytes())) / (240*240*3)
                self.assertLess(error, 22, package.name)


if __name__ == '__main__':
    unittest.main()
