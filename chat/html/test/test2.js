
var testDiv = document.getElementById("testDiv");

var myuri = window.location;
var newuri = "ws://";
var colon = myuri.host.indexOf(":");
if (colon == -1)
    newuri += myuri.host;
else
    newuri += myuri.host.substring(0,colon);
newuri += "/websocket/test";

var wsurl = newuri;
var cgiurl = "/cgi/test.cgi";

var socket = new WebAppClient(wsurl, cgiurl,
			      PFK.TestMsgs.Response_m,
			      myMsgHandler, myOpenHandler, myCloseHandler);

function myOpenHandler() {
    console.log("wac opened");
}

function myCloseHandler() {
    console.log("wac closed");
}

function myMsgHandler(responseMsg) {
    console.log("decoded reponse: ", responseMsg);
    if (responseMsg.type ==
	PFK.TestMsgs.ResponseType.RESPONSE_ADD)
    {
	var str = "the sum is " + responseMsg.add.sum + "<br>";
	testDiv.innerHTML += str;
    }
    if (responseMsg.add.sum == 0)
    {
	socket.close();
    }
}

$("#do_add").click( function() {
    var cmdMsg = new PFK.TestMsgs.Command_m;
    cmdMsg.type = PFK.TestMsgs.CommandType.COMMAND_ADD;
    cmdMsg.add = new PFK.TestMsgs.CommandAdd_m;
    cmdMsg.add.a = parseInt($("#value_a").val());
    cmdMsg.add.b = parseInt($("#value_b").val());
    socket.send(cmdMsg);
    console.log('performing add, sent request:', cmdMsg);
});
