#!/usr/bin/env python3
"""Read the installed game's unencrypted Godot PCK; extract selected assets only.

Format reference: Godot 4.5 core/io/file_access_pack.cpp.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import io
import subprocess
import sys

def index(pck):
    with pck.open('rb') as f:
        magic,version,major,minor,patch,flags,base=struct.unpack('<4s5IQ',f.read(32))
        if magic!=b'GDPC' or version not in (2,3) or flags&1:
            raise ValueError('Unsupported or encrypted PCK')
        if version==3:
            directory=struct.unpack('<Q',f.read(8))[0];f.seek(directory)
        else: f.seek(96)
        count=struct.unpack('<I',f.read(4))[0]
        records={}
        for _ in range(count):
            n=struct.unpack('<I',f.read(4))[0]
            name=f.read(n).decode().rstrip('\0')
            offset,size=struct.unpack('<QQ',f.read(16))
            md5=f.read(16).hex();eflags=struct.unpack('<I',f.read(4))[0]
            records[name]={'offset':base+offset,'size':size,'md5':md5,'flags':eflags}
        return records

def read(pck,record):
    if record['flags']&1: raise ValueError('Encrypted entry')
    with pck.open('rb') as f:
        f.seek(record['offset']);data=f.read(record['size'])
    if hashlib.md5(data).hexdigest()!=record['md5']: raise ValueError('Asset hash mismatch')
    return data

if __name__=='__main__':
    a=argparse.ArgumentParser();a.add_argument('pck',type=Path);a.add_argument('--index',type=Path,required=True)
    a.add_argument('--export',action='store_true',help='Export Lightning/Plasma/Glass and the Cubex setup pose into dist/images/sts2')
    args=a.parse_args();records=index(args.pck)
    args.index.parent.mkdir(parents=True,exist_ok=True)
    args.index.write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf8')
    names=[n for n in records if any(k in n.lower() for k in ('plasma','lightning','glass_orb','pillar'))]
    if args.export:
        from PIL import Image
        raw=Path('.tools/sts2-extracted');raw.mkdir(parents=True,exist_ok=True)
        images=Path('dist/images/sts2');images.mkdir(parents=True,exist_ok=True)
        selected=[n for n in records if n.startswith('.godot/imported/') and any(n.split('/')[-1].startswith(s) for s in ('cubex_construct.','lightning_orb.png-','plasma_orb.png-','glass_orb.png-'))]
        hashes={}
        for name in selected:
            data=read(args.pck,records[name]);raw.joinpath(name.split('/')[-1]).write_bytes(data)
            hashes[name]={'md5':records[name]['md5'],'sha256':hashlib.sha256(data).hexdigest()}
            if name.endswith('.ctex'):
                if data[:4]!=b'GST2' or struct.unpack_from('<I',data,36)[0] not in (1,2):raise ValueError('Unsupported texture codec')
                size=struct.unpack_from('<I',data,52)[0]
                Image.open(io.BytesIO(data[56:56+size])).save(images/(name.split('/')[-1].split('.png')[0]+'.png'))
        subprocess.run([sys.executable,'scripts/read-cubex-setup.py'],check=True,stdout=subprocess.DEVNULL)
        subprocess.run([sys.executable,'scripts/render-cubex-setup.py'],check=True)
        subprocess.run([sys.executable,'scripts/render-cubex-setup.py','--shooter'],check=True)
        report={'source_game':str(args.pck.parent),'source_pck':args.pck.name,'format':'Godot PCK v3; unencrypted, per-entry MD5 verified','sources':hashes,'outputs':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in images.glob('*.png') if p.name.startswith(('pillar','shooter','lightning_orb','plasma_orb','glass_orb'))},'pillar_pose':'Original Cubex Construct atlas, moss1 + circleeye skins, original weighted geometry and setup transforms','shooter_pose':'Same original atlas, moss2 + squareeye skins; bottom/core/top exported separately for both plants'}
        Path('dist/sts2-assets-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
        print('Exported construct sprites and animation layers; verified',len(selected),'source entries.')
    else: print(json.dumps({'entries':len(records),'matches':names},ensure_ascii=False,indent=2))
