from plashlib.eval import register_macro


@register_macro()
def workaround_unionfs():
    'workaround apt so it works despite this issue: https://github.com/rpodgorny/unionfs-fuse/issues/78'
    return '''if [ -d /etc/apt/apt.conf.d ]; then
echo 'Dir::Log::Terminal "/dev/null";' > /etc/apt/apt.conf.d/unionfs_workaround
echo 'APT::Sandbox::User "root";' >> /etc/apt/apt.conf.d/unionfs_workarounds
: See https://github.com/rpodgorny/unionfs-fuse/issues/78
chown root:root /var/cache/apt/archives/partial || true
fi'''
