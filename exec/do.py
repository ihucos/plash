

files = [
'import-lxc',
'with-mount',
'export-tar',
'create',
'import-tar',
'init',
'purge',
'import-url',
'sudo',
'clean',
'version',
'parent',
'add-layer',
'map',
'shrink',
'run',
'help',
'help-macros',
'runb',
'eval',
'rm',
'mount',
'copy',
'b',
'build',
'import-docker',
'eval-plashfile',
]


for file in files:
    name = file.replace('-', '_')

    l = f'int {name}_main(int argc, char *argv[]);\n'
    open(file + '.h', 'w').write(l)

    r = open(file + '.c', 'r').read()
    r = r.replace("main(int argc, char *argv[])", f'int {name}_main(int argc, char *argv[])')
    open(file + '.c', 'w').write(r)

