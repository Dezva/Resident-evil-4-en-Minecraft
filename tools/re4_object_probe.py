"""Read-only snapshot of nearby actors and pickup-dialog flags. Never calls the game."""
import argparse,json,struct,math
from pathlib import Path
from re4_reader import WindowsProcess,find_pid,locate_player_slot,sample_player,code_sections,matches,u32,DEFAULT_EXE,ReaderError
ENEMY='81 E1 01 02 00 00 83 F9 01 75 ? 50 B9 ? ? ? ? E8'
GLOBAL='A1 ? ? ? ? B9 FF FF FF 7F 21 48 ? A1'
MESSAGE='51 52 6A 04 B9 ? ? ? ? 8B F0 E8'

def snapshot(read,base,size,slot, hazards=False):
    pose=sample_player(read,slot,base,size);position=pose.get('position_raw')
    blobs=[(address,read(address,n)) for address,n in code_sections(read,base,size)]
    def operands(pattern,offset):return sorted({u32(read(address+i+offset,4)) for address,data in blobs for i in matches(data,pattern)})
    result=dict(mode='READ ONLY',player=pose,pools=[],messages=[],code={})
    globals=operands(GLOBAL,1)
    if len(globals)==1:
        address=u32(read(globals[0],4));result['global_routine']=read(address+0x20,1)[0];result['status_flags']=[hex(v) for v in struct.unpack('<4I',read(address+0x501C,16))]
    for manager in operands(ENEMY,13):
        row=dict(manager_rva=hex(manager-base),actors=[]);result['pools'].append(row)
        try:
            vt,array,count,stride=struct.unpack('<4I',read(manager,16));row.update(count=count,stride=stride)
            if not base<=vt<base+size or not 0<count<=4096 or not 0x104<=stride<=65536 or count*stride>32*1024*1024:continue
            for i in range(count):
                actor=read(array+i*stride,min(stride,0x408));flags=u32(actor,4);vt=u32(actor)
                if flags&0x601!=1 or not base<=vt<base+size:continue
                pos=struct.unpack_from('<3f',actor,0x94)
                if not position or not all(math.isfinite(v) for v in pos) or sum((pos[j]-position[j])**2 for j in range(3))>15000**2:continue
                hp=list(struct.unpack_from('<2h',actor,0x324)) if len(actor)>=0x328 else None
                item=dict(slot=i,vtable_rva=hex(vt-base),kind=hex(actor[0x100]),flags=hex(flags),routine=list(actor[0xFC:0x100]),position=pos,hp=hp)
                if len(actor)>=0x378:item.update(message=actor[0x328:0x344].hex(' '),part=actor[0x344:0x378].hex(' '))
                row['actors'].append(item)
                if len(row['actors'])==64:break
        except (ReaderError,ValueError,struct.error) as error:row['error']=str(error)
    for manager in operands(MESSAGE,5):
        if not base<=manager<=base+size-0x101c:continue
        for i in range(16):
            data=read(manager+8+i*0x100,0x100)
            if not u32(data,8) and not data[0xf8]:continue
            result['messages'].append(dict(slot=i,flags=hex(u32(data,8)),routine=list(data[0xc:0x10]),state=hex(u32(data,0x10)),choices=data[0xf8],selection=data[0xf9],current=data[0xfa],jump_max=data[0x9f]))
    entries=[('box_move',0x4827),('object_box_move',0x9F52),('barrel_move',0x1A5F),('apply_damage',0x1B5AF0)]
    if hazards:entries.append(('trap_move',0xA209))
    for name,rva in entries:
        address=base+rva;entry=read(address,5)
        if entry[0]==0xE9:address+=5+struct.unpack_from('<i',entry,1)[0]
        if base<=address<=base+size-768:result['code'][name]=dict(rva=hex(address-base),bytes=read(address,768).hex(' '))
    if hazards:
        # The observed cEm2a update selects two impact handlers by subtype.
        # Read only their bounded bodies; never call either function.
        move=result['code'].get('trap_move')
        if move:
            start=base+int(move['rva'],16);body=bytes.fromhex(move['bytes'])
            prefix=bytes.fromhex('56 8b f1 80 be fc 00 00 00 00 74 1b 0f b6 86 01 01 00 00 48 74 0a 48 74 07')
            if body.startswith(prefix):
                for name,offset in [('trap_impact',0x19),('mine_impact',0x22)]:
                    if body[offset]!=0xe8:continue
                    target=start+offset+5+struct.unpack_from('<i',body,offset+1)[0]
                    if base<=target<=base+size-2048:result['code'][name]=dict(rva=hex(target-base),bytes=read(target,2048).hex(' '))
        result['trap_details']=[]
        for row in result['pools']:
            for actor in row['actors']:
                if actor['kind']!='0x2a':continue
                vt=base+int(actor['vtable_rva'],16)
                if base<=vt<=base+size-0x60:
                    result['trap_details'].append(dict(slot=actor['slot'],vtable=read(vt,0x60).hex(' ')))
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--game-exe',default=DEFAULT_EXE);parser.add_argument('--hazards',action='store_true');args=parser.parse_args()
    p=WindowsProcess(find_pid(),args.game_exe)
    try:report=snapshot(p.read,p.base,p.size,locate_player_slot(p.read,p.base,p.size),args.hazards)
    finally:p.close()
    output=Path(__file__).resolve().parents[1]/('trampas.json' if args.hazards else 'cajas-menu.json');output.write_text(json.dumps(report,indent=2),encoding='utf-8')
    print('Informe:',output);print('Solo lectura; no se llamo ni modifico el juego.')
    return 0
if __name__=='__main__':raise SystemExit(main())
