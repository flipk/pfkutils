
var get = function(id) {
    return document.getElementById( id );
}

var username = get( "username" ).value;

var pfk_chat_proto = dcodeIO.ProtoBuf.protoFromFile( "pfkchat.proto" );
var pfk_chat = pfk_chat_proto.build("PFK.Chat");
var stc_decoder = pfk_chat_proto.build("PFK.Chat.ServerToClient");

var message = function(msg){
    var div = get( "messages" );
    div.innerHTML = div.innerHTML + msg + '\n';
    div.scrollTop = div.scrollHeight;
}

var myuri = window.location;
var newuri = "ws://";
var colon = myuri.host.indexOf(":");
if (colon == -1)
    newuri += myuri.host;
else
    newuri += myuri.host.substring(0,colon);
newuri += ":1081/websocket/thing";
message('opening websocket to uri : ' + newuri);

var socket = null;

var makesocket = function () {

    socket = new WebSocket(newuri);
    socket.binaryType = "arraybuffer";

    socket.onopen = function() {
        message('socket opened');
        var loginMsg = new pfk_chat.ClientToServer;
        loginMsg.type = pfk_chat.ClientToServerType.CTS_LOGIN;
        loginMsg.login = new pfk_chat.Username;
        loginMsg.login.username = username;
        socket.send(loginMsg.toArrayBuffer());
    }

    socket.onclose = function() {
        message('socket closed');
        socket = null;
    }

    socket.onmessage = function(msg){

        var stcmsg = stc_decoder.decode(msg.data);

        switch (stcmsg.type)
        {
        case pfk_chat.ServerToClientType.STC_USER_LIST:
            message("users currently logged in:");
            for (var ind = 0; ind < stcmsg.userList.usernames.length; ind++)
                message("-->" + stcmsg.userList.usernames[ind]);
            break;

        case pfk_chat.ServerToClientType.STC_USER_STATUS:
            // not handled
            break;

        case pfk_chat.ServerToClientType.STC_LOGIN_NOTIFICATION:
            message("user " + 
                    stcmsg.notification.username +
                    " has logged in");
            break;

        case pfk_chat.ServerToClientType.STC_LOGOUT_NOTIFICATION:
            message("user " + 
                    stcmsg.notification.username +
                    " has logged out");
            break;

        case pfk_chat.ServerToClientType.STC_CHANGE_USERNAME:
            message("username changed " +
                    stcmsg.changeUsername.oldusername +
                    " to " +
                    stcmsg.changeUsername.newusername);
            break;

        case pfk_chat.ServerToClientType.STC_IM_MESSAGE:
            message(stcmsg.imMessage.username +
                    ":" +
                    stcmsg.imMessage.msg);
            break;

        case pfk_chat.ServerToClientType.STC_PONG:
            console.log("got PONG");
            break;
        }
    }
}

makesocket();

window.setInterval(
    function(){
        if (socket)
        {
            var ping = new pfk_chat.ClientToServer();
            ping.type = pfk_chat.ClientToServerType.CTS_PING;
            socket.send(ping.toArrayBuffer());
        } else {
            message("trying to reconnect...");
            makesocket();
        }
    }, 10000);

var sendMessage = function() {
    var txtbox = get( "entry" );
    var im = new pfk_chat.ClientToServer;
    im.type = pfk_chat.ClientToServerType.CTS_IM_MESSAGE;
    im.imMessage = new pfk_chat.IM_Message;
    im.imMessage.username = username;
    im.imMessage.msg = txtbox.value;
    socket.send(im.toArrayBuffer());
    txtbox.value = "";
}

get( "submit" ).onclick = sendMessage;

get( "entry" ).onkeypress = function(event) {
    if (event.keyCode == '13')
        sendMessage();
}

get( "clear" ).onclick = function() {
    get( "messages" ).innerHTML = "";
}

get( "username" ).onblur = function() {
    var newusername = get("username").value;
    if (socket)
    {
        var chgMsg = new pfk_chat.ClientToServer;
        chgMsg.type = pfk_chat.ClientToServerType.CTS_CHANGE_USERNAME;
        chgMsg.changeUsername = new pfk_chat.NewUsername;
        chgMsg.changeUsername.oldusername = username;
        chgMsg.changeUsername.newusername = newusername;
        socket.send(chgMsg.toArrayBuffer());
    }
    username = newusername;
}

// Local Variables:
// mode: Javascript
// indent-tabs-mode: nil
// tab-width: 4
// End:
