/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#ifndef __CHAT_CONNECTION_H__
#define __CHAT_CONNECTION_H__ 1

#include <string>
#include "pfkchat-messages.pb.h"

void initChatServer(void);

class pfkChatAppConnection : public WebAppServer::WebAppConnection {
    int id;
    std::string username;
    bool authenticated;
    int idleTime;
    int unreadCount;
    PFK::Chat::TypingState typing;
    void sendClientMessage(const PFK::Chat::ServerToClient &msg,
                           bool broadcast, bool meToo = true);
    void sendUserList(bool broadcast, bool meToo = true);
public:
    pfkChatAppConnection(void);

    const bool get_authenticated(void) const { return authenticated; }
    const PFK::Chat::TypingState get_typing(void) { return typing; }
    const std::string& get_username(void) const { return username; };
    const int get_idleTime(void) { return idleTime; }
    const int get_unreadCount(void) { return unreadCount; }
    const int get_id(void) { return id; }

    /*virtual*/ ~pfkChatAppConnection(void);
    /*virtual*/ bool onMessage(const WebAppServer::WebAppMessage &);
    /*virtual*/ bool doPoll(void);
};

class pfkChatAppConnectionCallback : public WebAppServer::WebAppConnectionCallback {
public:
    /*virtual*/ WebAppServer::WebAppConnection * newConnection(void)
    {
        return new pfkChatAppConnection;
    }
};

#endif /* __CHAT_CONNECTION_H__ */
