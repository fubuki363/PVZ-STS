import struct,json
from pathlib import Path
data=next(Path('.tools/sts2-extracted').glob('cubex_construct.skel*')).read_bytes()
pos=8
def byte():
 global pos
 v=data[pos];pos+=1;return v
def var():
 value=0
 for shift in range(0,35,7):
  b=byte();value|=(b&127)<<shift
  if b<128:return value
def string():
 global pos
 n=var()
 if not n:return None
 s=data[pos:pos+n-1].decode();pos+=n-1;return s
def fl():
 global pos
 v=struct.unpack_from('>f',data,pos)[0];pos+=4;return v
print('version',string(),'bounds',[fl() for _ in range(5)])
ne=byte()
if ne:print('ne',fl(),string(),string())
strings=[string() for _ in range(var())];print('strings',strings)
bones=[]
for i in range(var()):
 name=string();parent=var() if i else -1;floats=[fl() for _ in range(8)];inherit=var();required=byte()
 if ne:pos+=4;icon=string();visible=byte()
 bones.append(dict(name=name,parent=parent,values=floats,inherit=inherit))
print(json.dumps(bones,indent=1))
slots=[]
for i in range(var()):
 name=string();bone=var();color=[byte() for _ in range(4)];pos+=4;ref=var();blend=var()
 if ne:visible=byte()
 slots.append(dict(name=name,bone=bone,color=color,attachment=strings[ref-1] if ref else None,blend=blend))
print(json.dumps(slots,indent=1));print('constraints offset',pos,'next',data[pos:pos+10].hex())
assert var()==0 # IK
for _ in range(var()):
 name=string();order=var();bs=[var() for _ in range(var())];target=var();flags=byte()
 values=[fl() for bit in (8,16,32,64,128) if flags&bit];flags2=byte()
 values += [fl() for bit in (1,2,4,8,16,32,64) if flags2&bit]
 print('transform',name,bs,target,values)
assert var()==0 # paths
assert var()==0 # physics
def ref():
 n=var();return strings[n-1] if n else None
def vertices(weighted):
 n=var()
 if weighted:return [[(var(),fl(),fl(),fl()) for _ in range(var())] for _ in range(n)]
 return [[fl(),fl()] for _ in range(n)]
def attachments(n):
 result=[]
 for _ in range(n):
  slot=var()
  for _ in range(var()):
   key=ref();flags=byte();name=ref() if flags&8 else key;typ=flags&7
   a=dict(slot=slot,key=key,name=name,type=typ)
   if typ in (0,2,3):
    a['path']=ref() if flags&16 else name
    if flags&32:a['color']=[byte() for _ in range(4)]
    if flags&64:a['sequence']=[var() for _ in range(4)]
   if typ==0:
    a['rotation']=fl() if flags&128 else 0;a['values']=[fl() for _ in range(6)]
   elif typ==2:
    hull=var();a['weighted']=bool(flags&128);a['vertices']=vertices(a['weighted']);n=len(a['vertices'])*2
    a['uvs']=[fl() for _ in range(n)];a['triangles']=[var() for _ in range((n-hull-2)*3)]
    if ne:a['edges']=[var() for _ in range(var())];a['size']=[fl(),fl()]
   elif typ==3:
    a['skin']=var();a['parent']=ref()
    if ne:a['size']=[fl(),fl()]
   elif typ==6:
    a['end']=var();a['vertices']=vertices(bool(flags&16))
    if ne:pos_inc(4)
   else:raise ValueError(typ)
   result.append(a)
 return result
def pos_inc(n):
 global pos
 pos+=n
default=attachments(var());skins=[]
for _ in range(var()):
 name=string()
 if ne:pos+=4
 for i in range(5):[var() for _ in range(var())]
 skins.append(dict(name=name,attachments=attachments(var())))
print('skins',[(s['name'],len(s['attachments'])) for s in skins])
Path('.tools/cubex-setup.json').write_text(json.dumps({'bones':bones,'slots':slots,'default':default,'skins':skins},indent=2))
