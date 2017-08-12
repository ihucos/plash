#!/usr/bin/env python3

import subprocess
from tempfile import mkdtemp

p = subprocess.Popen(["wget", "--spider", "--reject-regex", r"\?", "--force-html", "-r", "https://uk.images.linuxcontainers.org/images/"], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
names = {}

ROOTFS_TEMPL = "https://images.linuxcontainers.org/images/{}/{}/{}/{}/{}/rootfs.tar.xz"

for line in p.stderr:
      line = line.decode()
      if line.startswith('--'):
            link = line.split()[-1].rstrip('\n')
            try:
                  _, path = link.split('/images/')
            except ValueError:
                  continue
            path_parts = path.rstrip('/').split('/')
            if len(path_parts) == 5:
                  distro, version, platform, dummy_folder, upload_version = path_parts
                  if platform != 'amd64':  # only support this right now
                        continue
                  if version == 'current':
                        names[distro] = ROOTFS_TEMPL.format(*path_parts)
                  elif version[0].isalpha():
                        # path parts with older upload_version also come later
                        # (ignore possibel alphanumeric sort for dates on this right now)
                        names[version] = ROOTFS_TEMPL.format(*path_parts)
                  else:
                        names['{}{}'.format(distro, version)] = ROOTFS_TEMPL.format(*path_parts)

from pprint import pprint
pprint(names)
