
class thingy(some other classname):
    def __init__(self, args):
        super( <something?> ).__init__(stuff ? )


what is __call__ ?

The __call__ method enables Python programmers to write classes
where the instances behave like functions and can be called like
a function. When the instance is called as a function; if this
method is defined, x(arg1, arg2, ...) is a shorthand
for x.__call__(arg1, arg2, ...).

class Example:
    def __init__(self, init_args):
        print('instance created')

    def __call__(self, call_args):
        print('instance is being called')

# instance constructed
e = Example(init_args)

# call the __call__ method
e(call_args)
