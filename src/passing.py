
def fun(a):
    a=5

a=3
fun(a)
print a




class callByRef:
    def __init__(self, **args):
        for val in args.items():
            setattr(self, 'val', value)

def func4(args):
    args.a = 'new-value'        # args is a mutable callByRef
    args.b = args.b + 1         # change object in-place

args = callByRef(a='old-value')
func4(args)
print args
