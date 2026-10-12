import json
import shutil
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch
from re4_map_export import extract,write_outputs,SIGNATURES
from re4_reader import ReaderError

class MapExportTests(unittest.TestCase):
    def world(self):
        base=0x10000;size=0x10000;memory=bytearray(size)
        for j,(name,(signature,offset)) in enumerate(SIGNATURES.items()):
            code=0x100+j*0x100;manager=0x1000+j*0x100;array=0x2000+j*0x100;vertices=0x4000+j*0x100;polygons=0x6000+j*0x100
            pattern=bytes(0 if x=='?' else int(x,16) for x in signature.split());memory[code:code+len(pattern)]=pattern
            struct.pack_into('<I',memory,code+offset,base+manager)
            struct.pack_into('<4I',memory,manager,base+0x9000,base+array,1,0xC0)
            struct.pack_into('<2I',memory,array,base+0x9000,1)
            struct.pack_into('<I',memory,array+0xC,base+vertices);struct.pack_into('<I',memory,array+0x18,base+polygons)
            struct.pack_into('<5H',memory,array+0x1C,3,1,1,0,0);memory[array+0x2A]=4
            struct.pack_into('<12f',memory,array+0x60,1,0,0,100+j*1000,0,1,0,-400,0,0,1,1700)
            struct.pack_into('<9f',memory,vertices,0,0,0,1000,0,0,0,0,1000)
            struct.pack_into('<10H',memory,polygons,0,1,2,0,0,0,0,0,0x1000,0x20)
        def read(address,n):
            if not base<=address<=base+size-n:raise ReaderError('bad read')
            return bytes(memory[address-base:address-base+n])
        return base,size,memory,read
    def run_extract(self,read,base,size,radius=20000):
        with patch('re4_map_export.code_sections',return_value=[(base+0x100,0x300)]):return extract(read,base,size,[100,-400,1700],radius)
    def test_exports_transformed_triangles_attributes_and_offline_viewer_without_game_writes(self):
        base,size,memory,read=self.world();before=bytes(memory);report=self.run_extract(read,base,size)
        self.assertTrue(report['complete']);self.assertEqual(len(report['triangles']),2);self.assertEqual(before,bytes(memory))
        tri=report['triangles'][0];self.assertEqual(tri['vertices'][0],[100,-400,1700]);self.assertEqual(tri['vertices'][1],[1100,-400,1700])
        self.assertEqual(tri['kind'],'floor');self.assertEqual(tri['attributes'],0x10000020)
        with tempfile.TemporaryDirectory() as d:
            stem=Path(d)/'geometry';out=write_outputs(report,stem);self.assertEqual(json.loads(out.read_text()),report)
            self.assertIn('v 0.000000 0.000000 0.000000',stem.with_suffix('.obj').read_text())
            html=stem.with_suffix('.html').read_text();self.assertNotIn('/* COLLISION_DATA */',html);self.assertNotIn('/* COMPLETE */',html)
            self.assertNotIn('<script src=',html);self.assertIn('complete=true',html)
    def test_ambiguous_signature_counts_vertex_index_finite_layout_and_object_races_fail_closed(self):
        for mode in ('ambiguous','manager','index','nan','vtable','race'):
            base,size,memory,read=self.world()
            if mode=='ambiguous':
                memory[0x300:0x300+19]=memory[0x100:0x100+19];struct.pack_into('<I',memory,0x30f,base+0x1100)
            if mode=='manager':struct.pack_into('<I',memory,0x1008,99999)
            if mode=='index':struct.pack_into('<H',memory,0x6000,65535)
            if mode=='nan':struct.pack_into('<f',memory,0x4060,float('nan'));struct.pack_into('<f',memory,0x4000,float('nan'))
            if mode=='vtable':struct.pack_into('<I',memory,0x2000,4)
            if mode=='race':
                original=read
                def read(address,n):
                    data=original(address,n)
                    if address==base+0x2000 and n==0xC0:memory[0x202a]=0
                    return data
            with self.subTest(mode=mode):
                report=self.run_extract(read,base,size);self.assertFalse(report['complete']);self.assertTrue(report['managers'][0]['errors'])
                self.assertEqual([t for t in report['triangles'] if t['source']=='SAT'],[])
    def test_eighteen_aliases_require_the_same_verified_manager_and_reject_disagreement(self):
        base,size,memory,read=self.world()
        for i in range(17):memory[0x300+i*32:0x300+i*32+19]=memory[0x100:0x100+19]
        with patch('re4_map_export.code_sections',return_value=[(base+0x100,0x800)]):
            report=extract(read,base,size,[100,-400,1700]);self.assertTrue(report['complete'])
            self.assertEqual(report['managers'][0]['signature_matches'],18);self.assertEqual(len(report['triangles']),2)
            struct.pack_into('<I',memory,0x30f,base+0x1100)
            report=extract(read,base,size,[100,-400,1700]);self.assertFalse(report['complete'])
            self.assertEqual(len(report['managers'][0]['references']),18)
            self.assertEqual([t for t in report['triangles'] if t['source']=='SAT'],[])
    def test_scan_accepts_real_seven_megabyte_section_and_local_filter_and_empty_disabled_manager(self):
        base,size,memory,read=self.world()
        # Explicit executable section bounds come from the production PE validator.
        def padded(address,n):return read(address,n) if n<7_400_000 else read(base+0x100,0x300)+b'\0'*(n-0x300)
        with patch('re4_map_export.code_sections',return_value=[(base+0x100,7_400_000)]):
            report=extract(padded,base,size,[100,-400,1700],1000)
            self.assertTrue(report['complete']);self.assertGreater(report['bytes_read'],7_400_000)
        memory[0x202a]=0;memory[0x212a]=0;report=self.run_extract(read,base,size)
        self.assertTrue(report['complete']);self.assertEqual(report['triangles'],[])
    def test_cli_uses_real_reader_version_gate_and_writes_local_artifacts(self):
        import re4_map_export as module
        base,size,memory,read=self.world()
        class Process:
            def __init__(self,pid,path):self.base=base;self.size=size;self.read=read
            def close(self):pass
        # Version/player implementation is already covered by the reader suites;
        # CLI still invokes both before attempting collision discovery.
        with tempfile.TemporaryDirectory() as d,patch.object(module,'WindowsProcess',Process),patch.object(module,'find_pid',return_value=7),patch.object(module,'locate_player_slot',return_value=base+0x9000),patch.object(module,'sample_player',return_value={'state':'player','position_raw':[100,-400,1700]}),patch.object(module,'code_sections',return_value=[(base+0x100,0x300)]),patch.object(module,'__file__',str(Path(d)/'tools'/ 're4_map_export.py')),patch('sys.argv',['map']):
            (Path(d)/'tools').mkdir();shutil.copy(Path(__file__).with_name('re4_map_viewer.html'),Path(d)/'tools'/'re4_map_viewer.html')
            self.assertEqual(module.main(),0);self.assertEqual(len(list(Path(d).glob('re4-map-*.json'))),1)
