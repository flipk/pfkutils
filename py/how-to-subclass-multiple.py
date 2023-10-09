# two ways to initialize parent classes in multiple derivation.

# the first way, the child class calls __init__ on both parents
# one at a time, passing args along as required.

class Producer:
    pv: int
    def __init__(self, producer_arg):
        print(f'initializing Producer (arg={producer_arg})')
        self.pv = producer_arg

class Consumer:
    cv: int
    def __init__(self, consumer_arg):
        print(f'initializing Consumer (arg={consumer_arg})')
        self.cv = consumer_arg

class Thingy(Producer, Consumer):
    def __init__(self, arg1, arg2):
        print(f'initializing Thingy({arg1},{arg2})')
        Producer.__init__(self, arg1)
        Consumer.__init__(self, arg2)
    def doit(self):
        print(f'pv = {self.pv}   cv = {self.cv}')

x = Thingy(1,2)
x.doit()

# the second method, we use __mro__ (which i don't understand)
# and use super().__init__ which chains from one to the other;
# the only tway this one works though is if there is agreement
# amongst all three classes that **kwargs will carry unique parameter
# names for each class, because super().__init__ only works on
# passing **kwargs. (i personally don't like this one as much)

class Producer:
    pv: int
    def __init__(self, **kwargs):
        producer_arg = kwargs['producer_arg']
        del kwargs['producer_arg']
        print(f'initializing Producer (arg={producer_arg} kw={kwargs})')
        self.pv = producer_arg
        super().__init__(**kwargs)

class Consumer:
    cv: int
    def __init__(self, **kwargs):
        consumer_arg = kwargs['consumer_arg']
        del kwargs['consumer_arg']
        print(f'initializing Consumer (arg={consumer_arg} kw={kwargs})')
        self.cv = consumer_arg
        super().__init__(**kwargs)

class Thingy(Producer, Consumer):
    def __init__(self, **kwargs):
        print(f'initializing Thingy({kwargs})')
        super().__init__(**kwargs)
    def doit(self):
        print(f'pv = {self.pv}   cv = {self.cv}')

x = Thingy(producer_arg=1, consumer_arg=2)
x.doit()
