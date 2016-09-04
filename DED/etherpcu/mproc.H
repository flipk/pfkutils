
#ifndef __MPROC_H__
#define __MPROC_H__

struct mproc_info {
    int mac;
    int pcu;
    int mproc;
};

#define PCUMPF  "%02d:%02d:%d:%06d: "
#define PCUMP info->mac, info->pcu, info->mproc, time_get()

void mproc_main( mproc_info * info );

#endif /* __MPROC_H__ */
