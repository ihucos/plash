from plash.eval import register_macro


@register_macro()
def workaround_unionfs():
    return '''if [ -d /etc/apt/apt.conf.d ]; then
echo 'Dir::Log::Terminal "/dev/null";' > /etc/apt/apt.conf.d/unionfs_workaround
echo 'APT::Sandbox::User "root";' >> /etc/apt/apt.conf.d/unionfs_workarounds
: See https://github.com/rpodgorny/unionfs-fuse/issues/78
chown root:root /var/cache/apt/archives/partial || true
fi'''


@register_macro()
def from_docker(image):
    import subprocess
    container = subprocess.check_output(['docker', 'create', 'image', 'sh']).decode().rstrip('\n')
    docker_export = subprocess.Popen(['docker', 'export', container], stdout=subprocess.PIPE)
    atexit.register(lambda: docker_export.kill())
    subprocess.check_call(['plash', 'import-tar'], stdin=docker_export.stdout)
