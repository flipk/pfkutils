
extern int treesync_verbose;

void     treesync_analyze     (char * dir1, TSFileEntryList * fel1,
                               char * dir2, TSFileEntryList * fel2 );
void     treesync_copy_file   (char *fromroot, char *fromfile,
                               char *toroot,   char *tofile    );
void     treesync_delete_file (char *root, char *file);
int      treesync_create_dirs (char * path);
void     treesync_display_md5 (char *path, UINT8 *md5);
void     treesync_calc_md5    (char *root_dir, char *relpath,
                               UINT8 * hashbuffer );
Btree *  open_treesync_db     (char *dir);
void     treesync_update_db   (char *root_dir, Btree * db,
                               TSFileEntryList * fel );

