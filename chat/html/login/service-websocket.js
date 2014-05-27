
var webSocketServiceLastModified = {
    file:"service-websocket.js", modified:"2014/05/26  23:18:10"
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
    ret.cgiuri = "/cgi/pfkchat.cgi";

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
                        ret.socket.send(cts);
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
                console.log('logging in');
                if (stc.loginStatus.token)
                {
                    data.token = stc.loginStatus.token;
                    data.savePersistent();
                    document.cookie="CHATAUTHTOKEN=" + stc.loginStatus.token;
                    console.log('set cookie ' + stc.loginStatus.token);
                }
                location.reload(true);
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
                if (stc.loginStatus.token)
                {
                    data.token = stc.loginStatus.token;
                    data.savePersistent();
                    document.cookie="key=" + stc.loginStatus.token;
                    console.log('set cookie ' + stc.loginStatus.token);
                }
                location.reload(true);
                break;

            default:
                data.setStatus('LOGIN REJECTED','red');
                data.token = "";
                data.savePersistent();
                location.reload(true);
                break;
            }
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
        ret.webappOnmessage = function(stc) {
console.log("got new wac msg : ", stc);
            if (stc.type in ret.handlers)
                $rootScope.$apply(function() {
                    ret.handlers[stc.type](stc);
                });
        };
        ret.webappOnOpen = function() {
            var cts = new PFK.Chat.ClientToServer;
            cts.type = PFK.Chat.ClientToServerType.CTS_PROTOVERSION;
            cts.protoversion = new PFK.Chat.ProtoVersion;
            cts.protoversion.version = PFK.Chat.CurrentProtoVersion;
            ret.newsocket.send(cts);
        };
        ret.webappOnClose = function() {
            if (ret.newsocket)
                ret.newsocket.close();
            ret.socket = null;
            ret.newsocket = null;
            $rootScope.$apply(data.setStatus('DISCONNECTED','red'));
        };
        ret.newsocket = new WebAppClient(ret.wsuri,
                                         ret.cgiuri,
                                         PFK.Chat.ServerToClient,
                                         ret.webappOnmessage,
                                         ret.webappOnOpen,
                                         ret.webappOnClose);
    };

    $rootScope.$on('sendLogin', function(scope,username,password) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_LOGIN;
        cts.login = new PFK.Chat.Login;
        cts.login.username = username;
        cts.login.password = password;
        if (ret.socket)
            ret.socket.send(cts);
    });

    $rootScope.$on('sendRegister', function(scope,username,password) {
        var cts = new PFK.Chat.ClientToServer;
        cts.type = PFK.Chat.ClientToServerType.CTS_REGISTER;
        cts.login = new PFK.Chat.Login;
        cts.login.username = username;
        cts.login.password = password;
        if (ret.socket)
            ret.socket.send(cts);
    });

    function sendPing() {
        if (ret.socket)
        {
            var cts = new PFK.Chat.ClientToServer;
            cts.type = PFK.Chat.ClientToServerType.CTS_PING;
            cts.ping = new PFK.Chat.Ping;
            ret.socket.send(cts);
            lastPingSent = Date.now();
        }
    }

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
                sendPing();
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
