/*
 * d3des algorithm from richard outerbridge, converted to a C++ method
 * by phillip f knaack for the 'enctelgate' project.
 *
 * core of algorithm completely unaltered.  simply rearranged into a class
 * so that encryption/decryption data contained inside class, which also
 * allows multiple instantiations to exist simultaneously.
 */


/* d3des.h -
 *
 *	Headers and defines for d3des.c
 *	Graven Imagery, 1992.
 *
 * Copyright (c) 1988,1989,1990,1991,1992 by Richard Outerbridge
 *	(GEnie : OUTER; CIS : [71755,204])
 */

class DES_CRYPT {

    enum en_de_flag { ENCRYPT,  DECRYPT };

    en_de_flag      flag;
    unsigned long * KnL;
    unsigned long   KnL_encrypt[32];
    unsigned long   KnL_decrypt[32];

    void usekey  ( unsigned long * from );
    void scrunch ( unsigned char *, unsigned long * );
    void unscrun ( unsigned long *, unsigned char * );
    void desfunc ( unsigned long *, unsigned long * );
    void cookey  ( unsigned long * );
    void deskey  ( const unsigned char *, en_de_flag );

public:
    DES_CRYPT( const unsigned char * key );   // [8]
    void encrypt( unsigned char * from, unsigned char * to );  // [8], [8]
    void decrypt( unsigned char * from, unsigned char * to );  // [8], [8]
};
