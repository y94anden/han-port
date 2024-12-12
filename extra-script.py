from datetime import datetime
from subprocess import run, PIPE
from pathlib import Path

import sys
Import('env')

project_dir = Path(env['PROJECT_DIR'])
embedded_dir = Path(env['PROJECT_INCLUDE_DIR']) / 'embedded-filesystem'
filesystem_dir = project_dir / 'filesystem'
about_template = project_dir / 'about.html.template'
about_rendered = filesystem_dir / 'about.html'

compile_time = datetime.now().isoformat(' ')[:19]
git_commit = run('git describe --dirty --always --exclude "*"'.split(), stdout=PIPE).stdout.decode().strip()
git_remote = run('git config --get remote.origin.url'.split(), stdout=PIPE).stdout.decode().strip()

about = about_template.read_text().format(compile_time=compile_time, git_commit=git_commit, git_remote=git_remote)
about_rendered.write_text(about)

command = ['make',
           '-C',
           f'{embedded_dir}',
           f'FOLDER={filesystem_dir}',
           'build/files.c',
]

run(command)


