import struct
import unittest
from unittest.mock import Mock,patch
from pathlib import Path
import tempfile
import json
import contextlib
import io
import re4_graphics_probe
from re4_graphics_probe import inspect,Audit,FACTORY_SIGNATURE
from re4_reader import ReaderError
from test_re4_reader import Memory


class GraphicsMemory:
    def __init__(self,private_table=False,outside_method=False):
        self.game=Memory();self.regions=[];self.data={}
        self.modules=[dict(name='bio4.exe',base=self.game.base,size=self.game.size,path='bio4.exe'),
                      dict(name='d3d9.dll',base=0x2000000,size=0x10000,path=r'C:\Windows\SysWOW64\d3d9.dll'),
                      dict(name='dxgi.dll',base=0x3000000,size=0x10000,path=r'D:\Games\dxgi.dll')]
        self.add(self.game.base,len(self.game.data),4,0x1000000,self.game.data)
        self.add(0x1000000,4096,4,0x20000)
        self.add(0x1001000,4096,4,0x20000)
        self.add(0x1002000,4096,4,0x20000)
        self.add(0x2001000,4096,2,0x1000000)
        self.add(0x2002000,4096,0x20,0x1000000)
        self.add(0x3001000,4096,0x20,0x1000000)
        self.object=0x1000000;self.root=0x1001000;self.table=0x1002000 if private_table else 0x2001000
        self.put(self.object,self.table)
        for i in range(119):self.put(self.table+i*4,0x3001000 if outside_method and i==42 else 0x2002000+i*4)
        self.put(self.root+1024,self.object) # Beyond native0.4.12's256-byte prefix.
        self.put(self.game.base+0x3140,self.root)
        self.game.put_signature(0x1300,FACTORY_SIGNATURE,0)
        self.put(self.game.base+0x1306,self.game.base+0x3150)
        self.put(self.game.base+0x3150,self.object)

    def add(self,base,size,protect,kind,data=None):
        self.regions.append(dict(base=base,end=base+size,protect=protect,kind=kind,committed=True))
        self.data[base]=data if data is not None else bytearray(size)

    def region(self,address):
        for r in self.regions:
            if r['base']<=address<r['end']:return r
        raise ReaderError('Unmapped synthetic region')

    def put(self,address,value):
        r=self.region(address);struct.pack_into('<I',self.data[r['base']],address-r['base'],value)

    def read(self,address,size):
        r=self.region(address)
        if address+size>r['end'] or r['protect']&0x101:raise ReaderError('Unavailable synthetic memory')
        return bytes(self.data[r['base']][address-r['base']:address-r['base']+size])

    def report(self,**kw):
        return inspect(self.read,self.region,self.game.base,self.game.size,self.modules,**kw)


class GraphicsProbeTests(unittest.TestCase):
    def test_indirect_device_beyond_native_prefix_and_factory_are_reported_read_only(self):
        m=GraphicsMemory();m.put(m.game.base+0x3150,0)
        before={k:bytes(v) for k,v in m.data.items()}
        report=m.report()
        self.assertEqual(report['mode'],'READ ONLY');self.assertFalse(report['truncated'])
        self.assertEqual(report['factory_pattern_matches'],1)
        samples=report['possible_device_tables']
        self.assertEqual(len(samples),1)
        self.assertEqual(samples[0]['object_address'],hex(m.object))
        self.assertEqual(samples[0]['source']['kind'],'referenced_structure')
        self.assertEqual(samples[0]['source']['offset'],'0x400')
        self.assertEqual(samples[0]['method_slots']['end_scene']['module'],'d3d9.dll')
        self.assertEqual({k:bytes(v) for k,v in m.data.items()},before)

    def test_private_table_and_foreign_module_method_are_diagnostic_hints(self):
        m=GraphicsMemory(private_table=True,outside_method=True)
        sample=m.report()['possible_device_tables'][0]
        self.assertIsNone(sample['table_location']['module'])
        self.assertEqual(sample['method_slots']['end_scene']['module'],'dxgi.dll')
        self.assertEqual(sample['methods_outside_table_module'],119)

    def test_guarded_structure_unknown_pointer_and_nonexecuting_table_rejected(self):
        m=GraphicsMemory();m.put(m.game.base+0x3150,0)
        m.region(m.root)['protect']=0x104
        self.assertEqual(m.report()['possible_device_tables'],[])
        m=GraphicsMemory();m.put(m.object,0x99999998)
        self.assertEqual(m.report()['possible_device_tables'],[])
        m=GraphicsMemory();m.region(0x2002000)['protect']=4
        self.assertEqual(m.report()['possible_device_tables'],[])

    def test_limits_do_not_claim_complete_search(self):
        m=GraphicsMemory();m.put(m.game.base+0x3150,0)
        self.assertTrue(m.report(root_limit=0)['truncated'])
        audit=Audit(m.read,m.region,m.modules,seconds=0)
        audit.scan(m.game.base+0x3000,4096,dict(kind='test'),True)
        self.assertTrue(audit.truncated)

        audit=Audit(m.read,m.region,m.modules,byte_limit=1)
        audit.scan(m.game.base+0x3000,4096,dict(kind='test'),True)
        self.assertTrue(audit.truncated)

    def test_main_saves_report_and_closes_read_handles_on_success_or_failure(self):
        for fail in (False,True):
            m=GraphicsMemory();process=Mock(base=m.game.base,size=m.game.size,read=m.read)
            with tempfile.TemporaryDirectory() as directory,contextlib.ExitStack() as stack:
                stack.enter_context(patch('sys.argv',['probe']))
                stack.enter_context(patch.object(re4_graphics_probe,'__file__',str(Path(directory)/'tools/probe.py')))
                stack.enter_context(patch.object(re4_graphics_probe,'find_pid',return_value=42))
                stack.enter_context(patch.object(re4_graphics_probe,'WindowsProcess',return_value=process))
                stack.enter_context(patch.object(re4_graphics_probe,'loaded_modules',return_value=m.modules))
                stack.enter_context(patch.object(re4_graphics_probe,'query_memory',return_value=(123,m.region)))
                if fail:stack.enter_context(patch.object(re4_graphics_probe,'inspect',side_effect=ReaderError('scene changed')))
                stack.enter_context(contextlib.redirect_stdout(io.StringIO()))
                self.assertEqual(re4_graphics_probe.main(),1 if fail else 0)
                process.k.CloseHandle.assert_called_once_with(123);process.close.assert_called_once()
                reports=list(Path(directory).glob('graphics-probe-*.json'))
                self.assertEqual(len(reports),0 if fail else 1)
                if reports:self.assertEqual(json.loads(reports[0].read_text())['mode'],'READ ONLY')
