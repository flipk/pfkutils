/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

#include "WebAppServer.h"
#include "WebAppServerInternal.h"
#include "base64.h"

#include <stdlib.h>

using namespace std;

namespace WebAppServer {

#define VERBOSE 1

WebFastCGIConnection :: WebFastCGIConnection(
    serverPort::ConfigRecList_t &_configs, int _fd)
    : WebServerConnectionBase(_configs, _fd)
{
    state = STATE_BEGIN;
    cgiParams = NULL;
    queryStringParams = NULL;
    cgiConfig = NULL;
    time(&lastCall);

    // i wanted to startFdThread in WebServerConnectionBase, but i can't
    // because it would call pure virtual methods that aren't set up until
    // this derived contructor is complete. so.. bummer.
    startFdThread(_fd);
}

WebFastCGIConnection :: ~WebFastCGIConnection(void)
{
    if (wac)
    {
        WebAppConnectionDataFastCGI * dat = wac->connData->fcgi();
        Lock lock(dat);
        dat->waiter = NULL;
        wac = NULL;
    }
    if (cgiParams)
        delete cgiParams;
    if (queryStringParams)
        delete queryStringParams;
}

//virtual
bool
WebFastCGIConnection :: handleSomeData(void)
{
    while (1)
    {
        size_t rbSize = readbuf.size();
        if (rbSize < sizeof(FastCGIHeader))
            break; // not enough for a header.

        FastCGIRecord rec;

        readbuf.copyOut((char*)&rec.header, 0, sizeof(FastCGIHeader));
        if (rec.header.version != FastCGIHeader::VERSION_1)
        {
            cerr << "Fast CGI proto version error" << endl;
            return false;
        }

        uint16_t contLen = rec.header.get_contentLength();
        uint16_t padLen = rec.header.paddingLength;
        uint16_t fullLen = contLen + padLen + sizeof(FastCGIHeader);

        if (fullLen > rbSize)
            break; // not enough for the body.

        if (0)
            cout << "handling record of size " << contLen << endl;

        if (fullLen > sizeof(FastCGIRecord))
        {
            cerr << "FastCGI record overflow!" << endl;
            return false;
        }

        readbuf.copyOut((char*)&rec.body,
                        sizeof(FastCGIHeader), contLen);

        if (handleRecord(&rec) == false)
            return false;

        readbuf.erase0(fullLen);
    }

    return true;
}

//virtual
bool
WebFastCGIConnection :: doPoll(void)
{
    time_t now = time(NULL);

    if ((now - lastCall) > maxIdleTime)
        return false;

    return true;
}

//virtual
void
WebFastCGIConnection :: done(void)
{
    // wac is not deleted because it lives beyond
    // this ephemeral connection.
    wac = NULL;
    deleteMe = true;
}

static void doPercentSubstitute(CircularReaderSubstr &str)
{
    size_t pos = 0;
    while (pos != string::npos)
    {
        int pc = str.find("%",pos);
        if (pc == CircularReaderSubstr::npos)
            break;
        char hex[3];
        hex[0] = str[pc+1];
        hex[1] = str[pc+2];
        hex[2] = 0;
        char c = (char) strtoul(hex,NULL,16);
        str[pc] = c;
        str.erase(pc+1,2);
        pos = pc+1;
    }
}

FastCGIParam :: FastCGIParam(const CircularReaderSubstr &_name,
                             const CircularReaderSubstr &_value,
                             bool percentSubstitute /*=false*/)
    : name(_name), value(_value)
{
    if (percentSubstitute)
    {
        doPercentSubstitute(name);
        doPercentSubstitute(value);
    }
}

FastCGIParam :: ~FastCGIParam(void)
{
}

FastCGIParamsList :: FastCGIParamsList(void)
{
}

FastCGIParamsList :: FastCGIParamsList(const CircularReaderSubstr &queryString)
{
    int namePos = 0, valuePos, eqPos, ampPos;

    if (queryString.size() == 0)
        return;

    paramBuffer = queryString;

    while (namePos != CircularReaderSubstr::npos)
    {
        CircularReaderSubstr name, value;
        eqPos = paramBuffer.find("=", namePos);
        if (eqPos == CircularReaderSubstr::npos)
        {
            // if no '=', then add one variable whose
            // name is the whole querystring, with no value,
            // and bail.
            name = paramBuffer;
            namePos = CircularReaderSubstr::npos;
        }
        else
        {
            name = paramBuffer.substr(namePos,eqPos-namePos);
            valuePos = eqPos+1;
            ampPos = paramBuffer.find("&", eqPos);
            if (ampPos != CircularReaderSubstr::npos)
            {
                // found an '&'.
                value = paramBuffer.substr(valuePos,ampPos-valuePos);
                namePos = ampPos+1;
            }
            else
            {
                // there was no '&', so this variable's value
                // is the rest of the queryString, and bail.
                value = paramBuffer.substr(valuePos);
                namePos = CircularReaderSubstr::npos;
            }
        }
        params[name] = new FastCGIParam(name,value,true);
    }
}

FastCGIParamsList :: ~FastCGIParamsList(void)
{
    FastCGIParamsList::paramIter_t it;
    for (it = params.begin(); it != params.end(); it++)
    {
        FastCGIParam * p = it->second;
        delete p;
        params.erase(it);
    }
}

int getFieldLen(const CircularReaderSubstr &data,
                int &pos, int &remaining)
{
    int nameLen = data[pos];
    if ((nameLen & 0x80) == 0x80)
    {
        // this is a 32-bit length, not an 8-bit length.
        if (remaining < 4)
        {
            cerr << "getFieldLen: not enough space for 32-bit len" << endl;
            return 0xFFFFFFFF;
        }
        nameLen = 
            ((((uint8_t)data[pos+0]) & 0x7f) << 24) |
            ( ((uint8_t)data[pos+1])         << 16) |
            ( ((uint8_t)data[pos+2])         <<  8) |
              ((uint8_t)data[pos+3]); 
        pos += 4;
        remaining -= 4;
    }
    else
    {
        pos += 1;
        remaining -= 1;
    }
    return nameLen;
}

FastCGIParamsList *
FastCGIParams :: getParams(uint16_t length) const
{
    FastCGIParamsList * ret = new FastCGIParamsList;

    CircularReader &data = ret->paramBuffer;
    data.assign((char*)rawdata, (int)length);
    int pos = 0;
    int remaining = (int)length;

    while (pos < length)
    {
        int nameLen, valueLen;
        nameLen = getFieldLen(data, pos, remaining);
        if (nameLen < 0)
            break;
        valueLen = getFieldLen(data, pos, remaining);
        if (valueLen < 0)
            break;
        if (remaining < nameLen) // cast to remove warning
        {
            cerr << "getParams: not enough space for name" << endl;
            goto fail;
        }
        CircularReaderSubstr fieldName = data.substr(pos, nameLen);
        pos += nameLen;
        remaining -= nameLen;
        CircularReaderSubstr fieldValue = data.substr(pos, valueLen);
        pos += valueLen;
        ret->params[fieldName] = new FastCGIParam(fieldName,fieldValue);
    }

    return ret;
fail:
    delete ret;
    return NULL;    
}

bool
WebFastCGIConnection :: handleRecord(const FastCGIRecord *rec)
{
    bool ret = true;

    if (0)
        printRecord(rec);

    switch (rec->header.type)
    {
    case FastCGIHeader::BEGIN_REQ:
        ret = handleBegin(rec);
        break;

    case FastCGIHeader::ABORT_REQ:
        cout << "got ABORT" << endl;
        ret = false;
        break;

    case FastCGIHeader::END_REQ:
        cout << "got END request" << endl;
        ret = false;
        break;

    case FastCGIHeader::STDIN:
        ret = handleStdin(rec);
        break;

    case FastCGIHeader::PARAMS:
        ret = handleParams(rec);
        break;

    default:
        cout << "got unhandled record:" << endl;
        printRecord(rec);
    }

    return ret;
}

bool
WebFastCGIConnection :: handleBegin(const FastCGIRecord *rec)
{
    if (state != STATE_BEGIN)
    {
        cerr << "WebFastCGIConnection does not support multiplexing" << endl;
        FastCGIRecord  resp;
        resp.header.type = FastCGIHeader::END_REQ;
        resp.header.set_requestId(rec->header.get_requestId());
        resp.header.set_contentLength(sizeof(FastCGIEnd));
        resp.body.end.set_appStatus(0);
        resp.body.end.protocolStatus = FastCGIEnd::STATUS_CANT_MULTIPLEX;
        sendRecord(resp);
        return false;
    }
    else
    {
        requestId = rec->header.get_requestId();
        uint16_t role = rec->body.begin.get_role();
        if (role != FastCGIBegin::ROLE_RESPONDER)
        {
            cout << "unsupported fastcgi role " << role << endl;
            return false;
        }
        state = STATE_PARAMS;
        if (0)
            cout << "got cgi begin record" << endl;
    }
    return true;
}

bool
WebFastCGIConnection :: handleParams(const FastCGIRecord *rec)
{
    if (state != STATE_PARAMS)
    {
        cerr << "fastcgi protocol error: not in PARAMS state" << endl;
        return false;
    }

    uint16_t contentLength = rec->header.get_contentLength();

    if (contentLength == 0)
    {
        if (0)
            cout << "got zero length params, moving to INPUT state" << endl;
        state = STATE_INPUT;
        return startWac();
    }

    if (0)
        cout << "got Params record of length " << contentLength << endl;

    cgiParams = rec->body.params.getParams(
        rec->header.get_contentLength());
    FastCGIParamsList::paramIter_t it;
    for (it = cgiParams->params.begin();
         it != cgiParams->params.end(); it++)
    {
        FastCGIParam * p = it->second;
        if (VERBOSE)
            cout << "param: " << p->name << " = " << p->value << endl;
    }

    it = cgiParams->params.find(CircularReader("QUERY_STRING"));
    if (it != cgiParams->params.end())
    {
        FastCGIParam * qs = it->second;
        queryStringParams = new FastCGIParamsList(qs->value);
        for (it = queryStringParams->params.begin();
             it != queryStringParams->params.end(); it++)
        {
            FastCGIParam * p = it->second;
            if (VERBOSE)
                cout << "param: " << p->name << " = " << p->value << endl;
        }
    }

    return true;
}

bool
WebFastCGIConnection :: handleStdin(const FastCGIRecord *rec)
{
    if (state != STATE_INPUT)
    {
        cerr << "fastcgi protocol error: not in INPUT state" << endl;
        return false;
    }

    uint16_t contentLength = rec->header.get_contentLength();

    if (contentLength == 0)
    {
        if (VERBOSE)
            cout << "got end of input, decoding" << endl;

        if (decodeInput() == false)
            return false;

        if (VERBOSE)
            cout << "switching to OUTPUT" << endl;

        return startOutput();
    }

    if (VERBOSE)
        cout << "got stdin record of length " << contentLength << endl;

    printRecord(rec);

    stdinBuffer.append( rec->body.text, 
                        rec->header.get_contentLength() );

    return true;
}

void
WebFastCGIConnection :: printRecord(const FastCGIRecord *rec)
{
    uint8_t * header = (uint8_t*) &rec->header;
    uint8_t * body   = (uint8_t*) &rec->body;
    uint16_t  bodylen = rec->header.get_contentLength();
    printf("got record:\n");
    printf("  header:  ");
    for (uint32_t ctr = 0; ctr < sizeof(rec->header); ctr++)
        printf("%02x", header[ctr]);
    printf("\n");
    printf("  body: ");
    for (int ctr = 0; ctr < bodylen; ctr++)
        printf("%02x", body[ctr]);
    printf("\n");
}

bool
WebFastCGIConnection :: sendRecord(const FastCGIRecord &rec)
{
    int writeLen =
        sizeof(FastCGIHeader) +
        rec.header.get_contentLength() + 
        rec.header.paddingLength;

    int cc = ::write(fd, &rec, writeLen);
    
    if (cc != writeLen)
        return false;

    return true;
}

//virtual
void
WebFastCGIConnection :: sendMessage(const WebAppMessage &m)
{
    FastCGIRecord  mimeHeaders;
    mimeHeaders.header.version = FastCGIHeader::VERSION_1;
    mimeHeaders.header.type = FastCGIHeader::STDOUT;
    mimeHeaders.header.paddingLength = 0;
    mimeHeaders.header.reserved = 0;
    mimeHeaders.header.set_requestId(requestId);
    /** \todo bounds check */
    memcpy(mimeHeaders.body.text, m.buf.c_str(),
           m.buf.length());
    mimeHeaders.header.set_contentLength( m.buf.length() );
    printRecord(&mimeHeaders);
    // we really don't care about return value, because
    // in either case we're returning 'false' from the sender
    // to close the connection and complete the transaction
    // to the browser.
    (void) sendRecord(mimeHeaders);
    stopFdThread();
}

bool
WebFastCGIConnection :: startWac(void)
{
    FastCGIParamsList::paramIter_t paramit =
        cgiParams->params.find(CircularReader("DOCUMENT_URI"));

    if (paramit == cgiParams->params.end())
    {
        cerr << "DOCUMENT_URI is not set?!" << endl;
        return false;
    }
    resource = paramit->second->value.toString();

    if (findResource() == false)
    {
        cerr << "no handler installed for path: " << resource << endl;
        return false;
    }
    if (VERBOSE)
        cout << "found handler for path: " << resource << endl;

    cgiConfig = dynamic_cast<WebAppServerFastCGIConfigRecord*>(config);
    if (cgiConfig == NULL)
    {
        cerr << "invalid config, config rec is not a fastCgi config rec!"
             << endl;
        return false;
    }

    string visitorId;

    paramit = cgiParams->params.find(CircularReader("HTTP_COOKIE"));
    if (paramit != cgiParams->params.end())
    {
        FastCGIParam * p = paramit->second;
        if (VERBOSE)
            cout << "cookie: " << p->value << endl;

        size_t visitorIdNamePos = p->value.find("visitorId=");
        if (visitorIdNamePos != string::npos)
        {
            visitorIdNamePos += 10; // length of "visitorId="
            int semicolonOrEndPos =
                p->value.find_first_of(';',visitorIdNamePos);
            visitorId = p->value.toString(visitorIdNamePos,semicolonOrEndPos);
        }
    }

    cookieString.clear();

    if (visitorId.length() == 0)
    {
        if (VERBOSE)
            cout << "found no visitor Id, generating one" << endl;

        generateNewVisitorId(visitorId);

        cookieString = "Set-Cookie: visitorId=";
        cookieString.append(visitorId);
        cookieString += "; path=/\r\n";
    }
    else
    {
        if (VERBOSE)
            cout << "found visitor id : " << visitorId << endl;
    }

    WebAppServerFastCGIConfigRecord::ConnListIter_t visitorIt;

    {
        Lock lock(cgiConfig);
        visitorIt = cgiConfig->conns.find(visitorId);
        if (visitorIt == cgiConfig->conns.end())
        {
            // make a new one
            wac = config->cb->newConnection();
            wac->connData = new WebAppConnectionDataFastCGI;
            cgiConfig->conns[visitorId] = wac;
            if (VERBOSE)
                cout << "made a new wac" << endl;
        }
        else
        {
            wac = visitorIt->second;
            if (VERBOSE)
                cout << "found existing wac" << endl;
        }
    }

    return true;
}

bool
WebFastCGIConnection :: decodeInput(void)
{
    std::string decodeBuf;

    /** \todo */

    // base64 decode

    const WebAppMessage m(WS_TYPE_BINARY, decodeBuf);
    if (wac->onMessage(m) == false) // \todo mutex
        return false;

    return true;
}

bool
WebFastCGIConnection :: startOutput(void)
{
    state = STATE_OUTPUT;
    time(&lastCall);

    {
        FastCGIRecord  mimeHeaders;
        mimeHeaders.header.version = FastCGIHeader::VERSION_1;
        mimeHeaders.header.type = FastCGIHeader::STDOUT;
        mimeHeaders.header.paddingLength = 0;
        mimeHeaders.header.reserved = 0;
        mimeHeaders.header.set_requestId(requestId);

        int len = snprintf(mimeHeaders.body.text,
                           sizeof(mimeHeaders.body.text),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/ascii\r\n"
                           "%s\r\n", cookieString.c_str());
        mimeHeaders.header.set_contentLength(  len  );

        printRecord(&mimeHeaders);
        if (sendRecord(mimeHeaders) == false)
            return false;
    }

    FastCGIParamsList::paramIter_t it;
    it = cgiParams->params.find(CircularReader("REQUEST_METHOD"));
    if (it != cgiParams->params.end())
    {
        FastCGIParam * qs = it->second;
        cout << "REQUEST_METHOD value is " << qs->value << endl;

        if (qs->value == "POST")
        {
            // we're done here, no output. there's a separate
            // GET connection for messages going the other way.

            return false;
        }
    }

    /* if there is a message queued, send it here and return false to 
       close the connection so the client gets it. 
       if there is no message queued, return true. */

    WebAppConnectionDataFastCGI * dat = wac->connData->fcgi();
    Lock lock(dat);

    if (dat->outq.size() > 0)
    {
        dat->sendFrontMessage();
    }
    else
    {
        // queue is empty
        state = STATE_BLOCKED;
        dat->waiter = this;
        return true;
    }

    return false;
}

void
WebFastCGIConnection :: generateNewVisitorId(
    std::string &visitorId)
{
    static const char randChars[] =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    static const int numRandChars = sizeof(randChars)-1; // minus nul

    while (1)
    {
        visitorId.clear();
        for (int cnt = 0; cnt < visitorCookieLen; cnt++)
            visitorId.push_back(randChars[random() % numRandChars]);
        cout << "trying visitorId " << visitorId << endl;
        {
            Lock lock(cgiConfig);
            if (cgiConfig->conns.find(visitorId) == cgiConfig->conns.end())
                return;
        }
        // generated a visitorId that already exists; go round again.
    }
}

void
WebAppConnectionDataFastCGI :: sendMessage(const WebAppMessage &m)
{ 
    int quant = 0, left = m.buf.size();

    if (left == 0)
    {
        cerr << "WebFastCGIConnection :: sendMessage : size=0? no." << endl;
        return;
    }

    std::string  b64_str;

    // calculate number of b64 encoded quanta are
    // needed to encode a binary string.
    b64_str.resize((((left-1)/3)+1)*4);

    unsigned char * inbuf = (unsigned char *) m.buf.c_str();
    unsigned char * outbuf = (unsigned char *) b64_str.c_str();

    for (quant = 0; left <= 0; quant++, left -= 3)
    {
        b64_encode_quantum(inbuf + (quant*3),
                           (left >= 3) ? 3 : left, 
                           outbuf + (quant*4));
    }

    Lock  lock(this);
    outq.push_back(b64_str);

    if (waiter)
        sendFrontMessage();
}

// this object should be locked before calling this.
void
WebAppConnectionDataFastCGI :: sendFrontMessage(void)
{
    const WebAppMessage m(WS_TYPE_BINARY, outq.front());
    waiter->sendMessage(m);
    outq.pop_front();
}

WebAppServerFastCGIConfigRecord :: WebAppServerFastCGIConfigRecord(
    WebAppType _type, 
    int _port,
    const std::string _route,
    WebAppConnectionCallback *_cb,
    int _pollInterval )
    : WebAppServerConfigRecord(_type,_port,_route,_cb,_pollInterval)
{
    if (pollInterval > 0)
    {
        pipe(closePipe);
        pthread_attr_t  attr;
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        pthread_create(&thread_id, &attr, &_thread_entry, (void*) this);
        pthread_attr_destroy(&attr);
    }
}

WebAppServerFastCGIConfigRecord :: ~WebAppServerFastCGIConfigRecord(void)
{
    if (pollInterval > 0)
    {
        close(closePipe[1]);
        void * dummy;
        pthread_join(thread_id, &dummy);
        close(closePipe[0]);
    }
}

//static
void *
WebAppServerFastCGIConfigRecord :: _thread_entry(void * me)
{
    WebAppServerFastCGIConfigRecord * obj =
        (WebAppServerFastCGIConfigRecord *) me;
    obj->thread_entry();
    return NULL;
}

void
WebAppServerFastCGIConfigRecord :: thread_entry(void)
{
    while (1)
    {
        struct timeval tv;
        fd_set rfds;
        tv.tv_sec  = pollInterval / 1000;
        tv.tv_usec = pollInterval % 1000;
        FD_ZERO(&rfds);
        FD_SET(closePipe[0], &rfds);
        if (select(closePipe[0]+1, &rfds, NULL, NULL, &tv) > 0)
            break;

        Lock lock(this);

        ConnListIter_t it;
        for (it = conns.begin(); it != conns.end(); it++)
        {
            WebAppConnection * wac = it->second;
            if (wac->doPoll() == false)
            {
    /** \todo mutex interactions with wac */
                delete wac;
                conns.erase(it);
            }
        }
    }
}

} // namespace WebAppServer
