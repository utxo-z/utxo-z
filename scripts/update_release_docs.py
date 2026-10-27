#!/usr/bin/env python3
"""Update actionable UTXO-Z release references, not historical version prose."""

import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile


VERSION = r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)"
STABLE = VERSION + r"(?![\w.+-])"
PATTERNS = (
    re.compile(r"(?P<prefix>\butxoz/)" + STABLE),
    re.compile(r"(?P<prefix>(?:\./)?scripts/build-create\.sh[ \t]+)" + STABLE),
    re.compile(r"(?P<prefix>https://github\.com/utxo-z/utxo-z/(?:releases/tag/v|tree/v))" + STABLE),
)
CLONE_TAG = re.compile(r"(?P<prefix>--branch[ \t]+v)" + STABLE)
BEGIN = '<!-- utxoz:current-release:start -->'
END = '<!-- utxoz:current-release:end -->'


def git_env():
    """Use the repository selected by cwd, never an inherited index or worktree."""
    return {key: value for key, value in os.environ.items() if not key.startswith('GIT_')}


def marked_lines(text):
    """Identify opt-in current-release blocks; reject malformed marker pairs."""
    active = False
    lines = []
    for line in text.splitlines(keepends=True):
        marker = line.strip()
        if marker == BEGIN:
            if active:
                raise ValueError('Nested current-release markers')
            active = True
            lines.append((False, line))
        elif marker == END:
            if not active:
                raise ValueError('Unmatched current-release end marker')
            active = False
            lines.append((False, line))
        else:
            lines.append((active, line))
    if active:
        raise ValueError('Unclosed current-release marker')
    return lines


def update(text, version):
    """Update only explicitly marked current examples, preserving history verbatim."""
    output = []
    for current, line in marked_lines(text):
        if current:
            for pattern in PATTERNS:
                line = pattern.sub(lambda match: match['prefix'] + version, line)
            if 'git clone ' in line and 'https://github.com/utxo-z/utxo-z.git' in line:
                line = CLONE_TAG.sub(lambda match: match['prefix'] + version, line)
        output.append(line)
    return ''.join(output)


def candidates(root):
    tracked = subprocess.check_output(['git', 'ls-files', '-z'], cwd=root, env=git_env())
    for name in tracked.decode().split('\0'):
        path = Path(name)
        if not name or path.suffix.lower() not in {'.md', '.rst'}:
            continue
        if name != 'README.md' and path.parts[0] not in {'doc', 'docs', 'examples'}:
            continue
        if any('release-notes' in part.lower() or 'changelog' in part.lower()
               or part.lower() in {'fixtures', 'releases'} for part in path.parts):
            continue
        if (root / path).resolve() != root / path:
            raise ValueError(f'Refusing linked documentation path: {name}')
        yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('version')
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parent.parent)
    parser.add_argument('--check', action='store_true', help='Refuse stale references without writing')
    parser.add_argument('--stage', action='store_true', help='Stage only changed documentation paths')
    args = parser.parse_args()
    if not re.fullmatch(VERSION, args.version):
        parser.error('Expected a stable version such as 1.0.1')
    if args.check and args.stage:
        parser.error('--check and --stage cannot be combined')
    root = args.root.resolve()
    changes = []
    for path in candidates(root):
        old = (root / path).read_bytes()
        new = update(old.decode('utf-8'), args.version).encode('utf-8')
        if old != new:
            changes.append((path, new))
    readme = (root / 'README.md').read_text(encoding='utf-8')
    if not any(current and PATTERNS[0].search(line) for current, line in marked_lines(readme)):
        raise ValueError('README.md has no marked stable utxoz/package reference; update the release-doc rules')
    if args.check:
        for path, _ in changes:
            print(f'Stale release reference: {path}')
        return int(bool(changes))
    for path, content in changes:
        target = root / path
        descriptor, temporary = tempfile.mkstemp(dir=target.parent)
        try:
            with os.fdopen(descriptor, 'wb') as stream:
                stream.write(content)
            os.chmod(temporary, target.stat().st_mode & 0o777)
            os.replace(temporary, target)
        finally:
            if os.path.exists(temporary):
                os.unlink(temporary)
        print(path)
    if args.stage and changes:
        subprocess.run(['git', 'add', '--', *(str(path) for path, _ in changes)], cwd=root, env=git_env(), check=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
