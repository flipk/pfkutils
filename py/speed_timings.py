import time

class speed_timing:
    _timings: list
    _descriptions: list
    _enabled: bool

    def __init__(self, enabled=True):
        self._timings = []
        self._descriptions = []
        self._enabled = enabled
        self.mark("startup")
        print("speed_timing CONSTRUCT")

    def __del__(self):
        print("speed_timing DESTRUCT")

    def mark(self,desc):
        if (self._enabled):
            self._timings.append(time.monotonic())
            self._descriptions.append(desc)

    def print(self):
        if (self._enabled):
            print("speed_timing TIMINGS started at %f" % (self._timings[0]))
            for ind in range(1,len(self._timings)):
                td = (self._timings[ind] - self._timings[ind-1]) * 1000
                print("%9.1f ms : %s" % (td,self._descriptions[ind]))
            print("speed_timing TIMINGS finished at %f" % (self._timings[ind]))

               
                
timings = speed_timing()
time.sleep(0.2)
timings.mark("one")
time.sleep(0.2)
timings.mark("two")
timings.print()
