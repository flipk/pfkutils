
#ifndef __TREESYNC_H_
#define __TREESYNC_H_

#define TREESYNC_FILE_PREFIX        "00TSDB."
#define TREESYNC_DB_FILE            TREESYNC_FILE_PREFIX "bt"
#define TREESYNC_CONFLICT_FILE      TREESYNC_FILE_PREFIX "conflict"

#define TREESYNC_ORDER        15
#define TREESYNC_CACHE_INFO   1000

#include <sys/stat.h>

struct db_file_entry {
    int random_signature;  // changes each time 'regen' is run
    struct stat sb;
};

#endif /* __TREESYNC_H_ */
