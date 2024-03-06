import time

# timings = SpeedTiming('test')
# time.sleep(0.2)
# timings.mark("one")
# time.sleep(0.2)
# timings.mark("two")
# timings.print()


class SpeedTiming:
    _timings: list
    _descriptions: list
    _enabled: bool
    _name: str

    def __init__(self, name: str, enabled: bool = True):
        self._timings = []
        self._descriptions = []
        self._enabled = enabled
        self._name = name
        self.mark("startup")

    def mark(self, desc):
        if self._enabled:
            self._timings.append(time.monotonic())
            self._descriptions.append(desc)

    def print(self):
        if self._enabled:
            print(f'SpeedTiming {self._name} started at {self._timings[0]}')
            ind: int = 0
            for ind in range(1, len(self._timings)):
                td = (self._timings[ind] - self._timings[ind-1]) * 1000
                print("%9.1f ms : %s" % (td, self._descriptions[ind]))
            print(f'SpeedTiming {self._name} finished at {self._timings[ind]}')
