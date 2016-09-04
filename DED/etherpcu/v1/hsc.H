
//
// operations:
//
//   register a mailbox for indications
//   attempt to take mastership, return if i am master or secondary
//   release mastership (sends indication to other that it has it)
//   if secondary attempts to take mastership, send indication to master
//   master can issue a reset to secondary (indication)
//

#ifndef __HSC_H_
#define __HSC_H_

#include "threads.H"

MsgDef( HSC_OTHER_RELEASED, 0xd705b0d, );
MsgDef( HSC_OTHER_ATTEMPTED_CLAIM, 0xd2a4e52, );
MsgDef( HSC_RESET_PLEASE, 0xd5a7989, );

class hsc : public ThreadShortCuts {
    int cage;
    int slot7_mqid;
    int slot9_mqid;
    int owner;
public:
    enum hsc_take_bcast_type { BCAST_START, BCAST_FINISH };
    typedef void (*bcast_func_type)( void *, hsc_take_bcast_type );
    hsc( int _cage, int _slot7_mqid, int _slot9_mqid );
    bool take( int slot, bcast_func_type func, void * arg );
    void release( int slot );
    void reset_other( int slot );
};

#endif /* __HSC_H_ */
