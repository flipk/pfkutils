
#define DB_CACHE_SIZE 2097152
#define DB_ORDER      25
#define TREESYNC_DB  "treesync.db"

void analyze( char * dir1, FileEntryList * fel1,
              char * dir2, FileEntryList * fel2 );
void copy_file(char *fromroot, char *fromfile,
               char *toroot,   char *tofile    );
int create_dirs(char * path);
void display_md5(char *path, UINT8 *md5);
void calc_md5( char *root_dir, char *relpath, UINT8 * hashbuffer );
Btree * open_ts_db(char *dir);
void update_db( char *root_dir, Btree * db, FileEntryList * fel );
