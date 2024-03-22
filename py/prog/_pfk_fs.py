
class Widths:
    num: int
    name: int
    depends: int
    uuid: int
    luks: int
    passgrp: int
    open: int
    checked: int
    nfs: int
    luksnfs: int
    mntpt: int
    mounted: int
    tickle: int

    def __init__(self):
        self.num = 2  # fixed
        self.name = 4
        self.depends = 0
        self.uuid = 0
        self.luks = 0  # not used for cols (luksfs instead)
        self.passgrp = 0
        self.open = 0
        self.checked = 0
        self.nfs = 0  # not used for cols (luksfs instead)
        self.luksnfs = 0
        self.mntpt = 5
        self.mounted = 4  # fixed
        self.tickle = 4  # fixed

    def max(self, other):
        if other.name > self.name:
            self.name = other.name
        if other.depends > self.depends:
            self.depends = other.depends
        if other.uuid > self.uuid:
            self.uuid = other.uuid
        if other.luks > self.luks:
            self.luks = other.luks
        if other.passgrp > self.passgrp:
            self.passgrp = other.passgrp
        if other.open > self.open:
            self.open = other.open
        if other.checked > self.checked:
            self.checked = other.checked
        if other.nfs > self.nfs:
            self.nfs = other.nfs
        if other.mntpt > self.mntpt:
            self.mntpt = other.mntpt
        if self.luks > self.luksnfs:
            self.luksnfs = self.luks
        if self.nfs > self.luksnfs:
            self.luksnfs = self.nfs

    def calc_cols(self, other):
        pos = 0
        self.num = pos
        pos += other.num + 1
        self.name = pos
        pos += other.name + 1
        self.depends = pos
        if other.depends > 0:
            pos += other.depends + 1
        self.uuid = 0  # uuid is not printed
        self.luksnfs = pos
        if other.luksnfs > 0:
            pos += other.luksnfs + 1
        self.passgrp = pos
        if other.passgrp > 0:
            pos += other.passgrp + 1
        self.open = pos
        if other.open > 0:
            pos += other.open + 1
        self.checked = pos
        if other.checked > 0:
            pos += other.checked + 1
        self.mntpt = pos
        if other.mntpt > 0:
            pos += other.mntpt + 1
        self.mounted = pos
        pos += other.mounted + 1
        self.tickle = pos
        # pos += other.tickle + 1

    def __str__(self):
        return f'num:{self.num} name:{self.name} dep:{self.depends} ' + \
               f'uuid:{self.uuid} luks:{self.luks} passgrp:{self.passgrp} ' + \
               f'open:{self.open} checked:{self.checked} nfs:{self.nfs} ' + \
               f'mntpt:{self.mntpt} mounted:{self.mounted}'


class Fs:
    name: str
    mntpt: str
    UUID: str | None
    imgfile: str | None
    nfs: str | None
    depends: str | None
    luks: str | None
    open: bool
    checked: bool
    mounted: bool
    passgrp: int | None
    tickle: bool
    open_output: str
    fsck_output: str
    mount_output: str
    widths: Widths

    def __init__(self, name: str,
                 mntpt: str,
                 uuid: str | None = None,
                 imgfile: str | None = None,
                 nfs: str | None = None,
                 depends: str | None = None,
                 luks: str | None = None,
                 passgrp: int | None = None,
                 tickle: bool = False
                 ):
        count = 0
        count += 1 if uuid else 0
        count += 1 if imgfile else 0
        count += 1 if nfs else 0
        if count != 1:
            print(f'ERROR: must specify only one of [uuid, imgfile, nfs]')
            return
        self.widths = Widths()
        self.widths.name = len(name) if name else 0
        self.name = name
        self.widths.uuid = len(uuid) if uuid else 0
        self.UUID = uuid
        self.imgfile = imgfile
        if imgfile or uuid:
            self.widths.checked = 3
        self.widths.nfs = len(nfs) if nfs else 0
        self.nfs = nfs
        self.widths.depends = len(depends) if depends else 0
        self.depends = depends
        if luks:
            self.widths.luks = len(luks)
            self.widths.open = 4
            self.open = False
        else:
            self.open = True
        self.luks = luks
        self.checked = False
        self.mounted = False
        self.widths.passgrp = 1 if passgrp else 0
        self.passgrp = passgrp
        self.widths.mntpt = len(mntpt) if mntpt else 0
        self.mntpt = mntpt
        self.tickle = tickle
        self.open_output = ''
        self.fsck_output = ''
        self.mount_output = ''
