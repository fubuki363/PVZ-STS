#!/usr/bin/env python3
"""Package the current verified Windows executable and the user's local assets."""
import hashlib
import argparse
import json
from pathlib import Path
import subprocess
import zipfile
import pefile

root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser()
parser.add_argument('--verification',type=Path,default=root/'.tools/glass-final-check/sandbox-verification.json')
parser.add_argument('--archive',type=Path,default=root/'PvZ-Windows-HD-Glass.zip')
args=parser.parse_args()
dist=root/'dist'
exe=dist/'pvz-portable-glass.exe'
verification=json.loads(args.verification.read_text())
if not verification['passed']:raise RuntimeError('Native runtime verification did not pass')
if not verification.get('construct_assets') or verification.get('animation_frames',0)!=24:
    raise RuntimeError('Construct assets and animation capture were not verified')
if not verification.get('shooting_hits') or not verification.get('stoke_rules'):
    raise RuntimeError('Real projectile hits and Stoke row transformation were not verified')
if not verification.get('glass_rules'):
    raise RuntimeError('Glass penetration and burst save/load were not verified')
sha=hashlib.sha256(exe.read_bytes()).hexdigest()
pe=pefile.PE(str(exe))
report=json.loads((dist/'build-report.json').read_text(encoding='utf8'))
report.update(executable=exe.name,bytes=exe.stat().st_size,sha256=sha,
              architecture=hex(pe.FILE_HEADER.Machine),subsystem=pe.OPTIONAL_HEADER.Subsystem,
              dll_imports=[i.dll.decode() for i in pe.DIRECTORY_ENTRY_IMPORT])
report['verification']['sandbox_runtime']=verification
report['orb_mod']={'slots':2,'starting_peashooter_orb':'Lightning','starting_pillar_orb':'Plasma',
                   'glass_card_slot':5,'glass_card_sun_cost':25,'glass_card_recharge_frames':200,
                   'glass_projectile_damage':10,'glass_projectile_max_targets':3,
                   'burst_queue':'FIFO batches preserve each evoked type, including during overlapping releases',
                   'plasma_sun_per_orb':25,'production_frames':[2350,2500],
                   'lightning_card_sun_cost':25,'lightning_card_recharge_frames':200,
                   'overflow':'evoke oldest then append new orb','evoke_projectiles':20,
                   'evoke_projectile_spacing_frames':6,'pea_damage':20,
                   'save_format':'PVZP v4; plant fields 101/102/103 and projectile fields 101/102'}
report['construct_visuals']={'source':'Local Slay the Spire 2 Cubex Construct atlas and weighted setup geometry',
    'pillar_skin':['moss1','circleeye'],'peashooter_skin':['moss2','squareeye'],
    'animation':'Upper and lower body sections bob vertically in opposite phases; core floats gently',
    'period_seconds':1.9635,'upper_amplitude_pixels':4.5,'lower_amplitude_pixels':2.5,
    'cards_and_cursor':'Scaled construct sprites','projectile_origin':'Upper section; ground clearance at least 40px'}
report['stoke_mod']={'seed_id':20,'name':'添柴辣椒','starter_card_slot':4,'sun_cost':125,
    'recharge_frames':5000,'targets':'Each plant in the clicked row, including occupied rows',
    'random_pool':'Current seedbank slots, excluding Stoke/Lightning/Glass skills; compatible terrain and layers only',
    'new_state':'Fresh health, timers, and initial orbs','invalid_target':'No charge or cooldown when no compatible target'}
(dist/'build-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf8')
debug={'executable':exe.name,'sha256':sha,'bytes':exe.stat().st_size,'plant_cards':55,
       'zombie_types':33,'scenes':10,'card_slots':[1,10],
       'features':['all cards in three pages','all zombies in three pages','all backgrounds',
                   'row/batch spawning','clear plants/zombies','infinite sun','no cooldown',
                   'free placement','freeze','wake mushrooms','no loss','channel/evoke tools','Stoke row transformation'],
       'profile_isolation':'Temporary PlayerInfo copy; saving and normal-level file deletion disabled',
       'verification':verification}
(dist/'debug-report.json').write_text(json.dumps(debug,ensure_ascii=False,indent=2),encoding='utf8')
(dist/'defect-report.json').write_text(json.dumps(report['orb_mod'],ensure_ascii=False,indent=2),encoding='utf8')
archive=args.archive
files=[]
for name in (exe.name,'main.pak','LICENSE','COPYING','build-report.json',
             'resource-report.json','hd-resources-report.json','debug-report.json','defect-report.json','sts2-assets-report.json'):
    files.append(dist/name)
for folder in ('properties','licenses'):files.extend(p for p in (dist/folder).rglob('*') if p.is_file())
files.extend(dist/'images/sts2'/n for n in ('pillar.png','shooter.png','lightning_orb.png','plasma_orb.png','glass_orb.png'))
files.extend(dist/'images/sts2'/f'{variant}_{part}.png' for variant in ('pillar','shooter') for part in ('bottom','core','top'))
files.extend(p for p in root.glob('*.cmd') if p.name!='导入资源.cmd')
files.append(root/'WINDOWS-MOD.md')
patch=subprocess.check_output(['git','diff','--binary'],cwd=root)
new_files=subprocess.check_output(['git','ls-files','--others','--exclude-standard','-z'],cwd=root).decode('utf8').split('\0')
source_files=[root/p for p in new_files if p and Path(p).suffix in ('.cpp','.h','.py','.ps1','.md','.json','.cmd')]
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED,compresslevel=6) as z:
    for p in files:z.write(p,p.relative_to(root).as_posix())
    for name in ('sandbox-verification.json','shooting-verification.csv','stoke-verification.csv','glass-verification.csv'):
        z.write(args.verification.parent/name,'verification/'+name)
    z.writestr('source-changes/tracked.patch',patch)
    for p in source_files:z.write(p,'source-changes/'+p.relative_to(root).as_posix())
with zipfile.ZipFile(archive) as z:
    bad=z.testzip()
    if bad:raise RuntimeError('ZIP verification failed: '+bad)
    if hashlib.sha256(z.read('dist/'+exe.name)).hexdigest()!=sha:raise RuntimeError('Packaged executable differs')
    if any('/savedata/' in n or '/debugdata/' in n for n in z.namelist()):raise RuntimeError('User save data included')
print(json.dumps({'archive':str(archive),'bytes':archive.stat().st_size,'executable_sha256':sha,
                  'game_files':len(files),'native_verification':verification},ensure_ascii=True,indent=2))
