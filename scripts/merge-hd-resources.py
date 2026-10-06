from pathlib import Path
import io,json,sys,struct,hashlib
from PIL import Image
sys.path.insert(0,'scripts');import pak
root=Path('.tools/original-resources')
def records(p):
 data=pak.xor_data(p.read_bytes());entries,offset=pak.read_records(data);result=[]
 for name,size,stamp in entries:result.append([name,size,stamp,data[offset:offset+size]]);offset+=size
 return result
base=records(root/'main.pak');hd={n.as_posix().casefold():content for n,size,stamp,content in records(root/'hd-main.pak')}
changed=[];skipped=[]
for entry in base:
 name,size,stamp,content=entry;key=name.as_posix().casefold()
 if key in hd and hd[key]!=content and key.endswith(('.png','.jpg','.gif')):
  if Image.open(io.BytesIO(content)).size==Image.open(io.BytesIO(hd[key])).size:
   entry[1]=len(hd[key]);entry[3]=hd[key];changed.append(key)
  else:skipped.append(key)
header=bytearray(struct.pack('<II',pak.PAK_MAGIC,pak.PAK_VERSION))
for name,size,stamp,content in base:
 raw=name.as_posix().encode('utf-8');header.extend(struct.pack('<BB',0,len(raw)));header.extend(raw);header.extend(struct.pack('<iq',size,stamp))
header.append(pak.PAK_END_FLAG)
output=Path('dist/main.pak');temporary=output.with_suffix('.pak.tmp')
with temporary.open('wb') as f:
 pak.write_xored(f,header)
 for name,size,stamp,content in base:pak.write_xored(f,content)
# Validate the newly assembled container before replacing the working resources.
pak.read_records(pak.xor_data(temporary.read_bytes()))
temporary.replace(output)
report={'compatible_hd_images':len(changed),'skipped_dimension_changes':len(skipped),'replacements':changed,'skipped':skipped,'base_pak_sha256':hashlib.sha256((root/'main.pak').read_bytes()).hexdigest(),'hd_pak_sha256':hashlib.sha256((root/'hd-main.pak').read_bytes()).hexdigest(),'output_pak_sha256':hashlib.sha256(output.read_bytes()).hexdigest()}
Path('dist/hd-resources-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print('Compatible HD replacements:',len(changed),'Incompatible sizes retained from original:',len(skipped))
