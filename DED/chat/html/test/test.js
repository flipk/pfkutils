
var testDiv = document.getElementById("testDiv");

var cgiuri = "/cgi/test.cgi";

var xreq = null;

var sendMessage = function (data) {
    var config = {
               dataType : 'text',
               data : data,
               type : "POST",
               complete : function(jqxhr, status) {
		   console.log("POST completed with status " + status);
               },
	   }
    $.ajax(cgiuri, config);
}

$("#do_add").click( function() {
    var cmdMsg = new PFK.TestMsgs.Command_m;
    cmdMsg.type = PFK.TestMsgs.CommandType.COMMAND_ADD;
    cmdMsg.add = new PFK.TestMsgs.CommandAdd_m;
    cmdMsg.add.a = parseInt($("#value_a").val());
    cmdMsg.add.b = parseInt($("#value_b").val());

    var b64msg = cmdMsg.encode64();

    sendMessage(b64msg);

    console.log('performing add, sent request:', b64msg);
});


(function getNextMsg () {
    console.log("starting new GET ajax");
    $.ajax( {
	url : cgiuri,
	success : function(data) {
	    if (data.length == 0)
		console.log("GET success got zero-length data, server "+
			    "may have set cookie");
	    else
	    {
		console.log("GET success callback called with data:", data);
		var responseMsg = PFK.TestMsgs.Response_m.decode64(data);
		console.log("decoded reponse: ", responseMsg);
		if (responseMsg.type ==
		    PFK.TestMsgs.ResponseType.RESPONSE_ADD)
		{
		    var str = "the sum is " + responseMsg.add.sum + "<br>";
		    testDiv.innerHTML += str;
		}
	    }
	},
	dataType : 'text',
	type : 'GET',
	complete : function() {
	    console.log("GET complete callback sleeping before restarting");
	    setTimeout(getNextMsg, 250);
	},
	timeout : 30000
    });
})();
