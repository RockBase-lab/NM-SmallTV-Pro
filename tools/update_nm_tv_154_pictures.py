"""Validate and replace the ten built-in NM-TV-154 pictures over HTTP.

Python 3 standard library only. Dry run by default; --apply writes the device.
Keep the state file after interruptions and rerun the same command to resume.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import urllib.request
import uuid

IDS = ('pixel-motion', 'ultra-01', 'ultra-02', 'ultra-03', 'ultra-04',
       'ultra-05', 'ultra-06', 'ultra-08', 'ultra-09', 'ultra-10')
SELECTION = ('mode', 'themeId', 'pictureId', 'pictureAuto', 'pictureRotateSec')


def load_library(directory):
    record = json.loads((directory / 'pictures.json').read_text(encoding='utf-8'))
    items = {item['id']: item for item in record['pictures']}
    if set(items) != set(IDS) or len(record['pictures']) != len(IDS):
        raise ValueError('Expected exactly the ten built-in pictures')
    packages = {}
    for identifier in IDS:
        data = (directory / (identifier + '.stheme')).read_bytes()
        if hashlib.sha256(data).hexdigest() != items[identifier]['sha256']:
            raise ValueError('Checksum mismatch: ' + identifier)
        if data[:4] != b'STH1' or len(data) < 8:
            raise ValueError('Invalid package: ' + identifier)
        count, reserved = struct.unpack_from('<HH', data, 4)
        entries, position = {}, 8
        if reserved or not 1 <= count <= 256:
            raise ValueError('Invalid package header: ' + identifier)
        for _ in range(count):
            name_size, size = struct.unpack_from('<HI', data, position)
            position += 6
            name = data[position:position + name_size].decode('ascii')
            position += name_size
            if name in entries or not name or position + size > len(data):
                raise ValueError('Invalid package entry: ' + identifier)
            entries[name] = data[position:position + size]
            position += size
        if position != len(data):
            raise ValueError('Trailing package data: ' + identifier)
        manifest = json.loads(entries['theme.json'])
        if manifest['theme']['id'] != identifier or manifest['theme']['version'] != '2.0.0':
            raise ValueError('Unexpected picture version: ' + identifier)
        assets = [payload for name, payload in entries.items() if name != 'theme.json']
        if not assets or any(payload[:4] != b'STJ1' or len(payload) < 16 or
                             struct.unpack_from('<HHI', payload, 4) != (240, 240, 0) or
                             payload[12:14] != b'\xff\xd8' for payload in assets):
            raise ValueError('Expected full-color 240x240 JPEG assets: ' + identifier)
        for layer in manifest['layers']:
            if layer['type'] == 'animation' and type(layer.get('loop')) is not bool:
                raise ValueError('Invalid animation loop: ' + identifier)
        packages[identifier] = data
    return packages


class Device:
    def __init__(self, address, username=None, password=None):
        self.base = address.rstrip('/') if '://' in address else 'http://' + address.rstrip('/')
        if not self.base.startswith('http://') or '/' in self.base[7:]:
            raise ValueError('Use the device HTTP address without a path')
        handlers = []
        if username:
            credentials = urllib.request.HTTPPasswordMgrWithDefaultRealm()
            credentials.add_password(None, self.base, username, password)
            handlers.append(urllib.request.HTTPDigestAuthHandler(credentials))
        self.opener = urllib.request.build_opener(*handlers)

    def request(self, path, payload=None, content_type='application/json'):
        headers = {'Origin': self.base}
        if payload is not None:
            headers['Content-Type'] = content_type
        request = urllib.request.Request(self.base + path, data=payload, headers=headers)
        with self.opener.open(request, timeout=60) as response:
            raw = response.read()
            return json.loads(raw) if raw else {}

    def post(self, path, value):
        return self.request(path, json.dumps(value).encode())

    def install(self, identifier, data):
        boundary = 'smalltv-' + uuid.uuid4().hex
        body = ('--' + boundary + '\r\nContent-Disposition: form-data; name="theme"; filename="' +
                identifier + '.stheme"\r\nContent-Type: application/octet-stream\r\n\r\n').encode()
        body += data + ('\r\n--' + boundary + '--\r\n').encode()
        return self.request('/api/themes/install?category=picture', body,
                            'multipart/form-data; boundary=' + boundary)


def update(device, packages, state_path, apply=False):
    configuration = device.request('/api/config')
    listing = device.request('/api/themes')
    existing = {item['id']: item for item in listing['themes']}
    print('Validated ten full-color packages; payload:', sum(map(len, packages.values())), 'bytes')
    print('Device free space:', listing['freeBytes'], 'bytes')
    print('Replace built-ins:', ', '.join(IDS))
    if not apply:
        print('Dry run. Add --apply to install. Custom pictures and clock themes are kept.')
        return
    available = listing['freeBytes'] + sum((existing[identifier]['bytes'] + 4095) // 4096 * 4096
                                          for identifier in IDS if identifier in existing)
    if available < sum((len(data) + 4095) // 4096 * 4096 for data in packages.values()) + 20480:
        raise ValueError('Insufficient space after replacing built-ins; remove an unused custom package first')
    if state_path.exists():
        state = json.loads(state_path.read_text(encoding='utf-8'))
        if state['device'] != device.base or set(state['selection']) != set(SELECTION):
            raise ValueError('State file belongs to another device or is incomplete')
    else:
        state = {'device': device.base, 'selection': {key: configuration[key] for key in SELECTION}}
        with state_path.open('x', encoding='utf-8') as output:
            json.dump(state, output, indent=2)
    try:
        device.post('/api/config', {'mode': 'weather'})
        for identifier in IDS:
            if identifier in existing:
                device.post('/api/themes/delete', {'id': identifier})
        for identifier, data in packages.items():
            free = device.request('/api/themes')['freeBytes']
            if free < len(data) + 20480:
                raise RuntimeError('Not enough free storage to install ' + identifier)
            print('Installing', identifier, len(data), 'bytes')
            device.install(identifier, data)
        installed = {item['id']: item for item in device.request('/api/themes')['themes']}
        for identifier, data in packages.items():
            item = installed.get(identifier, {})
            if not item.get('valid') or item.get('version') != '2.0.0' or item.get('bytes') != len(data):
                raise RuntimeError('Device verification failed: ' + identifier)
        device.post('/api/config', state['selection'])
        restored = device.request('/api/config')
        if any(restored[key] != value for key, value in state['selection'].items()):
            raise RuntimeError('Could not restore mode and picture settings')
    except BaseException:
        print('Update interrupted. State saved in', state_path)
        print('Rerun with --apply to reinstall and restore the original selection.')
        raise
    state_path.unlink()
    print('Installed and verified ten full-color pictures; original mode and picture settings restored.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('device', help='Device IP, for example 192.168.124.14')
    parser.add_argument('--library', type=Path, default=Path(__file__).resolve().parent / 'pictures')
    parser.add_argument('--state', type=Path, default=Path('nm-tv-154-picture-update-state.json'))
    parser.add_argument('--username')
    parser.add_argument('--apply', action='store_true')
    args = parser.parse_args()
    packages = load_library(args.library)
    device = Device(args.device, args.username, os.environ.get('SMALLTV_PASSWORD'))
    update(device, packages, args.state, args.apply)


if __name__ == '__main__':
    main()
