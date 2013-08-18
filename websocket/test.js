var get = function(id) {
    return document.getElementById( id );
}
var username = get( "username" ).value;
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
    socket.onopen = function() {
	message('socket opened');
	var loginMsg = new PFK.Chat.ClientToServer();
	loginMsg.type = PFK.Chat.ClientToServer.ClientToServerType.LOGIN;
	loginMsg.login = new PFK.Chat.Username;
	loginMsg.login.username = username;
	var serialized = new PROTO.Base64Stream;
	loginMsg.SerializeToStream(serialized);
	socket.send(serialized.getString());
    }
    socket.onclose = function() {
	message('socket closed');
	socket = null;
    }
    socket.onmessage = function(msg){
	var b64str = new PROTO.Base64Stream(msg.data);
	var srvToClient = new PFK.Chat.ServerToClient();
	srvToClient.ParseFromStream(b64str);
	switch (srvToClient.type)
	{
	case PFK.Chat.ServerToClient.ServerToClientType.USER_LIST:
	    break;
	case PFK.Chat.ServerToClient.ServerToClientType.USER_STATUS:
	    break;
	case PFK.Chat.ServerToClient.ServerToClientType.LOGIN_NOTIFICATION:
	    message("user " + 
		    srvToClient.notification.username +
		    " has logged in");
	    break;
	case PFK.Chat.ServerToClient.ServerToClientType.LOGOUT_NOTIFICATION:
	    message("user " + 
		    srvToClient.notification.username +
		    " has logged out");
	    break;
	case PFK.Chat.ServerToClient.ServerToClientType.CHANGE_USERNAME:
	    message("username changed " +
		    srvToClient.changeUsername.oldusername +
		    " to " +
		    srvToClient.changeUsername.newusername);
	    break;
	case PFK.Chat.ServerToClient.ServerToClientType.IM_MESSAGE:
	    message(srvToClient.imMessage.username +
		    ":" +
		    srvToClient.imMessage.msg);
	    break;
	}
    }
}
makesocket();
//window.setInterval(
//    function(){
//	if (socket)
//	    socket.send('__PINGPONG');
//	else {
//	    message("trying to reconnect...");
//	    makesocket();
//	}
//    }, 5000);
var sendMessage = function() {
    var txtbox = get( "entry" );
    var im = new PFK.Chat.ClientToServer();
    im.type = PFK.Chat.ClientToServer.ClientToServerType.IM_MESSAGE;
    im.imMessage = new PFK.Chat.IM_Message;
    im.imMessage.username = username;
    im.imMessage.msg = txtbox.value;
    var imserial = new PROTO.Base64Stream;
    im.SerializeToStream(imserial);
    console.log("sending message: " + imserial.getString());
    socket.send(imserial.getString());
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
	var chgMsg = new PFK.Chat.ClientToServer();
	chgMsg.type = 
	    PFK.Chat.ClientToServer.ClientToServerType.CHANGE_USERNAME;
	chgMsg.changeUsername = new PFK.Chat.NewUsername;
	chgMsg.changeUsername.oldusername = username;
	chgMsg.changeUsername.newusername = newusername;
	var imserial = new PROTO.Base64Stream;
	chgMsg.SerializeToStream(imserial);
	socket.send(imserial.getString());
    }
    username = newusername;
}
