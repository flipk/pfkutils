
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
