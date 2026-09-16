"""Compile and exercise the actual C++ control logic without a connected PLC."""
import ctypes
import os
from pathlib import Path
import re
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
build = root / '.pio' / 'host-tests'
build.mkdir(parents=True, exist_ok=True)
compiler = shutil.which('clang++') or shutil.which('g++')
if not compiler:
    raise SystemExit('Install LLVM (clang++) or GCC (g++) to run the host tests.')
library = build / ('control_tests.dll' if os.name == 'nt' else 'control_tests.so')
command = [compiler, '-std=c++11', '-O2', '-Wall', '-Wextra', '-Werror',
           '-shared', '-I' + str(root / 'lib/Control'), '-I' + str(root / 'include'),
           '-I' + str(root),
           str(root / 'test/control_tests.cpp'), '-o', str(library)]
if os.name == 'nt' and 'clang' in Path(compiler).name:
    # No CRT/Visual Studio dependency: tests only use fixed-width integer arithmetic.
    command += ['-ffreestanding', '-fno-stack-protector', '-nostdlib',
                '-fuse-ld=lld', '-Wl,/noentry']
elif os.name != 'nt':
    command += ['-fPIC']
subprocess.run(command, check=True)
tests = ctypes.CDLL(str(library))
names = re.findall(r'^TEST (\w+)\(', (root / 'test/control_tests.cpp').read_text(), re.M)
failed = False
for name in names:
    line = getattr(tests, name)()
    print(f'{name}: ' + (f'FAIL at line {line}' if line else 'PASS'))
    failed |= bool(line)
raise SystemExit(1 if failed else 0)
