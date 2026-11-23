"""Bundle built NM-TV-154 firmware and a verified full-color picture updater."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile
import gzip
import re

from update_nm_tv_154_pictures import IDS, load_library

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('--readme', type=Path, default=ROOT/'docs/product/nm-tv-154-pictures-modes-update.md')
    parser.add_argument('--include-manuals', action='store_true')
    parser.add_argument('--review', type=Path, default=ROOT/'docs/product/2026-11-20-product-usability-review.md')
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists():
        parser.error('Output must be a new directory')
    files = {}
    for environment, suffix in [('smalltv_esp32', ''), ('smalltv_esp32_wg', '-wg')]:
        build = ROOT/'.pio/build'/environment
        filesystem = (build/'littlefs.bin').read_bytes()
        factory = (build/'firmware.factory.bin').read_bytes()
        if len(filesystem) != 196608 or factory[0x3d0000:0x400000] != filesystem:
            parser.error('Factory filesystem is missing or mismatched: ' + environment)
        for source, kind in [('firmware.bin', 'ota.bin'), ('firmware.elf', 'debug.elf'),
                             ('firmware.factory.bin', 'factory.bin')]:
            path = ROOT/'.pio/build'/environment/source
            if not path.is_file():
                parser.error('Build artifact missing: ' + str(path))
            if kind == 'ota.bin' and path.stat().st_size > 1966080 - 8192:
                parser.error('OTA headroom below 8192 bytes: ' + environment)
            files['nm-smalltv-pro-nm-tv-154' + suffix + '-' + kind] = path
    output.mkdir(parents=True)
    pictures = output/'pictures'
    pictures.mkdir()
    record = {'version': '2.0.0', 'encoding': '240x240 RGB baseline JPEG', 'pictures': []}
    for identifier in IDS:
        data = (ROOT/'data/nm_tv_154/themes'/f'{identifier}.stheme').read_bytes()
        (pictures/f'{identifier}.stheme').write_bytes(data)
        record['pictures'].append({'id': identifier, 'bytes': len(data),
                                   'sha256': hashlib.sha256(data).hexdigest()})
    (pictures/'pictures.json').write_text(json.dumps(record, indent=2) + '\n', encoding='utf-8')
    load_library(pictures)
    for name, path in files.items():
        shutil.copyfile(path, output/name)
    definitions=dict(re.findall(r'^#define\s+(FW_DISPLAY_NAME|FW_NAME|FW_VERSION|REPO_URL)\s+"([^"]+)"',(ROOT/'src/config.h').read_text(encoding='utf-8'),re.M))
    header=(ROOT/'src/webui.h').read_text(encoding='utf-8').split('PROGMEM = {',1)[1]
    ui=gzip.decompress(bytes(int(value,16) for value in re.findall(r'0x([0-9a-f]{2})',header)))
    (output/'webui.html').write_bytes(ui)
    manifest={'displayName':definitions['FW_DISPLAY_NAME'],'name':definitions['FW_NAME'],'version':definitions['FW_VERSION'],
              'repository':definitions['REPO_URL'],'target':'NM-TV-154',
              'otaSlotBytes':1966080,'minimumOtaFreeBytes':8192,'webuiSha256':hashlib.sha256(ui).hexdigest(),
              'firmware':{name:{'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
                          for name,path in files.items() if name.endswith('ota.bin')}}
    (output/'release.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    shutil.copyfile(ROOT/'tools/update_nm_tv_154_pictures.py', output/'update_nm_tv_154_pictures.py')
    shutil.copyfile(args.readme, output/'README.md')
    if args.include_manuals:
        manuals = output/'manuals'
        manuals.mkdir()
        for language in ('zh', 'en'):
            shutil.copyfile(ROOT/'docs/src/content/docs/manual'/f'user-manual-{language}.md', manuals/f'user-manual-{language}.md')
            shutil.copyfile(ROOT/'output/pdf'/f'SmallTV-User-Manual-{language.upper()}.pdf', manuals/f'SmallTV-User-Manual-{language.upper()}.pdf')
        shutil.copyfile(args.review, manuals/'product-usability-review.md')
        for name in ('weather-clocks-preview.png', 'nm-tv-154-color-pictures-preview.png'):
            shutil.copyfile(ROOT/'docs/product'/name, manuals/name)
        for language in ('zh', 'en'):
            path = manuals/f'user-manual-{language}.md'
            path.write_text(path.read_text(encoding='utf-8').replace('../../../../product/', ''), encoding='utf-8')
    shutil.copyfile(ROOT/'docs/product/nm-tv-154-color-pictures-preview.png', output/'pictures-preview.png')
    with zipfile.ZipFile(output/'full-color-picture-library-v2.zip', 'w', zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(pictures.iterdir()):
            archive.write(path, path.relative_to(output))
        archive.write(output/'update_nm_tv_154_pictures.py', 'update_nm_tv_154_pictures.py')
        archive.write(output/'README.md', 'README.md')
    checksums = []
    for path in sorted(output.rglob('*')):
        if path.is_file():
            checksums.append(hashlib.sha256(path.read_bytes()).hexdigest() + '  ' + path.relative_to(output).as_posix())
    (output/'SHA256SUMS.txt').write_text('\n'.join(checksums) + '\n', encoding='utf-8')
    print(output)


if __name__ == '__main__':
    main()
