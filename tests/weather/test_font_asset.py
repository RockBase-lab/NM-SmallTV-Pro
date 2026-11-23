import importlib.util
from pathlib import Path
import re
import unittest
import sys

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('font',ROOT/'tools/build_weather_font.py')
font=importlib.util.module_from_spec(spec)
spec.loader.exec_module(font)

class FontAssetTests(unittest.TestCase):
    def test_lossless_font_compression(self):
        self.assertTrue(hasattr(font,'encode_glyph'),'weather font needs lossless compact storage')
        def decode(encoded):
            out=[];position=0
            while position<len(encoded):
                flags=encoded[position];position+=1
                for bit in range(8):
                    if position>=len(encoded):break
                    if flags&(1<<bit):out.append(encoded[position]);position+=1
                    else:
                        token=(encoded[position]<<8)|encoded[position+1];position+=2
                        for _ in range((token&63)+3):out.append(out[-((token>>6)+1)])
            return out
        total=0
        for character in '0123456789':
            pixels=font.pack(font.glyph(character,40,80));encoded=font.encode_glyph(pixels)
            self.assertEqual(decode(encoded),pixels)
            total+=len(encoded)
        self.assertLess(total,2500,'compression must reclaim real flash space without changing any coverage sample')
        self.assertEqual(decode(font.encode_glyph([0]*300+[1,2,3]+[255]*128)),[0]*300+[1,2,3]+[255]*128)
        for code in range(32,127):
            pixels=font.pack(font.letter_glyph(code)[0])
            self.assertEqual(decode(font.encode_glyph(pixels)),pixels)

    def test_native_decoder_matches_original_rasters(self):
        path=ROOT/'tmp/font-decoded.bin'
        if not path.exists():self.skipTest('Run native font decoder first')
        expected=[]
        for character in '0123456789':expected.extend(font.pack(font.glyph(character,40,80)))
        for code in range(32,127):expected.extend(font.pack(font.letter_glyph(code)[0]))
        self.assertEqual(path.read_bytes(),bytes(expected),'all 105 decoded glyphs must match the original raster samples')

if __name__=='__main__':unittest.main()
