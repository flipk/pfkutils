/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

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
