from .baseaction import Action
from .eval import register_action


# ========== OTHER FILE ==========

@register_action('pdb')
def pdb():
    import pdb
    pdb.set_trace()

@register_action('layer')
def layer():
    return "echo 'MY LAYER MARKER asdjfalkdf8a9df7yg'"

# class Include(Action):
#     def handle_arg(self, file):
#         fname = os.path.realpath(fname)
#         with open(fname) as f:
#             config = f.read()
#         loaded = yaml.load(config)
#         return self.eval(loaded)


class LayeEach(Action):
    name = 'with-layers'

    def __call__(self, command, *args):
        lst = []
        for arg in args:
            lst.append([command, arg])
            lst.append(['layer'])
        return self.eval(lst)

class Inline(Action):
    name = 'inline'
    def handle_arg(self, arg):
        return arg

# class Home(Action):

#     def __call__(self):
#         return self.eval([['include', path.join(home_path, '.plash.yaml')]])


# print(eval([['layer-each', 'inline', 'hi', 'ho']]))
