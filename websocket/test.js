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
	socket.send('user ' + username + ' logged in');
	socket.send('__USERLOGIN:' + username);
    }
    socket.onclose = function() {
	message('socket closed');
	socket = null;
    }
    socket.onmessage = function(msg){
	message(msg.data);
    }
}
makesocket();
window.setInterval(
    function(){
	if (socket)
	    socket.send('__PINGPONG');
	else {
	    message("trying to reconnect...");
	    makesocket();
	}
    }, 10000);
get( "submit" ).onclick = function() {
    var txtbox = get( "entry" );
    var str = txtbox.value;
    socket.send(username + ':' + str);
    txtbox.value = "";
}
get( "entry" ).onkeypress = function(event) {
    if (event.keyCode == '13')
    {
	var txtbox = get( "entry" );
	var str = txtbox.value;
	socket.send(username + ':' + str);
	txtbox.value = "";
    }
}
get( "clear" ).onclick = function() {
    get( "messages" ).innerHTML = "";
}
get( "username" ).onblur = function() {
    var newusername = get("username").value;
    socket.send('username changed from ' + username + ' to ' + newusername);
    username = newusername;
    socket.send('__USERLOGIN:' + username);
}
