"""Import and audit a PvZ GOTY resource ZIP/folder without running its software."""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import tempfile
import xml.etree.ElementTree as ET
import zipfile

import pak


def pak_entries(path):
    data = pak.xor_data(path.read_bytes())
    records, offset = pak.read_records(data)
    entries = {}
    for name, size, _ in records:
        key = name.as_posix().casefold()
        if key in entries:
            raise ValueError(f"Duplicate resource path: {name}")
        entries[key] = data[offset:offset + size]
        offset += size
    return entries


def audit(folder):
    entries = {f"properties/{p.relative_to(folder / 'properties').as_posix()}".casefold(): p.read_bytes()
               for p in (folder / 'properties').rglob('*') if p.is_file()}
    packed = pak_entries(folder / 'main.pak')
    entries.update(packed)
    required = ['properties/resources.xml', 'properties/lawnstrings.txt',
                'properties/default.xml', 'properties/layout.xml']
    missing = [name for name in required if name not in entries]
    if missing:
        raise ValueError(f"Missing required files: {missing}")
    manifest = ET.fromstring(entries['properties/resources.xml'])
    references = 0
    for group in manifest.findall('Resources'):
        base = ''
        for item in group:
            if item.tag == 'SetDefaults':
                base = item.get('path', base).replace('\\', '/').strip('/')
                continue
            name = item.get('path', '')
            if not name or name.startswith('!') or item.tag not in ('Image', 'Sound', 'Font'):
                continue
            path = f"{base}/{name}".casefold().replace('\\', '/')
            extensions = {'Image': ['', '.png', '.jpg', '.jpeg', '.gif', '_.gif', '_.png'],
                          'Sound': ['', '.wav', '.ogg', '.mp3', '.au'], 'Font': ['', '.txt']}[item.tag]
            references += 1
            if not any(path + ext in entries for ext in extensions):
                missing.append(f"{group.get('id')}: {item.get('id')} -> {path}")
    if missing:
        raise ValueError('Missing manifest resources:\n' + '\n'.join(missing))
    return {'pak_entries': len(packed), 'manifest_references': references,
            'main_pak_sha256': hashlib.sha256((folder / 'main.pak').read_bytes()).hexdigest()}


def import_resources(source, destination):
    destination.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='pvz-resources-') as temporary:
        staged = Path(temporary)
        if source.is_file() and zipfile.is_zipfile(source):
            with zipfile.ZipFile(source) as archive:
                candidates = [i for i in archive.infolist()
                              if PurePosixPath(i.filename.replace('\\', '/')).name.casefold() == 'main.pak']
                if len(candidates) != 1:
                    raise ValueError(f"Expected one main.pak in ZIP; found {len(candidates)}")
                selected = candidates[0]
                prefix = str(PurePosixPath(selected.filename.replace('\\', '/')).parent)
                prefix = '' if prefix == '.' else prefix + '/'
                (staged / 'main.pak').write_bytes(archive.read(selected))
                for item in archive.infolist():
                    name = item.filename.replace('\\', '/')
                    if item.is_dir() or not name.casefold().startswith((prefix + 'properties/').casefold()):
                        continue
                    relative = pak.safe_relative_path(name[len(prefix):])
                    target = staged / 'properties' / Path(*relative.parts[1:])
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(archive.read(item))
        elif source.is_dir():
            candidates = [p for p in source.rglob('*') if p.is_file() and p.name.casefold() == 'main.pak']
            if len(candidates) != 1:
                raise ValueError(f"Expected one main.pak in folder; found {len(candidates)}")
            selected = candidates[0]
            properties = next((p for p in selected.parent.iterdir()
                               if p.is_dir() and p.name.casefold() == 'properties'), None)
            if properties is None:
                raise ValueError('The external properties directory is missing')
            shutil.copy2(selected, staged / 'main.pak')
            shutil.copytree(properties, staged / 'properties')
        else:
            raise ValueError('Input must be a resource ZIP or folder')
        # Some GOTY distributions put all properties inside main.pak.
        # Make those files available to XML loaders that use normal file IO.
        for name, content in pak_entries(staged / 'main.pak').items():
            if name.startswith('properties/'):
                target = staged / pak.safe_relative_path(name)
                if not target.exists():
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(content)
        report = audit(staged)
        report['source'] = str(source.resolve())
        if source.is_file():
            report['archive_sha256'] = hashlib.sha256(source.read_bytes()).hexdigest()
        if (destination / 'properties').exists() or (destination / 'main.pak').exists():
            backup = destination.parent / '.tools' / 'resource-backups'
            backup.mkdir(parents=True, exist_ok=True)
            slot = Path(tempfile.mkdtemp(dir=backup, prefix='previous-'))
            for name in ('properties', 'main.pak'):
                if (destination / name).exists():
                    shutil.move(str(destination / name), slot / name)
        shutil.copy2(staged / 'main.pak', destination / 'main.pak')
        shutil.copytree(staged / 'properties', destination / 'properties')
        (destination / 'resource-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
        return report


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('--output', type=Path, default=Path('dist'))
    parser.add_argument('--audit-only', action='store_true')
    args = parser.parse_args()
    result = audit(args.source) if args.audit_only else import_resources(args.source, args.output)
    print(json.dumps(result, ensure_ascii=False, indent=2))
