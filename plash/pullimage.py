import atexit
import errno
import hashlib
import os
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
from os import path
from os.path import abspath, join
from tempfile import mkdtemp
from urllib.request import urlopen

class ImageDoesNotExist(Exception):
    pass

class BaseImageCreator:
    def __call__(self, image):

        self.arg = image
        image_id = self.get_id()
        image_dir = join(BUILDS_DIR, image_id)

        if not os.path.exists(image_dir):

            # create the basic directory structure
            try:
                os.mkdir(BASE_DIR, 0o0700)
            except FileExistsError:
                pass
            for dir in [BASE_DIR, TMP_DIR, LINKS_DIR]:
                try:
                    os.mkdir(dir)
                except FileExistsError:
                    pass

            tmp_image_dir = mkdtemp(dir=TMP_DIR) # must be on same fs than BASE_DIR for rename to work
            os.mkdir(join(tmp_image_dir, 'children'))
            os.mkdir(join(tmp_image_dir, 'payload'))
            rootfs = join(tmp_image_dir, 'payload')
            self.prepare_image(rootfs)

            # we want /etc/resolv to not by a symlink or not to not exist - that makes the later mount not work
            resolvconf = join(rootfs, 'etc/resolv.conf')
            try:
                os.unlink(resolvconf)
            except FileNotFoundError:
                pass
            with open(resolvconf, 'w') as f:
                f.seek(0)
                f.truncate()

            Container([image_id]).register_alias()
            try:
                os.rename(tmp_image_dir, image_dir)
            except OSError as exc:
                if exc.errno == errno.ENOTEMPTY:
                    info('another process already pulled that image')
                else:
                    raise
        
        return image_dir


class LXCImageCreator(BaseImageCreator):
    def get_id(self):
        return self.arg # XXX: dot dot attack and so son, escape or so

    def prepare_image(self, outdir):
        info('Preparing image')
        images = self._index_lxc_images()
        try:
            image_url = images[self.arg]
        except KeyError:
            raise ImageDoesNotExist('No such image, available: {}'.format(
                ' '.join(sorted(images))))

        import tempfile
        _, download_file = tempfile.mkstemp(prefix=self.arg + '.', suffix='.tar.xz') # join(mkdtemp(), self.arg + '.tar.xz')
        run(['wget', '-q', '--show-progress', image_url, '-O', download_file])
        t = tarfile.open(download_file)
        t.extractall(outdir)

    def _index_lxc_images(self):
        content = urlopen('http://images.linuxcontainers.org/').read().decode() # FIXME: use https
        matches = re.findall('<tr><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td><td>(.+?)</td></tr>', content)

        names = {}
        for distro, version, arch, variant, date, _, _, _ in matches:
            if not variant == 'default':
                continue

            if arch != 'amd64':  # only support this right now
                continue

            url = LXC_URL_TEMPL.format(distro, version, arch, variant, date)

            if version == 'current':
                names[distro] = url

            elif version[0].isalpha():
                # path parts with older upload_version also come later
                # (ignore possibel alphanumeric sort for dates on this right now)
                names[version] = url
            else:
                names['{}{}'.format(distro, version)] = url
        return names

def prepare_image(base_name):
    return lxc_image_creator(base_name)
