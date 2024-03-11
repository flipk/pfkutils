
schema = {
    # name is for printing only while running the tool.
    'name': '',

    # the source of this data is specified either as a UUID existing on
    # some hardware partition, as an image file, or a remote NFS server.
    # if it is an image file, it must exist on some other partition,
    # so see 'depends' field.
    'UUID': '',
    'imgfile': '',
    'nfs': '',

    # if this imgfile depends on another partition, but the 'name' of that
    # partition here.
    'depends': '',

    # the luks volume name when setting up a cryptsetup.
    # if this partition is NOT encrypted, this is None.
    'luks': '',

    # the password falls into groups. entries with the name
    # pass number have the same password and can be mounted
    # all at once. if an entry is not encrypted, this is None.
    'pass': 1,

    # the path there this partition is mounted.
    'mntpt': '',

    # should the tickler be spun up on this mountpoint?
    'tickle': False,
}

fs_list = [
    # list fs objects here.
]
