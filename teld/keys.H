/*
 * This file is licensed under the GPL version 2.
 * Refer to the file LICENSE in this distribution or
 * just search for GPL v2 on the website www.gnu.org.
 */

#define PFK_KEYLEN 64

struct PfkKeyPair {
    char challenge[ PFK_KEYLEN ];
    char response [ PFK_KEYLEN ];
};

class PfkKeyPairs {
    int num_pairs;
    PfkKeyPair * pairs;
    int fd;
public:
    PfkKeyPairs( void );
    PfkKeyPairs( int num );
    ~PfkKeyPairs(void);
    bool pick_key( PfkKeyPair * );
    bool find_response( PfkKeyPair * );
};
