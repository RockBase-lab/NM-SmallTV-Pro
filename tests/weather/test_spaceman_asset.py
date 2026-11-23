import importlib.util
from pathlib import Path
import re
import unittest
import sys
from PIL import Image, ImageSequence

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
spec = importlib.util.spec_from_file_location('weather_spaceman', ROOT/'tools/build_weather_spaceman.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class SpacemanAssetTests(unittest.TestCase):
    def test_generated_animation_matches_every_original_gif_frame(self):
        header = generator.OUTPUT.read_text(encoding='utf-8')
        self.assertEqual(header, generator.generate())
        def values(name):
            match = re.search(name+r'\[\].*?=\s*\{(.*?)\};',header,re.S)
            return [int(value) for value in re.findall(r'\d+',match[1])]
        encoded = values('weatherSpacemanPixels')
        offsets = values('weatherSpacemanOffsets')
        deadlines = values('weatherSpacemanDeadlines')
        self.assertLess(len(encoded),2600)
        expected_packed = []
        with Image.open(generator.SOURCE) as animation:
            self.assertEqual(len(offsets),animation.n_frames+1)
            self.assertEqual(len(deadlines),animation.n_frames)
            for index,frame in enumerate(ImageSequence.Iterator(animation)):
                packed = []
                compressed = encoded[offsets[index]:offsets[index+1]]
                cursor = 0
                while cursor < len(compressed):
                    flags = compressed[cursor];cursor += 1
                    for bit in range(8):
                        if cursor >= len(compressed):break
                        if flags & (1 << bit):packed.append(compressed[cursor]);cursor += 1
                        else:
                            token = (compressed[cursor] << 8) | compressed[cursor+1];cursor += 2
                            for _ in range((token & 63)+3):packed.append(packed[-((token >> 6)+1)])
                decoded = [(value >> shift) & 1 for value in packed for shift in range(7,-1,-1)]
                expected_packed.extend(packed)
                reference = frame.convert('L').resize((56,56),Image.Resampling.LANCZOS)
                self.assertEqual(decoded,[int(value>=100) for value in reference.tobytes()])
        native = ROOT/'tmp/sprite-decoded.bin'
        if native.exists():self.assertEqual(native.read_bytes(),bytes(expected_packed))


if __name__ == '__main__':
    unittest.main()
