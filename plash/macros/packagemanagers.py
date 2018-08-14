from plash.eval import eval, register_macro


@register_macro()
def defpm(name, *lines):
    'define a new package manager'

    @register_macro(name, group='package managers')
    def package_manager(*packages):
        if not packages:
            return
        sh_packages = ' '.join(pkg for pkg in packages)
        expanded_lines = [line.format(sh_packages) for line in lines]
        return eval([['run'] + expanded_lines])

    package_manager.__doc__ = "install packages with {}".format(name)



eval([[
    'defpm',
    'apt',
    'apt-get update',
    'apt-get install -y {}',
], [
    'defpm',
    'add-apt-repository',
    'apt-get install software-properties-common',
    'run add-apt-repository -y {}',
], [
    'defpm',
    'apk',
    'apk update',
    'apk add {}',
], [
    'defpm',
    'yum',
    'yum install -y {}',
], [
    'defpm',
    'dnf',
    'dnf install -y {}',
], [
    'defpm',
    'pip',
    'pip install {}',
], [
    'defpm',
    'pip3',
    'pip3 install {}',
], [
    'defpm',
    'npm',
    'npm install -g {}',
], [
    'defpm',
    'pacman',
    'pacman -Sy --noconfirm {}',
], [
    'defpm',
    'emerge',
    'emerge {}',
]])
