
#ifndef __MAGIC_H_
#define __MAGIC_H_

// this file lists all magic numbers in use in the system.
// this way the assignment of random numbers is not haphazard
// and guarantees uniqueness.

enum MagicNumbers {

    MagicNumbers_Encrypt_KeyNone                = 0x7456e5c5,
    MagicNumbers_Encrypt_ClassNone              = 0x74aacf23,
    MagicNumbers_Encrypt_KeyRubik5              = 0x10a7fe59,
    MagicNumbers_Encrypt_ClassRubik5            = 0x68adf434,
    MagicNumbers_Encrypt_KeyRubik4              = 0x7dd744b7,
    MagicNumbers_Encrypt_ClassRubik4            = 0x3b927dca,

#if 0
    MagicNumbers_MessageEncoding_Header         = 0x31bc397f,
    MagicNumbers_MessageEncoding_HeaderEnc      = 0x126735df,
    MagicNumbers_MessageEncoding_GenericType    = 0x095be460,

    MagicNumbers_Messages_MessageFdActive       = 0x08553203,
    MagicNumbers_Messages_MsgsTcpIndication     = 0x48492709,
    MagicNumbers_Messages_ConnEst               = 0x856491df,
    MagicNumbers_Messages_KillConn              = 0x3ebb51d7,
    MagicNumbers_Messages_NewKeyReq             = 0x69098f33,
    MagicNumbers_Messages_NewKeyAck             = 0xd7a30ea5,
    MagicNumbers_Messages_NewKeyReqInt          = 0x3a23685d,
    MagicNumbers_Messages_NewKeyAckInt          = 0x1fe16547,
    MagicNumbers_Messages_LookupReq             = 0xbe788abf,
    MagicNumbers_Messages_LookupReply           = 0xbb28b0f3,
    MagicNumbers_Messages_conn_ind              = 0x094a699a,
    MagicNumbers_Messages_discon_ind            = 0x26f33c08,

    MagicNumbers_Threads_PrintMessage           = 0x24fec2ea
#endif

};

#endif /* __MAGIC_H_ */
