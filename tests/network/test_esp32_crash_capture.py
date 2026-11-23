from pathlib import Path
import unittest


class Esp32CrashCaptureTest(unittest.TestCase):
    def test_panic_capture_precedes_original_handler(self):
        source = Path('src/Platform.cpp').read_text(encoding='utf-8')
        self.assertTrue('set_arduino_panic_handler(capturePanic, nullptr)' in source)
        handler = source.split('void IRAM_ATTR capturePanic', 1)[1].split('\n}', 1)[0]
        self.assertIn('crashSeal', handler)
        self.assertNotIn('__wrap_esp_panic_handler', source)
        self.assertNotIn('Serial.', handler)
        self.assertNotIn('String', handler)
        self.assertIn('g_panic_abort ?', handler)
        self.assertIn('g_panic_abort_details', handler)

    def test_capture_is_linked_and_safe_mode_is_visible(self):
        main = Path('src/main.cpp').read_text(encoding='utf-8')
        self.assertIn('appSafeMode()', main)
        self.assertIn('platformCrashTrace(m->id())', main)
        portal = Path('src/WebPortal.cpp').read_text(encoding='utf-8')
        self.assertIn('d.safeMode = appSafeMode()', portal)
        self.assertNotIn('OTA flash to fix:', Path('src/Gfx.cpp').read_text(encoding='utf-8'))


if __name__ == '__main__':
    unittest.main()
