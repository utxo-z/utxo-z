#!/usr/bin/env python3
"""Check the shared Emscripten profile and local/CI toolchain wiring offline."""

import configparser
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest

from test_release_authority import parse_jobs

ROOT = Path(__file__).resolve().parent.parent


def recipe_checkout_ref(lane):
    """Read ref only from the SDK recipe checkout step, not unrelated job text."""
    step = re.search(r'(?ms)^      - name: 📥 Emscripten recipe source\n(.*?)(?=^      - |\Z)', lane)
    if step is None:
        raise ValueError('Emscripten recipe checkout step is absent')
    refs = re.findall(r'^          ref: ([^\n]+)$', step.group(1), re.MULTILINE)
    if len(refs) != 1:
        raise ValueError('Recipe checkout must have exactly one ref')
    return refs[0].strip()


class EmsdkProfile(unittest.TestCase):
    def test_profile_and_ci_use_one_sdk(self):
        profile = configparser.ConfigParser(allow_no_value=True, delimiters=('=',), interpolation=None)
        profile.read(ROOT / 'ci/conan-ems6')
        self.assertEqual(profile['settings']['compiler'], 'emcc')
        self.assertEqual(profile['settings']['compiler.version'], '6.0.13')
        self.assertEqual(profile['settings']['arch'], 'wasm')
        self.assertEqual(profile['settings']['compiler.cppstd'], '23')
        self.assertIn('emsdk/6.0.13', profile['tool_requires'])
        self.assertEqual(profile['conf']['tools.cmake.cmaketoolchain:generator'], 'Ninja')
        jobs = parse_jobs((ROOT / '.github/workflows/ci.yml').read_text())
        lane = jobs['build-wasm']['text']
        self.assertIn('repository: conan-io/conan-toolchains', lane)
        pin = '11b83330cdeb20622fe4b097dccb254969639a54'
        self.assertEqual(recipe_checkout_ref(lane), pin)
        mutable = lane.replace('ref: ' + pin, 'ref: main', 1) + '\n        # ref: ' + pin
        self.assertIn('ref: ' + pin, mutable)
        self.assertNotEqual(recipe_checkout_ref(mutable), pin)
        self.assertIn('-pr:h ./ci/conan-ems6 -pr:b default', lane)
        self.assertIn('CONAN_PROFILE: ./ci/conan-ems6', lane)
        self.assertEqual(lane.count('source build/build/Release/generators/conanbuild.sh'), 3)
        self.assertNotIn('setup-emsdk@', lane)
        self.assertNotIn('ems2', lane)

    def test_local_script_activates_its_own_environment(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / 'scripts').mkdir()
            (root / 'ci').mkdir()
            (root / 'bin').mkdir()
            shutil.copyfile(ROOT / 'scripts/build-wasm.sh', root / 'scripts/build-wasm.sh')
            shutil.copyfile(ROOT / 'ci/conan-ems6', root / 'ci/conan-ems6')
            native_presets = root / 'CMakeUserPresets.json'
            original_presets = b'{"version":4,"include":["native/CMakePresets.json"]}\n'
            native_presets.write_bytes(original_presets)
            fake = '''#!/usr/bin/env python3
import json, os, pathlib, sys
args = sys.argv[1:]
tool = pathlib.Path(sys.argv[0]).name
with open(os.environ['CALLS'], 'a') as out:
    out.write(json.dumps([tool, args]) + '\\n')
if tool == 'conan' and args[0] == 'install':
    if 'tools.cmake.cmaketoolchain:user_presets=' not in args:
        pathlib.Path('CMakeUserPresets.json').write_text('duplicate WASM preset added')
    path = pathlib.Path(args[args.index('-of') + 1]) / 'build/Release/generators'
    path.mkdir(parents=True)
    (path / 'conanbuild.sh').write_text('export TEST_SDK_ENV=ready\\n')
    (path / 'conan_toolchain.cmake').write_text('# fixture\\n')
if tool == 'cmake' and os.environ.get('TEST_SDK_ENV') != 'ready':
    sys.exit(9)
'''
            for tool in ['conan', 'cmake']:
                command = root / 'bin' / tool
                command.write_text(fake)
                command.chmod(0o755)
            calls = root / 'calls'
            env = dict(os.environ, PATH=str(root / 'bin') + os.pathsep + os.environ['PATH'], CALLS=str(calls))
            env.pop('TEST_SDK_ENV', None)
            subprocess.run(['bash', str(root / 'scripts/build-wasm.sh')], env=env, check=True,
                           capture_output=True, text=True)
            commands = [json.loads(line) for line in calls.read_text().splitlines()]
            self.assertEqual(native_presets.read_bytes(), original_presets)
            self.assertEqual([tool for tool, _ in commands], ['conan', 'conan', 'cmake', 'cmake'])
            for _, args in commands[:2]:
                self.assertEqual(args[args.index('-pr:h') + 1], str(root / 'ci/conan-ems6'))
                self.assertEqual(args[args.index('-pr:b') + 1], 'default')
            configure = commands[2][1]
            self.assertIn('tools.cmake.cmaketoolchain:user_presets=', commands[1][1])
            self.assertEqual(configure[configure.index('-G') + 1], 'Ninja')
            build = root / 'build-wasm/build/Release'
            self.assertEqual(configure[configure.index('-B') + 1], str(build))
            self.assertIn('-DCMAKE_TOOLCHAIN_FILE=' + str(build / 'generators/conan_toolchain.cmake'), configure)
            self.assertEqual(commands[3][1][:2], ['--build', str(build)])


if __name__ == '__main__':
    unittest.main()
