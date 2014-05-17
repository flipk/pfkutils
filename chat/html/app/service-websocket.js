
var webSocketServiceLastModified = {
    file:"service-websocket.js", modified:"2013/10/28  13:29:59"
};

var webSocketService = function($rootScope, data) {
    var ret = {
        socket : null,
        handlers : [],
        makesocket : null,
        newsocket : null,
        wsuri : ""
    };

    debugWsService = ret;
    debugWsRootScope = $rootScope;

    var lastPingSent = -1;

    var myuri = window.location;
    var newuri = "wss://";
    var colon = myuri.host.indexOf(":");
    if (colon == -1)
        newuri += myuri.host;
    else
        newuri += myuri.host.substring(0,colon);
    newuri += "/websocket/pfkchat";
    ret.wsuri = newuri;

    ret.handlers[PFK.Chat.ServerToClientType.STC_PROTOVERSION_RESP] =
        function(stc) {
            if (stc.protoversionresp !=
                PFK.Chat.ProtoVersionResp.PROTO_VERSION_MATCH)
            {
                data.savePersistent();
                location.reload(true);
            }
            else
            {
                ret.socket = ret.newsocket;
                data.setStatus('CONNECTED','white');
                if (data.token == "")
                {
                }
                else
                {
                    var cts = new PFK.Chat.ClientToServer;
                    cts.type = PFK.Chat.ClientToServerType.CTS_LOGIN;
                    cts.login = new PFK.Chat.Login;
                    cts.login.username = data.username;
                    cts.login.token = data.token;
                    if (ret.socket)
                        ret.socket.send(cts.toArrayBuffer());
                }
            }
        };

    ret.handlers[PFK.Chat.ServerToClientType.STC_LOGIN_STATUS] =
        function(stc) {
            switch (stc.loginStatus.status)
            {
            case PFK.Chat.LoginStatusValue.LOGIN_ACCEPT:
                data.setStatus('LOGGED IN','white');
                data.id = stc.loginStatus.id;
                if (stc.loginStatus.token)
                {
                    data.token = stc.loginStatus.token;
                    data.savePersistent();
                    document.cookie="CHATAUTHTOKEN=" + stc.loginStatus.token;
                }
//                location.reload(true);
                break;

            case PFK.Chat.LoginStatusValue.REGISTER_INVALID_USERNAME:
                $rootScope.$broadcast(
                    'registerFailure',
                    "INVALID USERNAME (only letters and numbers please)");
                break;

            case PFK.Chat.LoginStatusValue.REGISTER_INVALID_PASSWORD:
                $rootScope.$broadcast(
                    'registerFailure',
                    "INVALID PASSWORD (only letters and numbers please)");
                break;

            case PFK.Chat.LoginStatusValue.REGISTER_DUPLICATE_USERNAME:
                $rootScope.$broadcast(
                    'registerFailure',
                    "USERNAME ALREADY IN USE");
                break;

            case PFK.Chat.LoginStatusValue.REGISTER_ACCEPT:
                data.token = stc.loginStatus.token;
                data.id = stc.loginStatus.id;
                data.savePersistent();
//                location.replace('#/chat.view');
                break;

            default:
                data.setStatus('LOGIN REJECTED','red');
                data.token = "";
                data.savePersistent();
                break;
            }
        };

    ret.handlers[PFK.Chat.ServerToClientType.STC_USER_LIST] =
        function(stc) {
            var userList = stc.userlist.users;

            // mark all entries as not seen;
            // if any are still not seen at the end,
            // delete them.
            data.userList.forEach(function(u) {
                u.seen = false;
            });

            // for each entry we received,
            // search user list for match.
            // if found, update. if not, add.
            userList.forEach(function(u) {
                var ind = data.userList.findId(u.id);
                if (ind == -1)
                {
                    data.userList.push(
                        { name : u.username,
                          typing : u.typing,
                          idle : u.idle,
                          unread : u.unread,
                          seen : true,
                          id : u.id,
                          pchatOpen : false,
                          plusopacity : "0" });
                }
                else
                {
                    var ent = data.userList[ind];
                    ent.typing = u.typing;
                    ent.idle = u.idle;
                    ent.unread = u.unread;
                    ent.seen = true;
                }
            });

            // now look for unseen entries and delete them.
            for (var ind = 0; ind < data.userList.length; ind++)
            {
                var u = data.userList[ind];
                if (u.seen == false)
                {
                    data.userList.splice(ind--,1);

                    data.personalChats.forEach(function(c) {
                        if (c.id == u.id)
                            c.status = false;
                    });
                }
            }
        };

    ret.handlers[PFK.Chat.ServerToClientType.STC_IM_MESSAGE] =
        function(stc) {
            data.postmsg(stc.im.username, stc.im.msg, false);
        };

    ret.handlers[PFK.Chat.ServerToClientType.STC_USER_STATUS] =
        function(stc) {
            var status ="";
            switch (stc.userstatus.status)
            {
	    case PFK.Chat.UserStatusValue.USER_LOGGED_IN:
                status = "__logged in";
	        break;
	    case PFK.Chat.UserStatusValue.USER_LOGGED_OUT:
                status = "__logged out";
	        break;
	    default:
	        status = "__did something bad";
            }
            data.postmsg(stc.userstatus.username, status, true);
        };

    ret.handlers[PFK.Chat.ServerToClientType.STC_PONG] =
        function(stc) {
            var ts = Date.now();
            var delay = ts-lastPingSent;
            $rootScope.$broadcast('roundTripDelay', delay);
        };

    ret.makesocket = function() {
        if (ret.newsocket)
            return;
        ret.newsocket = new WebSocket(ret.wsuri);
        ret.newsocket.binaryType = "arraybuffer";
        ret.newsocket.onopen = function() {
            var cts = new PFK.Chat.ClientToServer;
            cts.type = PFK.Chat.ClientToServerType.CTS_PROTOVERSION;
            cts.protoversion = new PFK.Chat.ProtoVersion;
            cts.protoversion.version = PFK.Chat.CurrentProtoVersion;
            ret.newsocket.send(cts.toArrayBuffer());
        };
        ret.newsocket.onclose = function() {
            if (ret.newsocket)
                ret.newsocket.close();
            ret.socket = null;
            ret.newsocket = null;
            $rootScope.$apply(data.setStatus('DISCONNECTED','red'));
        };
        ret.newsocket.onmessage = function(msg){
            var stc = PFK.Chat.ServerToClient.decode(msg.data);
            if (stc.type in ret.handlers)
                $rootScope.$apply(function() {
                    ret.handlers[stc.type](stc);
                });
        };
    };

    $rootScope.$on('send_IM', function(scope,msg) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_IM_MESSAGE;
        cts.im = new PFK.Chat.IM_Message;
        cts.im.msg = msg;
        if (ret.socket)
            ret.socket.send(cts.toArrayBuffer());
    });

    $rootScope.$on('sendTypingInd', function(scope,state) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_TYPING_IND;
        cts.typing = new PFK.Chat.TypingInd;
        cts.typing.state = state;
        if (ret.socket)
            ret.socket.send(cts.toArrayBuffer());
    });

    $rootScope.$on('sendLogin', function(scope,username,password) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_LOGIN;
        cts.login = new PFK.Chat.Login;
        cts.login.username = username;
        cts.login.password = password;
        if (ret.socket)
            ret.socket.send(cts.toArrayBuffer());
    });

    $rootScope.$on('sendRegister', function(scope,username,password) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_REGISTER;
        cts.login = new PFK.Chat.Login;
        cts.login.username = username;
        cts.login.password = password;
        if (ret.socket)
            ret.socket.send(cts.toArrayBuffer());
    });

    $rootScope.$on('sendLogout', function(scope) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_LOGOUT;
        if (ret.socket)
            ret.socket.send(cts.toArrayBuffer());
        data.token = "";
        data.id = -1;
        data.setStatus('CONNECTED','white');
    });

    function sendPing(forced) {
        if (ret.socket)
        {
            var cts = new PFK.Chat.ClientToServer;
            cts.type = PFK.Chat.ClientToServerType.CTS_PING;
            cts.ping = new PFK.Chat.Ping;
            cts.ping.idle = data.idleTime;
            cts.ping.unread = data.unreadCount;
            cts.ping.forced = forced;
            ret.socket.send(cts.toArrayBuffer());
            lastPingSent = Date.now();
        }
    }

    $rootScope.$on('notIdle', function() {
        sendPing(true);
    });

    window.setTimeout(function() {
        ret.makesocket();
        window.setInterval(function() {
            if (ret.socket == null)
            {
                $rootScope.$apply(data.setStatus('TRYING','yellow'));
                ret.makesocket();
            }
            else
            {
                sendPing(false);
            }
        }, 10000);
    }, 100);

    return ret;
}

/*
  Local Variables:
  mode: javascript
  indent-tabs-mode: nil
  tab-width: 8
  eval: (add-hook 'write-file-hooks 'time-stamp)
  time-stamp-line-limit: 5
  time-stamp-start: "modified:\""
  time-stamp-format: "%:y/%02m/%02d  %02H:%02M:%02S\""
  time-stamp-end: "$"
  End:
*/
