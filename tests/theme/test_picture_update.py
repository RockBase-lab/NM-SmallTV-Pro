import contextlib
import hashlib
from http.server import BaseHTTPRequestHandler, HTTPServer
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import threading
import unittest

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('picture_update', ROOT/'tools/update_nm_tv_154_pictures.py')
updater = importlib.util.module_from_spec(spec)
spec.loader.exec_module(updater)


class PictureUpdateTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.directory = Path(self.temp.name)
        record = {'pictures': []}
        for identifier in updater.IDS:
            data = (ROOT/'data/nm_tv_154/themes'/f'{identifier}.stheme').read_bytes()
            (self.directory/f'{identifier}.stheme').write_bytes(data)
            record['pictures'].append({'id': identifier, 'sha256': hashlib.sha256(data).hexdigest()})
        (self.directory/'pictures.json').write_text(json.dumps(record))
        self.packages = updater.load_library(self.directory)
        self.original = {'mode': 'carousel', 'themeId': 'ultra-01', 'pictureId': 'ultra-04', 'pictureAuto': True,
                         'pictureRotateSec': 17, 'carouselPictures': True}
        self.config = dict(self.original)
        self.installed = {identifier: {'id': identifier, 'valid': False, 'bytes': 13000, 'version': '1.0.0'}
                          for identifier in updater.IDS}
        self.installed['picture-custom'] = {'id': 'picture-custom', 'valid': True, 'bytes': 1000}
        self.installed['classic-24'] = {'id': 'classic-24', 'valid': True, 'bytes': 416}
        self.posts, self.fail_id = [], None
        test = self

        class Handler(BaseHTTPRequestHandler):
            def log_message(self, *args):
                pass

            def reply(self, value, code=200):
                raw = json.dumps(value).encode()
                self.send_response(code)
                self.send_header('Content-Length', str(len(raw)))
                self.end_headers()
                self.wfile.write(raw)

            def do_GET(self):
                self.reply(test.config if self.path == '/api/config' else
                           {'freeBytes': 196608 - 20480 - sum((item['bytes'] + 4095) // 4096 * 4096 for item in test.installed.values()),
                            'themes': list(test.installed.values())})

            def do_POST(self):
                test.assertEqual(self.headers['Origin'], test.device.base)
                raw = self.rfile.read(int(self.headers['Content-Length']))
                test.posts.append(self.path)
                if self.path == '/api/config':
                    test.config.update(json.loads(raw))
                elif self.path == '/api/themes/delete':
                    test.assertEqual(test.config['mode'], 'weather')
                    identifier = json.loads(raw)['id']
                    test.assertIn(identifier, updater.IDS)
                    del test.installed[identifier]
                    if test.config['pictureId'] == identifier:
                        test.config['pictureId'] = ''
                    if test.config['themeId'] == identifier:
                        test.config['themeId'] = ''
                else:
                    test.assertEqual(self.path, '/api/themes/install?category=picture')
                    identifier = raw.split(b'filename="')[1].split(b'.stheme"')[0].decode()
                    if identifier == test.fail_id:
                        self.reply({'error': 'simulated disconnect'}, 500)
                        return
                    payload = raw.split(b'\r\n\r\n', 1)[1].rsplit(b'\r\n--', 1)[0]
                    test.assertEqual(payload, test.packages[identifier])
                    test.installed[identifier] = {'id': identifier, 'bytes': len(payload),
                                                 'valid': True, 'version': '2.0.0'}
                self.reply({'ok': True})

        self.server = HTTPServer(('127.0.0.1', 0), Handler)
        self.worker = threading.Thread(target=self.server.serve_forever, daemon=True)
        self.worker.start()
        self.device = updater.Device(f'127.0.0.1:{self.server.server_port}')
        self.state = self.directory/'state.json'

    def tearDown(self):
        self.server.shutdown()
        self.server.server_close()
        self.worker.join()
        self.temp.cleanup()

    def run_update(self, apply):
        with contextlib.redirect_stdout(io.StringIO()):
            updater.update(self.device, self.packages, self.state, apply)

    def test_dry_run_has_no_writes(self):
        self.run_update(False)
        self.assertEqual(self.posts, [])
        self.assertFalse(self.state.exists())

    def test_replaces_builtin_library_preserving_custom_and_selection(self):
        self.run_update(True)
        self.assertEqual(self.config, self.original)
        self.assertEqual(len(self.installed), 12)
        self.assertEqual(self.installed['picture-custom']['bytes'], 1000)
        self.assertEqual(self.installed['classic-24']['bytes'], 416)
        self.assertEqual(self.posts[1:11], ['/api/themes/delete'] * 10)
        self.assertFalse(self.state.exists())

    def test_failed_install_resumes_original_selection(self):
        self.fail_id = 'ultra-02'
        with self.assertRaises(Exception):
            self.run_update(True)
        self.assertTrue(self.state.exists())
        self.assertEqual(self.config['mode'], 'weather')
        self.fail_id = None
        self.run_update(True)
        self.assertEqual(self.config, self.original)
        self.assertFalse(self.state.exists())

    def test_bad_library_is_rejected_before_device_access(self):
        path = self.directory/'ultra-10.stheme'
        path.write_bytes(path.read_bytes()[:-1])
        with self.assertRaisesRegex(ValueError, 'Checksum mismatch'):
            updater.load_library(self.directory)
        self.assertEqual(self.posts, [])

    def test_insufficient_capacity_is_rejected_before_deleting(self):
        self.installed['picture-custom']['bytes'] = 100000
        with self.assertRaisesRegex(ValueError, 'Insufficient space'):
            self.run_update(True)
        self.assertEqual(self.posts, [])
        self.assertEqual(self.config, self.original)

    def test_allocation_padding_is_included_in_preflight(self):
        self.installed['picture-custom']['bytes'] = 24576
        with self.assertRaisesRegex(ValueError, 'Insufficient space'):
            self.run_update(True)
        self.assertEqual(self.posts, [])
        self.assertFalse(self.state.exists())


if __name__ == '__main__':
    unittest.main()
