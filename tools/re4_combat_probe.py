"""Read-only executable profile for native impact messages. Never calls or patches RE4."""
import argparse,json,struct
from pathlib import Path
from re4_reader import WindowsProcess,find_pid,locate_player_slot,code_sections,DEFAULT_EXE

FUNCTIONS={'apply_damage':0x1B5AF0,'bullet_ray':0x1BB3C0,'damage_message':0x1AEEF0}

def describe_profile(read,base,size):
    pe=struct.unpack_from('<I',read(base,64),0x3C)[0]
    timestamp=struct.unpack_from('<I',read(base+pe+8,4))[0]
    sections=[(address,read(address,length)) for address,length in code_sections(read,base,size)]
    result=dict(mode='READ ONLY',timestamp=hex(timestamp),expected_timestamp='0x5a6192e5',functions={})
    for name,rva in FUNCTIONS.items():
        target=base+rva;entries=[]
        for address,data in sections:
            for i in range(5,min(len(data)-9,base+0x20000-address)):
                if data[i-5]==data[i]==data[i+5]==0xE9 and address+i+5+struct.unpack_from('<i',data,i+1)[0]==target:entries.append(address+i)
        calls=0
        for address,data in sections:
            start=0
            while True:
                i=data.find(b'\xe8',start)
                if i<0 or i+5>len(data):break
                if address+i+5+struct.unpack_from('<i',data,i+1)[0] in entries:calls+=1
                start=i+1
        result['functions'][name]=dict(rva=hex(rva),bytes=read(target,64).hex(' '),ilt_entries=[hex(x-base) for x in entries],ilt_calls=calls)
    return result

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--game-exe',default=DEFAULT_EXE);args=parser.parse_args()
    process=WindowsProcess(find_pid(),args.game_exe)
    try:
        locate_player_slot(process.read,process.base,process.size)
        result=describe_profile(process.read,process.base,process.size)
    finally:process.close()
    output=Path(__file__).resolve().parents[1]/'perfil-impactos.json';output.write_text(json.dumps(result,indent=2),encoding='utf-8')
    print(json.dumps(result,indent=2));print('Informe:',output);print('Solo lectura: no se modifico ni se llamo a ninguna funcion de RE4.')
    return 0

if __name__=='__main__':raise SystemExit(main())
