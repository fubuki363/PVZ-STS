"""Rasterize the original atlas geometry in its stored setup pose; no new artwork."""
import json,math
from pathlib import Path
import numpy as np
from PIL import Image
d=json.loads(Path('.tools/cubex-setup.json').read_text())
atlas=Image.open('dist/images/sts2/cubex_construct.png').convert('RGBA')
text=json.loads(next(Path('.tools/sts2-extracted').glob('cubex_construct.atlas*')).read_text().rstrip('\0'))['atlas_data']
regions={};current=None
for line in text.splitlines()[4:]:
 if ':' not in line:current=line;regions[current]={}
 else:
  key,value=line.split(':',1);regions[current][key]=[int(x) for x in value.split(',')]
textures={}
for name,r in regions.items():
 x,y,w,h=r['bounds'];rot=r.get('rotate',[0])[0]
 image=atlas.crop((x,y,x+(h if rot else w),y+(w if rot else h)))
 if rot:image=image.transpose(Image.Transpose.ROTATE_90)
 textures[name]=image
matrices=[]
for b in d['bones']:
 rot,x,y,sx,sy,shx,shy,length=b['values'];a=math.radians(rot+shx);c=math.radians(rot+90+shy)
 m=np.array([[math.cos(a)*sx,math.cos(c)*sy,x],[math.sin(a)*sx,math.sin(c)*sy,y],[0,0,1.]])
 matrices.append(matrices[b['parent']]@m if b['parent']>=0 else m)
import argparse
parser=argparse.ArgumentParser();parser.add_argument('--shooter',action='store_true');args=parser.parse_args()
prefix='shooter' if args.shooter else 'pillar'
attachments={(a['slot'],a['key']):a for a in d['default']}
for skin in d['skins']:
 if skin['name'] in (('moss2','squareeye') if args.shooter else ('moss1','circleeye')):
  attachments.update({(a['slot'],a['key']):a for a in skin['attachments']})
# Spine linked meshes share their parent's geometry but keep their own texture.
skin_attachments=[d['default']]+[s['attachments'] for s in d['skins']]
def resolve_mesh(a):
 if a['type']!=3:return a
 parent=next(p for p in skin_attachments[a['skin']] if p['slot']==a['slot'] and p['key']==a['parent'])
 geometry=resolve_mesh(parent).copy()
 geometry.update({k:v for k,v in a.items() if k not in ('type','skin','parent')})
 return geometry
attachments={key:resolve_mesh(a) for key,a in attachments.items()}
scale=.438;canvas=np.zeros((440,310,4),dtype=float)
layers={part:np.zeros_like(canvas) for part in ('bottom','core','top')}
origin=np.array([120,402])
for n,s in enumerate(d['slots']):
 a=attachments.get((n,s['attachment']))
 if not a or a['type'] not in (0,2):continue
 part='bottom' if n in (3,4,5,23) else 'core' if 6<=n<=14 else 'top' if n in (15,16,17,22) else None
 tex=textures[a['path']];r=regions[a['path']]
 if a['type']==0:
  x,y,sx,sy,w,h=a['values'];rot=math.radians(a['rotation']);matrix=matrices[s['bone']]@np.array([[math.cos(rot)*sx,-math.sin(rot)*sy,x],[math.sin(rot)*sx,math.cos(rot)*sy,y],[0,0,1.]])
  ox,oy,ow,oh=r.get('offsets',[0,0,tex.width,tex.height]);lx=-w/2+ox*w/ow;ly=-h/2+oy*h/oh;rx=lx+tex.width*w/ow;ty=ly+tex.height*h/oh
  vertices=np.array([(matrix@np.array([px,py,1.]))[:2] for px,py in ((lx,ty),(rx,ty),(rx,ly),(lx,ly))])
  uvs=np.array([[0,0],[1,0],[1,1],[0,1]]);triangles=[0,1,2,0,2,3]
 else:
  if a['weighted']:
   vertices=np.array([sum(((matrices[bn]@np.array([x,y,1]))[:2])*wt for bn,x,y,wt in v) for v in a['vertices']])
  else:vertices=np.array([(matrices[s['bone']]@np.array([*v,1]))[:2] for v in a['vertices']])
  uvs=np.array(a['uvs']).reshape(-1,2);triangles=a['triangles']
 vertices=vertices*np.array([scale,-scale])+origin
 texarr=np.asarray(tex,dtype=float)/255
 tint=np.array(s['color'],dtype=float)/255*np.array(a.get('color',[255]*4))/255
 for i in range(0,len(triangles),3):
  ids=triangles[i:i+3];v=vertices[ids];uv=uvs[ids];lo=np.maximum(0,np.floor(v.min(axis=0)).astype(int));hi=np.minimum([310,440],np.ceil(v.max(axis=0)).astype(int))
  if np.any(hi<=lo):continue
  yy,xx=np.mgrid[lo[1]:hi[1],lo[0]:hi[0]];xy=np.stack([xx+.5,yy+.5,np.ones_like(xx)],axis=-1)
  m=np.vstack([v.T,np.ones(3)])
  try:bary=xy@np.linalg.inv(m).T
  except np.linalg.LinAlgError:continue
  mask=np.all(bary>=-1e-6,axis=-1);coords=bary@uv
  tx=np.clip(coords[:,:,0]*(tex.width-1),0,tex.width-1);ty=np.clip(coords[:,:,1]*(tex.height-1),0,tex.height-1)
  x0=tx.astype(int);y0=ty.astype(int);x1=np.minimum(x0+1,tex.width-1);y1=np.minimum(y0+1,tex.height-1);fx=(tx-x0)[...,None];fy=(ty-y0)[...,None]
  pixels=((texarr[y0,x0]*(1-fx)+texarr[y0,x1]*fx)*(1-fy)+(texarr[y1,x0]*(1-fx)+texarr[y1,x1]*fx)*fy)*tint
  pixels[:,:,3]*=mask
  for target in ([canvas,layers[part]] if part else [canvas]):
   dst=target[lo[1]:hi[1],lo[0]:hi[0]];alpha=pixels[:,:,3:4];old=dst[:,:,3:4];new=alpha+old*(1-alpha)
   dst[:,:,:3]=np.divide(pixels[:,:,:3]*alpha+dst[:,:,:3]*old*(1-alpha),new,out=np.zeros_like(dst[:,:,:3]),where=new>0);dst[:,:,3:4]=new
image=Image.fromarray(np.clip(canvas*255,0,255).astype('uint8'));bounds=image.getbbox();image=image.crop(bounds)
image.save(f'dist/images/sts2/{prefix}.png')
for part,pixels in layers.items():
 Image.fromarray(np.clip(pixels*255,0,255).astype('uint8')).crop(bounds).save(f'dist/images/sts2/{prefix}_{part}.png')
print(prefix,image.size,'three independently moving layers')
