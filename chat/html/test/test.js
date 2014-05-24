
function getCookie(name) {
    var result = RegExp("(^" +name+ "|; " +name+ ")=(.*?)(;|$)").exec(document.cookie);
    return (result === null) ? null : result[2];
}

var cookieValue = getCookie("cgiwhatsit");
if (cookieValue)
{
    $("#INPUTFORMCOOKIE").attr("value",cookieValue);
    console.log('setting form cookie to ' + cookieValue);
}
else
{
    console.log('no cookie is found');
}

var testDiv = document.getElementById("testDiv");

var cgiuri = "/cgi/test.cgi";

var xreq = null;

var sendMessage = function (data) {
    var config = {
               dataType : 'text',
               data : data, // xxx  base64
               type : "POST",
               complete : function(jqxhr, status) {
		   console.log("POST completed with status " + status);
               },
	   }
    $.ajax(cgiuri, config);
}

$("#sendmessage").click( function () {
    sendMessage( "abcdefg=" );
});

(function getNextMsg () {
    console.log("starting new GET ajax");
    $.ajax( {
	url : cgiuri,
	success : function(data) {
	    console.log("GET success callback called with data:", data);
	    // xxx base64
	},
	dataType : 'text',
	data : 'GETMSG',
	type : 'GET',
	complete : function() {
	    console.log("GET complete callback sleeping before restarting");
	    setTimeout(getNextMsg, 250);
	},
	timeout : 30000
    });
})();
