#!/usr/bin/env python3
"""Offline release-doc regressions; no GitHub calls or package publication."""

import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).with_name('update_release_docs.py')
spec = importlib.util.spec_from_file_location('release_docs', SCRIPT)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ReleaseDocs(unittest.TestCase):
    def test_selective_references(self):
        source = '''utxoz/1.0.0 fmt/12.1.0 boost/1.91.0
./scripts/build-create.sh 1.0.0 spdlog
./scripts/build-create.sh 0.0.0-dev custom
utxoz/1.0.0-beta utxoz/1.0.0+build utxoz/1.0.0.1
git clone --branch v1.0.0 https://github.com/utxo-z/utxo-z.git
git clone --branch v1.0.0 https://github.com/other/project.git
https://github.com/utxo-z/utxo-z/releases/tag/v1.0.0
https://github.com/utxo-z/utxo-z/tree/v1.0.0/examples
geometry 4; before 1.0.0; v0.10.0 needs a rebuild
'''
        expected = source.replace('utxoz/1.0.0 ', 'utxoz/1.0.1 ').replace(
            'build-create.sh 1.0.0 ', 'build-create.sh 1.0.1 ').replace(
            '--branch v1.0.0 https://github.com/utxo-z', '--branch v1.0.1 https://github.com/utxo-z').replace(
            '/tag/v1.0.0', '/tag/v1.0.1').replace('/tree/v1.0.0/', '/tree/v1.0.1/')
        self.assertEqual(module.update(source, '1.0.1'), expected)
        self.assertEqual(module.update(expected, '1.0.1'), expected)

    def test_git_scope_check_stage_and_idempotence(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            subprocess.run(['git', 'init', '-q', root], check=True)
            files = {
                'README.md': 'utxoz/1.0.0\r\n',
                'examples/with space.md': './scripts/build-create.sh 1.0.0\n',
                'doc/release-notes/old.md': 'utxoz/1.0.0\n',
                'docs/CHANGELOG.md': 'utxoz/1.0.0\n',
                'conanfile.py': 'utxoz/1.0.0\n',
            }
            for name, content in files.items():
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(content.encode())
            subprocess.run(['git', '-C', root, 'add', '.'], check=True)
            subprocess.run(['git', '-C', root, '-c', 'user.name=test', '-c', 'user.email=t@example.com',
                            'commit', '-qm', 'fixture'], check=True)
            (root / 'docs/untracked.md').write_text('utxoz/1.0.0')
            command = [sys.executable, str(SCRIPT), '1.0.1', '--root', str(root)]
            stale = subprocess.run(command + ['--check'], capture_output=True)
            self.assertEqual(stale.returncode, 1)
            self.assertEqual((root / 'README.md').read_bytes(), b'utxoz/1.0.0\r\n')
            subprocess.run(command + ['--stage'], check=True, capture_output=True)
            self.assertEqual((root / 'README.md').read_bytes(), b'utxoz/1.0.1\r\n')
            staged = subprocess.check_output(['git', '-C', root, 'diff', '--cached', '--name-only']).decode().splitlines()
            self.assertEqual(staged, ['README.md', 'examples/with space.md'])
            for name in ['doc/release-notes/old.md', 'docs/CHANGELOG.md', 'conanfile.py']:
                self.assertEqual((root / name).read_text(), files[name])
            self.assertEqual((root / 'docs/untracked.md').read_text(), 'utxoz/1.0.0')
            subprocess.run(command + ['--check'], check=True, capture_output=True)
            self.assertEqual(subprocess.check_output(command), b'')
            before = (root / 'README.md').read_bytes()
            invalid = subprocess.run([sys.executable, str(SCRIPT), 'bad-version', '--root', str(root)], capture_output=True)
            self.assertNotEqual(invalid.returncode, 0)
            self.assertEqual((root / 'README.md').read_bytes(), before)

    def test_release_order(self):
        release = SCRIPT.with_name('release.sh').read_text()
        branch = release.index('git checkout -b "release/${VERSION}"')
        update = release.index('python3 scripts/update_release_docs.py "${VERSION}" --stage')
        commit = release.index('git commit --allow-empty')
        push = release.index('git push origin "release/${VERSION}"', commit)
        self.assertLess(branch, update)
        self.assertLess(update, commit)
        self.assertLess(commit, push)

    def test_missing_package_reference_refuses_before_writing(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            subprocess.run(['git', 'init', '-q', root], check=True)
            (root / 'README.md').write_text('./scripts/build-create.sh 1.0.0\n')
            subprocess.run(['git', '-C', root, 'add', 'README.md'], check=True)
            result = subprocess.run([sys.executable, str(SCRIPT), '1.0.1', '--root', str(root)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((root / 'README.md').read_text(), './scripts/build-create.sh 1.0.0\n')

    def test_linked_documentation_is_refused(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            subprocess.run(['git', 'init', '-q', root], check=True)
            (root / 'README.md').write_text('utxoz/1.0.0\n')
            (root / 'docs').mkdir()
            (root / 'docs/link.md').symlink_to(root / 'README.md')
            subprocess.run(['git', '-C', root, 'add', '.'], check=True)
            result = subprocess.run([sys.executable, str(SCRIPT), '1.0.1', '--root', str(root)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual((root / 'README.md').read_text(), 'utxoz/1.0.0\n')


if __name__ == '__main__':
    unittest.main()
