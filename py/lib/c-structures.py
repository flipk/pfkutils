
# see https://pypi.org/project/cstruct/

import cstruct

class Position(cstruct.MemCStruct):
#   __def__ = open("path/to/header.h").read()
    __def__ = """
        struct {
            unsigned char head;
            unsigned char sector;
            unsigned char cyl;
            unsigned int cyls;
        }
    """
    @property
    def lba(self):
        return (self.cyl * 16 + self.head) * 63 + (self.sector - 1)

pos = Position(cyl=7, head=2, sector=15)
print(f'head: {pos.head} sector: {pos.sector} cyl: {pos.cyl} lba: {pos.lba}')
print(f'packed: {pos.pack()}')
