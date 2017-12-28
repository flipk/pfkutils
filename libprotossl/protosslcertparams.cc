
#include "libprotossl.h"

using namespace ProtoSSL;

ProtoSSLCertParams::ProtoSSLCertParams(
    const std::string &_caCert, // file:/...
    const std::string &_myCert, // file:/...
    const std::string &_myKey,  // file:/...
    const std::string &_myKeyPassword)
    : caCert(_caCert), myCert(_myCert), myKey(_myKey),
      myKeyPassword(_myKeyPassword)
{
}

ProtoSSLCertParams::~ProtoSSLCertParams(void)
{
}
