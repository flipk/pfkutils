/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*- */

/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
 */

#ifndef __FASTCGI_H__
#define __FASTCGI_H__

#include <sys/time.h>
#include <netinet/in.h>
#include <iostream>

#include "CircularReader.h"

namespace WebAppServer {

struct FastCGIHeader {
    FastCGIHeader(void) {
        version = VERSION_1;
        paddingLength = 0;
        reserved = 0;
    }
    uint8_t version;
    enum FastCGIVersion {
        VERSION_1 = 1
    };
    uint8_t type; // actually enum FastCGIType
    enum FastCGIType {
        BEGIN_REQ = 1,
        ABORT_REQ = 2,
        END_REQ = 3,
        PARAMS = 4,
        STDIN = 5,
        STDOUT = 6,
        STDERR = 7,
        DATA = 8,
        GET_VALUES = 9,
        GET_VALUES_RESULT = 10,
        UNKNOWN_TYPE = 11
    };
    uint16_t _requestId;
    uint16_t get_requestId(void) const {
        return ntohs(_requestId);
    }
    void set_requestId(uint16_t value) {
        _requestId = htons(value);
    }
    enum RequestIds {
        NULL_REQ_ID = 0
    };
    uint16_t _contentLength;
    uint16_t get_contentLength(void) const {
        return ntohs(_contentLength);
    }
    void set_contentLength(uint16_t value) {
        _contentLength = htons(value);
    }
    uint8_t paddingLength;
    uint8_t reserved;
} __attribute__((packed));

struct FastCGIBegin {
    enum FastCGIBeginRole {
        ROLE_RESPONDER = 1,
        ROLE_AUTHORIZER = 2,
        ROLE_FILTER = 3
    };
    uint16_t _role;
    uint16_t get_role(void) const {
        return ntohs(_role);
    }
    void set_role(uint16_t value) {
        _role = htons(value);
    }
    static const char FLAG_KEEP_CONN = 1;
    uint8_t flags;
    uint8_t reserved[5];
};

struct FastCGIEnd {
    uint32_t _appStatus;
    uint32_t get_appStatus(void) const {
        return ntohl(_appStatus);
    }
    void set_appStatus(uint32_t value) {
        _appStatus = htonl(value);
    }
    enum FastCGIProtocolStatus {
        STATUS_REQUEST_COMPLETE = 0,
        STATUS_CANT_MULTIPLEX = 1,
        STATUS_OVERLOADED = 2,
        STATUS_UNKNOWN_ROLE = 3
    };
    uint8_t protocolStatus;
    uint8_t reserved[3];
};

struct FastCGIParam {
    FastCGIParam(const CircularReaderSubstr &name,
                 const CircularReaderSubstr &value,
                 bool percentSubstitute=false);
    ~FastCGIParam(void);
    CircularReaderSubstr name;
    CircularReaderSubstr value;
};

struct FastCGIParamsList {
    FastCGIParamsList(void);
    FastCGIParamsList(const CircularReaderSubstr &queryString);
    ~FastCGIParamsList(void);
    CircularReader  paramBuffer;
    typedef std::map<CircularReaderSubstr,FastCGIParam*> param_t;
    typedef std::map<CircularReaderSubstr,FastCGIParam*>::iterator paramIter_t;
    param_t params;
};

struct FastCGIParams {
    uint8_t rawdata[0];

    FastCGIParamsList * getParams(uint16_t length) const;

    // length of name, either 1 byte or 4 bytes (if bit 7 is set)
    // length of value, either 1 byte or 4 bytes (if bit 7 is set)
    // name
    // value

    // common names :
    //    SCRIPT_FILENAME (/usr/share/nginx/html/cgi-bin/whatever)
    //    SCRIPT_NAME (/cgi-bin/whatever)
    //    DOCUMENT_URI (/cgi-bin/whatever)
    //    DOCUMENT_ROOT (/usr/share/nginx/html)
    //    REQUEST_URI (/cgi-bin/whatever?query_string)
    //    QUERY_STRING (the part after the ?, cannot use /)
    //    REQUEST_METHOD (GET or POST)
    //    CONTENT_TYPE (might be null)
    //    CONTENT_LENGTH (might be null)
    //    SERVER_PROTOCOL (HTTP/1.1)
    //    REMOTE_ADDR (ip address)
    //    REMOTE_PORT
    //    SERVER_ADDR
    //    SERVER_PORT
    //    SERVER_NAME (i.e. localhost from nginx.conf)
    //    HTTP_HOST (www.testserver.com, from browser)
    //    HTTP_USER_AGENT (i.e. mozilla)
    // 
    //    you have to parse the name=value&name=value stuff yourself
    //    from QUERY_STRING or the STDIN data.
};

struct FastCGIData {
    uint8_t  data[0];
};

struct FastCGIRecord {
    FastCGIHeader  header;
    union {
        FastCGIBegin  begin;
        FastCGIEnd    end;
        FastCGIParams params;
        FastCGIData   data;
        char   text[16384];
    } body;
};

} // namespace WebAppServer

#endif /* __FASTCGI_H__ */
