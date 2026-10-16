"""The arrow hotfix must not require another native/host installation."""
import hashlib
import io
import json
from pathlib import Path
import unittest
import zipfile
VERSION='0.4.56-arrowfix1'
NATIVE_SHA256='5b25e60dd954c1b5e7480418f406cce8ecd43ee8775ada587da4ecae3ee88e6d'


class ArrowfixPackageTests(unittest.TestCase):
 def test_only_mod_and_instructions_differ_from_056(self):
  root=Path(__file__).resolve().parents[1];release=root/'releases'/VERSION
  with zipfile.ZipFile(root/'releases/0.4.56/re4bridge-0.4.56-windows.zip') as base, zipfile.ZipFile(release/f're4bridge-{VERSION}-windows.zip') as full, zipfile.ZipFile(release/f're4bridge-{VERSION}-mod.zip') as patch:
   old='mods/re4bridge-0.4.56.jar';new=f'mods/re4bridge-{VERSION}.jar'
   self.assertEqual(set(full.namelist()),(set(base.namelist())-{old})|{new})
   changed={name for name in base.namelist() if name!=old and base.read(name)!=full.read(name)}
   self.assertEqual(changed,{'README.md'})
   self.assertEqual(hashlib.sha256(full.read('native/dinput8.dll')).hexdigest(),NATIVE_SHA256)
   self.assertEqual(set(patch.namelist()),{new,'README-ARROWFIX1.md'})
   self.assertEqual(patch.read(new),full.read(new))
   self.assertEqual(patch.read('README-ARROWFIX1.md'),full.read('README.md'))
   with zipfile.ZipFile(io.BytesIO(full.read(new))) as mod, zipfile.ZipFile(io.BytesIO(base.read(old))) as original:
    metadata=json.loads(mod.read('fabric.mod.json'));previous=json.loads(original.read('fabric.mod.json'))
    self.assertEqual(metadata['version'],VERSION)
    self.assertEqual(metadata['depends'],previous['depends'])
    nested=metadata['jars'][0]['file'];self.assertEqual(mod.read(nested),original.read(previous['jars'][0]['file']))
    self.assertIn('dev/dezva/re4bridge/minecraft/ProjectileTrace.class',mod.namelist())
    self.assertFalse(any(n.startswith('net/minecraft/') for n in mod.namelist()))
  for line in (release/'SHA256SUMS.txt').read_text().splitlines():
   digest,name=line.split();self.assertEqual(hashlib.sha256((release/name).read_bytes()).hexdigest(),digest)
