import sqlite3
import json

db_file = '/home/flipk/.local/share/opencode/opencode.db'
db = sqlite3.connect(db_file)

if True:
   print(f'{"="*35} SESSIONS {"="*35}')
   sessions = db.execute('select id,directory,title from session')
   for s in sessions:
      print(f'{s[0]}: {s[1]}\n'
            f'title: {s[2]}')
   try:
      sessions = db.execute('select id,directory,title from session_v2')
      for s in sessions:
         print(f'{s[0]}: {s[1]}\n'
               f'title: {s[2]}')
   except:
      # opencode v1 may not have the _v2 table, skip it
      pass
   print ("="*80+'\n')

if True:
   projects = db.execute('select id,worktree from project')
   prjs = []
   for p in projects:
      prjs.append((p[0], p[1]))
   widths = [0, 0]
   for p in prjs:
      for ind in [0,1]:
         w = len(p[ind])
         if w > widths[ind]:
            widths[ind] = w

   print(f'{"project id":<{widths[0]}} {"worktree":<{widths[1]}}')
   print('='*widths[0], '='*widths[1])
   for p in prjs:
      print (f'{p[0]:<{widths[0]}} {p[1]:<{widths[1]}}')
   print('='*widths[0], '='*widths[1])
   print ()

if False:
   events = db,execute('select id, seq,type,data from event')
   for e in events:
      t = e[2]
      print(f'id: {e[0]}\n'
            f'seq: {e[1]}\n'
            f'type: {e[2]}\n'
            f'data: {e[3]}\n')

db.close()
