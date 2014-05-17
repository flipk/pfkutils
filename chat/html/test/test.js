
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

$("#testButton").click( function() {

    console.log("issuing thingy request");

    $.ajax("/cgi-bin/thingy.cgi", 
	   {
	       data : "ONE=fart%2bknocker&TWO=whatsitstuff&THREE=stuff",
	       type : "GET",
	       complete:function(jqxhr, status) {
		   console.log("ajax completed with status " + status);
	       },
	       success:function(data,status,jqxhr) {
		   console.log("got response: ", data);
	       }
	   }
	  );
});
