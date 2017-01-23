
#ifndef __INO_TYPES_H_
#define __INO_TYPES_H_

typedef enum {
    INODE_REMOTE  = 1,
    INODE_VIRTUAL = 2
} inode_type;

typedef enum {
    INODE_DIR   = 1,
    INODE_FILE  = 2,
    INODE_LINK  = 3  /* (symlink)  */
} inode_file_type;

#endif /* __INO_TYPES_H_ */
