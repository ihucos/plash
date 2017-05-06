from virt import DockerBuildable

distros = []
class OSMeta(type):
    def __new__(cls, clsname, superclasses, attributedict):
        cls = type.__new__(cls, clsname, superclasses, attributedict)
        for sp in superclasses:
            distros.append(cls())
        return cls

class OS(DockerBuildable, metaclass=OSMeta):
    packages = None


    @property
    def short_name(self):
        return self.name[0].upper()

    @property
    def name(self):
        return self.__class__.__name__.lower()

    def get_build_commands(self):
        if not self.packages:
            return ''
        return str(self.packages)

    def get_base_image_name(self):
        return self.base_image
