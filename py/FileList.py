
import os
import sys
from typing import Union, TextIO


class _FileEntry:
    """
    describes a file. fe.fullpath is the path to the file,
    while fe.filename is the upper-case version of the
    filename only, for string matching purposes.
    """
    fullpath: str  # the full path (minus prefix)
    filename: str  # just the basename
    firstwo: str   # just the first two chars

    def __init__(self, fullpath: str):
        self.fullpath = fullpath
        self.filename = os.path.basename(fullpath).upper()
        self.firstwo = self.filename[0:2]

    def __str__(self) -> str:
        if self.is_keeper():
            return "%s:%s" % (self.firsttwo, self.fullpath)
        return ""

    @property
    def doc(self) -> Union[dict, None]:
        if self.is_keeper():
            return {'firsttwo': self.firsttwo,
                    'filename': self.filename,
                    'fullpath': self.fullpath}
        return None

    # for now lets only keep the ones that look like our ids.
    # this is probably over simplified.
    def is_keeper(self):
        # note firsttwo is already in upper format.
        if self.firstwo[0] not in ['C', 'S', 'R']:
            return False
        # P's are ok.
        if self.firstwo[1] == 'P':
            return True
        # now look for 1-6 since no id has more than 6.
        if not self.firstwo[1].isdigit():
            return False
        d = int(self.firstwo[1])
        if d < 1 or d > 6:
            return False
        return True


class FileList:
    FileList = dict
    _filelist: FileList
    _handle: TextIO

    def __init__(self, path: str):
        """
        this version forks a 'find' command and parses the stdout.
        so, that's neat, right?
        """
        self._ok = False
        try:
            cp = subprocess.run(['find', path, '-type', 'f'],
                                stdout=subprocess.PIPE, encoding='utf-8', check=True)
            self._files = []
            lines = cp.stdout.split('\n')
            for line in lines:
                if len(line) > 0:
                    fe = _FileEntry(line)
                    self._files.append(fe)
            self._ok = True
        except Exception as e:
            print('subprocess exception:', e)

    def __init__(self, fn: str):
        """
        reads in a file containing the full list of filenames
        in the filesystem, and returns a dict containing
        digested versions of those filenames.
        note in many cases, the same filename exists in 2 or more
        subdirectories, so the FileList is in fact a two-level
        nested dictionary.
        :param fn: the name of the file
        :return: a nested dict [key:filename.upper()[0:2],
                         value: another dict [key:filename.upper(),
                                              value:list of full paths]
        """
        print("loading filenames, please wait...")
        try:
            self._handle = open(fn, 'r', encoding='utf-8')
        except BaseException as e:
            print("cant open file list:", e)
            raise
        self._filelist = {}
        ok = True
        count = 0
        while ok:
            line = self._handle.readline()
            if len(line) == 0:
                ok = False
            else:
                fe = _FileEntry(line.rstrip())
                if fe.is_keeper():
                    if fe.firstwo not in self._filelist:
                        d2 = {}
                        self._filelist[fe.firstwo] = d2
                    else:
                        d2 = self._filelist[fe.firstwo]
                    if fe.filename in d2:
                        l3 = d2[fe.filename]
                        l3.append(fe)
                    else:
                        d2[fe.filename] = [fe]
                    count += 1
                    if count % 8192 == 0:
                        print("\r", count, " ", end='')
                        sys.stdout.flush()
        print("\r", count, "\nloaded filenames:", self.count_known_docs(True))

    def find_a_doc(self, doc_id: str) -> Union[list, None]:
        up = doc_id.upper()
        firsttwo = up[0:2]
        if firsttwo in self._filelist:
            lst = self._filelist[firsttwo]
            ret = []
            for fn, felist in lst.items():
                if up in fn:
                    for fe in felist:
                        ret.append(fe.fullpath)
            # long boring story short: there's a lot of them
            # if len(ret) > 1:
            #     print("found dups:", ret)
            return ret
        return None

    def count_known_docs(self, count_all: bool = False) -> str:
        ret = ""
        if count_all:
            filecount = 0
            for key in self._filelist.keys():
                filecount += len(self._filelist[key])
            ret += "F:%u," % filecount
        validfilecount = 0
        for key in self._filelist.keys():
            if self.valid_doc_id(key):
                validfilecount += len(self._filelist[key])
        ret += "VF:%u" % validfilecount
        if count_all:
            ret += ",D:%u" % len(self._filelist)
        return ret

    @staticmethod
    def valid_doc_id(doc_id: str) -> bool:
        """
        returns True or False indicating if the id should be
        looked at more closely.
        :param doc_id: the id
        :return: True if the id can be processed.
        """
        #
        if doc_id[0] in ['C', 'S', 'R']:
            return True
        return False
