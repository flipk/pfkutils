/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

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

#ifndef __WEBAPPMESSAGE_H__
#define __WEBAPPMESSAGE_H__

namespace WebAppServer {

/** message types. \note FastCGI supports only BINARY at present. */
typedef enum {
    WS_TYPE_INVALID,  //!< initialization value
    WS_TYPE_TEXT,     //!< text mode (websocket only)
    WS_TYPE_BINARY,   //!< binary mode
    WS_TYPE_CLOSE     //!< close request (websocket only)
} WebAppMessageType;

/** message container passed between libWebAppServer and
 * your WebAppConnection. */
struct WebAppMessage {
    /** constructor.
     * \param _type  specify the type of the message
     * \param _buf   a std::string buffer containing the message body.
     * \note most messages should probably be WS_TYPE_BINARY, as
     *      WS_TYPE_TEXT messages are limited in the character sets
     *      they can pass. */
    WebAppMessage(WebAppMessageType _type,
                  const std::string &_buf) : type(_type), buf(_buf) { }
    /** the message type */
    WebAppMessageType type;
    /** the message contents */
    const std::string &buf;
};

extern const std::string websocket_guid;

} // namespace WebAppServer

#endif /* __WEBAPPMESSAGE_H__ */
