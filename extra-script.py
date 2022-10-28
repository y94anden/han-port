from subprocess import run
from pathlib import Path

import sys
Import('env')

embedded_dir = Path(env['PROJECT_INCLUDE_DIR']) / 'embedded-filesystem'
filesystem_dir = Path(env['PROJECT_DIR']) / 'filesystem'

command = ['make',
           '-C',
           f'{embedded_dir}',
           f'FOLDER={filesystem_dir}',
           'build/files.c',
]

run(command)


