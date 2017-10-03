
enum {
    DIGEST_NONMATCH,
    DIGEST_MATCH,
    DIGEST_MATCH_ARRAY_SIZE
};
enum {
    ACTION_C,   // conflict
    ACTION_CI,  // impossible conflict
    ACTION_S2F, // second sends to first
    ACTION_F2S, // first seconds to second
    ACTION_NA,  // no action
    ACTION_D1,  // first deletes
    ACTION_D2   // second deletes
};

#define C   ACTION_C
#define CI  ACTION_CI
#define S2F ACTION_S2F
#define F2S ACTION_F2S
#define NA  ACTION_NA
#define D1  ACTION_D1
#define D2  ACTION_D2

#define FIRST_STATUSES  FFE_NUM_STATUSES
#define SECOND_STATUSES FFE_NUM_STATUSES

char trade_actions
[ FIRST_STATUSES ][ SECOND_STATUSES ][ DIGEST_MATCH_ARRAY_SIZE ] =
{
    /*   NF        NEW       DELETED   SAME     CHANGED */
    { {CI ,CI }, {F2S,F2S}, {CI,CI}, {CI ,CI}, {CI ,CI} },  // NF
    { {S2F,S2F}, {C  ,NA }, {CI,CI}, {C  ,NA}, {C  ,NA} },  // NEW
    { {CI ,CI }, {CI ,CI }, {NA,NA}, {D1 ,D1}, {C  ,C } },  // DELETED
    { {CI ,CI }, {C  ,NA }, {D2,D2}, {C  ,NA}, {F2S,NA} },  // SAME
    { {CI ,CI }, {C  ,NA }, {C ,C }, {S2F,NA}, {C  ,NA} }   // CHANGED
};

#undef C
#undef CI
#undef S2F
#undef F2S
#undef NA
#undef D1
#undef D2
