
#include "file_db.H"
#include "update_file.H"
#include "update_dir.H"

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

struct dir_item {
    LListLinks <dir_item> links[1];
    char * fname;
    dir_item( char * _fname ) {
        int l = strlen( _fname ) + 1;
        fname = new char[ l ];
        memcpy( fname, _fname, l );
    }
    ~dir_item( void ) {
        delete[] fname;
    }
};

typedef LList <dir_item,0> item_list;

void
update_dir( file_db * db, char * dirname )
{
    item_list  lst;
    dir_item  * di;

    di = new dir_item( dirname );
    lst.add( di );

    while (( di = lst.dequeue_head() ))
    {
        struct stat sb;

        if ( lstat( di->fname, &sb ) < 0 )
        {
            fprintf( stderr, "unable to stat '%s': %s\n",
                     di->fname, strerror( errno ));
            goto next_entry;
        }

        if ( S_ISDIR( sb.st_mode ))
        {
            DIR * d = opendir( di->fname );
            if ( !d )
            {
                fprintf( stderr, "cannot open dir '%s': %s\n",
                         dirname, strerror( errno ));
                goto next_entry;
            }
            struct dirent * de;
            while (( de = readdir(d) ))
            {
                char combined_name[ 512 ];
                if ( strcmp( de->d_name, "." ) == 0  ||
                     strcmp( de->d_name, ".." ) == 0 )
                {
                    continue;
                }
                sprintf( combined_name, "%s/%s",
                         di->fname, de->d_name );
                lst.add( new dir_item( combined_name ));
            }
            closedir( d );
        }
        else if ( S_ISREG( sb.st_mode ))
        {
            update_file( db, di->fname );
        }

    next_entry:
        delete di;
    }

}
