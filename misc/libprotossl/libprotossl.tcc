/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

inline
ProtoSSLCertParams::ProtoSSLCertParams(
    const std::string &_caCert, // file:/...
    const std::string &_myCert, // file:/...
    const std::string &_myKey,  // file:/...
    const std::string &_myKeyPassword,
    const std::string &_otherCommonName)
    : caCert(_caCert), myCert(_myCert), myKey(_myKey),
      myKeyPassword(_myKeyPassword), otherCommonName(_otherCommonName)
{
}

inline
ProtoSSLCertParams::~ProtoSSLCertParams(void)
{
}

inline void
_ProtoSSLConn::closeConnection(void)
{
    char dummy = 1;
    ::write(exitPipe[1], &dummy, 1);
}

inline void
_ProtoSSLConn::stopMsgs(void)
{
    msgs->stop();
}

inline void
ProtoSSLMsgs::stop(void)
{
    char dummy = 1;
    ::write(exitPipe[1], &dummy, 1);
}
