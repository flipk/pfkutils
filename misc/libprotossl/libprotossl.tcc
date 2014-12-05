/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */


// ProtoSSLCertParams

inline
ProtoSSLCertParams::ProtoSSLCertParams(const std::string &_caCertFile,
                                       const std::string &_myCertFile,
                                       const std::string &_myKeyFile,
                                       const std::string &_myKeyPassword,
                                       const std::string &_otherCommonName)
    : caCertFile(_caCertFile), myCertFile(_myCertFile),
      myKeyFile(_myKeyFile), myKeyPassword(_myKeyPassword),
      otherCommonName(_otherCommonName)
{
}

inline
ProtoSSLCertParams::~ProtoSSLCertParams(void)
{
    memset(const_cast<char*>(myKeyPassword.c_str()),
           0, myKeyPassword.length());
}


// __ProtoSSLMsgs

inline void
__ProtoSSLMsgs::setRcvdMsg(google::protobuf::Message * msg)
{
    rcvdMsg = msg;
}

inline google::protobuf::Message *
__ProtoSSLMsgs::_getMsgPtr(void)
{
    return rcvdMsg;
}



// ProtoSSLMsgs

template <class IncomingMessageType, class OutgoingMessageType>
ProtoSSLMsgs<IncomingMessageType,OutgoingMessageType>
::ProtoSSLMsgs(const ProtoSSLCertParams &params, int listeningPort)
    : __ProtoSSLMsgs(params, listeningPort)
{
    setRcvdMsg(&_rcvdMsg);
}

template <class IncomingMessageType, class OutgoingMessageType>
ProtoSSLMsgs<IncomingMessageType,OutgoingMessageType>
::ProtoSSLMsgs(const ProtoSSLCertParams &params,
                           const std::string &remoteHost, int remotePort)
    : __ProtoSSLMsgs(params, remoteHost, remotePort)
{
    setRcvdMsg(&_rcvdMsg);
}

template <class IncomingMessageType, class OutgoingMessageType>
ProtoSSLMsgs<IncomingMessageType,OutgoingMessageType>
::~ProtoSSLMsgs(void)
{
}

template <class IncomingMessageType, class OutgoingMessageType>
void ProtoSSLMsgs<IncomingMessageType,OutgoingMessageType>
::getEvent( ProtoSSLEvent<IncomingMessageType> &event,
                        int timeoutMs )
{
    ProtoSSLEventType type = _getEvent(event.connectionId, timeoutMs);
    event.type = type;
    if (type == PROTOSSL_MESSAGE)
        event.msg = dynamic_cast<IncomingMessageType*>(_getMsgPtr());
    else
        event.msg = NULL;
}

template <class IncomingMessageType, class OutgoingMessageType>
bool ProtoSSLMsgs<IncomingMessageType,OutgoingMessageType>
::sendMessage(const OutgoingMessageType &msg)
{
    return _sendMessage(msg);
}
