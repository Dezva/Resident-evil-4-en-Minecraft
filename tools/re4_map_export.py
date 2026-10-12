"""Read-only local SAT/EAT collision export. Never edits RE4, Minecraft, hooks or IPC."""
import argparse
import datetime
import json
import math
from pathlib import Path
import struct

from re4_reader import DEFAULT_EXE, ReaderError, WindowsProcess, code_sections, find_pid, locate_player_slot, matches, sample_player, u32

REFERENCE='nipkownix/re4_tweaks@92c0208bd09c29c9640e13a2493b8e6bd6edd700:Game.cpp/SDK/atari.h/cManager.h'
SIGNATURES={'SAT':('8D 8E B4 02 00 00 E8 ? ? ? ? 6A 00 56 B9',15),
            'EAT':('6A 00 53 56 50 51 B9 ? ? ? ? E8',7)}
MAX_BYTES=32*1024*1024


def extract(read,base,size,position,radius=20000):
    if len(position)!=3 or not all(math.isfinite(v) and abs(v)<=20_000_000 for v in position) or not 1000<=radius<=60000:
        raise ReaderError('Rango de geometria no valido')
    report=dict(mode='READ ONLY COLLISION GEOMETRY',reference=REFERENCE,position=position,radius_native=radius,
                complete=True,managers=[],triangles=[],note='Poligonos de colision, no malla visual/texturas. No activa un mapa ni impide colocaciones todavia.')
    budget=0
    def bounded(address,length,aligned=True):
        nonlocal budget
        if address<0x10000 or (aligned and address%4) or length<0 or length>32*1024*1024 or address+length>0x100000000 or budget+length>MAX_BYTES:
            raise ReaderError('Lectura de geometria fuera de limite')
        budget+=length;data=read(address,length)
        if len(data)!=length:raise ReaderError('Lectura de geometria incompleta')
        return data
    blobs=[(address,bounded(address,length)) for address,length in code_sections(read,base,size)]
    for name,(signature,offset) in SIGNATURES.items():
        row=dict(name=name,objects=0,enabled=0,nearby_triangles=0,errors=[]);report['managers'].append(row)
        hits=[address+i for address,data in blobs for i in matches(data,signature)]
        if len(hits)!=1:
            report['complete']=False;row['errors'].append(f'Firma {name} ausente/ambigua: {len(hits)}');continue
        try:
            manager=u32(bounded(hits[0]+offset,4,False))
            if not base<=manager<=base+size-32 or manager%4:raise ReaderError('Manager fuera del modulo')
            header=bounded(manager,32);vtable,array,count,stride=struct.unpack_from('<4I',header)
            row.update(manager_rva=hex(manager-base),array_address=hex(array),stride=stride,vtable_rva=hex(vtable-base),header_hex=header.hex(' '))
            if not base<=vtable<base+size or count>2048 or stride!=0xC0:raise ReaderError('Layout de manager incompatible')
            row['objects']=count
            if count:
                objects=bounded(array,count*stride)
                for index in range(count):
                    obj=objects[index*stride:(index+1)*stride]
                    if u32(obj,4)&0x201!=1 or not obj[0x2a]&4:continue
                    row['enabled']+=1
                    try:
                        if not base<=u32(obj)<base+size:raise ReaderError('Vtable de cSat incompatible')
                        vertices_ptr=u32(obj,0xC);polygons_ptr=u32(obj,0x18)
                        nvertices,npolygons,nfloor,nslope,nwall=struct.unpack_from('<5H',obj,0x1C)
                        if not nvertices or not npolygons or nfloor+nslope+nwall>npolygons:raise ReaderError('Contadores cSat incompatibles')
                        transform=struct.unpack_from('<12f',obj,0x60)
                        if not all(math.isfinite(v) and abs(v)<=20_000_000 for v in transform):raise ReaderError('Transformacion cSat no valida')
                        vertices=bounded(vertices_ptr,nvertices*12);polygons=bounded(polygons_ptr,npolygons*20)
                        transformed=[]
                        for i in range(nvertices):
                            local=struct.unpack_from('<3f',vertices,i*12)
                            world=[sum(transform[4*r+c]*local[c] for c in range(3))+transform[4*r+3] for r in range(3)]
                            if not all(math.isfinite(v) and abs(v)<=20_000_000 for v in world):raise ReaderError('Vertice no valido')
                            transformed.append(world)
                        triangles=[]
                        for i in range(npolygons):
                            poly=struct.unpack_from('<10H',polygons,i*20)
                            if max(poly[:3])>=nvertices:raise ReaderError('Indice de vertice fuera de rango')
                            points=[transformed[v] for v in poly[:3]]
                            if any(min(p[axis] for p in points)>position[axis]+radius or max(p[axis] for p in points)<position[axis]-radius for axis in range(3)):continue
                            a,b,c=points;ab=[b[k]-a[k] for k in range(3)];ac=[c[k]-a[k] for k in range(3)]
                            cross=[ab[1]*ac[2]-ab[2]*ac[1],ab[2]*ac[0]-ab[0]*ac[2],ab[0]*ac[1]-ab[1]*ac[0]]
                            if sum(v*v for v in cross)<1e-8:continue
                            kind='floor' if i<nfloor else 'slope' if i<nfloor+nslope else 'wall' if i>=npolygons-nwall else 'other'
                            triangles.append(dict(source=name,object=index,kind=kind,vertices=points,attributes=(poly[8]<<16)|poly[9]))
                            if len(report['triangles'])+len(triangles)>50000:raise ReaderError('Demasiados poligonos locales')
                        # No mixed object pointers/counts/transforms during a room/object update.
                        if bounded(array+index*stride,stride)!=obj:raise ReaderError('cSat cambio durante la lectura; repite quieto')
                        report['triangles'].extend(triangles);row['nearby_triangles']+=len(triangles)
                    except ReaderError as error:
                        report['complete']=False;row['errors'].append(f'Objeto {index}: {error}')
                if bounded(manager,32)!=header:raise ReaderError('Manager cambio durante la lectura; repite quieto')
        except ReaderError as error:
            report['complete']=False;row['errors'].append(str(error))
    report['bytes_read']=budget
    return report


def write_outputs(report,root):
    root=Path(root);output=root.with_suffix('.json');output.write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    # Relative coordinates and metres/blocks for inspection, not a visual asset export.
    player=report['position'];lines=['# RE4 local collision triangles; 1000 native units = 1 provisional block']
    for i,tri in enumerate(report['triangles']):
        lines.append('g '+tri['source']+'_'+tri['kind'])
        for p in tri['vertices']:lines.append('v '+' '.join(f'{(p[k]-player[k])/1000:.6f}' for k in range(3)))
        lines.append(f'f {3*i+1} {3*i+2} {3*i+3}')
    root.with_suffix('.obj').write_text('\n'.join(lines)+'\n',encoding='utf-8')
    data=json.dumps([dict(kind=t['kind'],v=[[(p[k]-player[k])/1000 for k in range(3)] for p in t['vertices']]) for t in report['triangles']]).replace('</','<\\/')
    template=Path(__file__).with_name('re4_map_viewer.html').read_text(encoding='utf-8')
    root.with_suffix('.html').write_text(template.replace('/* COLLISION_DATA */',data).replace('/* COMPLETE */','true' if report['complete'] else 'false'),encoding='utf-8')
    return output


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--game-exe',default=DEFAULT_EXE)
    parser.add_argument('--radius-blocks',type=float,default=20);args=parser.parse_args()
    process=None
    try:
        process=WindowsProcess(find_pid(),args.game_exe);slot=locate_player_slot(process.read,process.base,process.size)
        sample=sample_player(process.read,slot,process.base,process.size)
        if sample['state']!='player':raise ReaderError('Carga una partida y deja a Leon quieto antes de exportar')
        pointer=u32(process.read(slot,4));report=extract(process.read,process.base,process.size,sample['position_raw'],args.radius_blocks*1000)
        if u32(process.read(slot,4))!=pointer:raise ReaderError('Cambio de personaje/zona durante la exportacion')
        stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S');report['generated_utc']=datetime.datetime.now(datetime.timezone.utc).isoformat()
        output=write_outputs(report,Path(__file__).resolve().parents[1]/('re4-map-'+stamp))
        print('Geometria local:',len(report['triangles']),'triangulos; lectura completa:',report['complete'])
        print('Informe:',output);print('Vista 3D:',output.with_suffix('.html'))
        print('No se modifico RE4 ni Minecraft. Este exportador no activa las colisiones compartidas.')
        return 0
    except (OSError,ValueError,ReaderError,struct.error) as error:print('ERROR:',error);return 1
    finally:
        if process is not None:process.close()


if __name__=='__main__':raise SystemExit(main())
