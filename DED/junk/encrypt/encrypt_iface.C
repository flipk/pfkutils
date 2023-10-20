
// this module understands all supported encryption types
// and parses them out.

#include "encrypt_iface.H"
#include "encrypt_rubik4.H"
#include "encrypt_rubik5.H"
#include "encrypt_none.H"
#include "lognew.H"

encrypt_iface *
parse_key( char * keystring )
{
    encrypt_key * key = NULL;
    encrypt_iface * ret = NULL;

    if ( encrypt_none_key::valid_keystring( keystring ))
        key = LOGNEW encrypt_none_key;
    else if ( rubik4_key::valid_keystring( keystring ))
        key = LOGNEW rubik4_key;
    else if ( rubik5_key::valid_keystring( keystring ))
        key = LOGNEW rubik5_key;
    
    if ( key == NULL )
        return NULL;

    if ( key->key_parse( keystring ) == false )
    {
        delete key;
        return NULL;
    }

    switch ( key->type )
    {
    case encrypt_none_key::TYPE:
        ret = LOGNEW encrypt_none;
        break;
    case rubik5_key::TYPE:
        ret = LOGNEW rubik5;
        break;
    case rubik4_key::TYPE:
        ret = LOGNEW rubik4;
        break;
    }

    if ( ret == NULL )
    {
        delete key;
        return NULL;
    }

    ret->key = key;

    return ret;
}
