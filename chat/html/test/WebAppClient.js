
// the argument to onMessage is a protobuf based on a msg type
function WebAppClient(wsurl, cgiurl, pbRcvMsg,
		      onMessage, onOpen, onClose) {
    this.CGIMODE = 1;
    this.WSMODE = 2;
    if (this.isWebSocketSupported())
    {
	// start websocket 
	this.mode = this.WSMODE;
	this.wsurl = wsurl;
	this.wsock = new WebSocket(wsurl);
	this.wsock.binaryType = 'arraybuffer';
	this.wsock.onopen = function() {
	    onOpen();
	};
	this.wsock.onclose = function() {
	    onClose();
	};
	this.wsock.onmessage = function(msg) {
	    onMessage(pbRcvMsg.decode(msg.data));
	};
    }
    else
    {
	// start fastcgi
	this.mode = this.CGIMODE;
	this.CGIcontinue = true;
	this.restartDelay = 250;
	this.getTimeout = 30000;
	this.cgiurl = cgiurl;
	this.CGIgetNextMsg();
	this.onClose = onClose;
	this.onMessage = onMessage;
	this.pbRcvMsg = pbRcvMsg;
	onOpen();
    }
}

WebAppClient.prototype.isWebSocketSupported = function () {
    // there could be more to this, like detecting the
    // old hixie-76 websocket (which we dont want to use)
    // but i dont yet know how to detect that.
    if ('WebSocket' in window)
	return true;
    return false;
}

WebAppClient.prototype.CGIgetNextMsg = function () {
    console.log("starting new GET ajax");
    var wac = this;
    $.ajax( {
	url : wac.cgiurl,
	success : function(data) {
	    if (data.length == 0)
		console.log("GET success got zero-length data, server "+
			    "may have set cookie");
	    else
	    {
		if (wac.CGIcontinue)
		{
		    console.log("GET success callback called with data:",
				data);
		    wac.onMessage(wac.pbRcvMsg.decode64(data));
		} else {
		    console.log("ignoring new msg that just arrived " +
				"because closed");
		}
	    }
	},
	dataType : 'text',
	type : 'GET',
	complete : function() {
	    if (wac.CGIcontinue)
	    {
		console.log("GET complete callback sleeping " +
			    "before restarting");
		setTimeout(function() { wac.CGIgetNextMsg(); },
			   wac.restartDelay);
	    } else {
		console.log("terminating WebAppClient object");
	    }
	},
	timeout : wac.getTimeout
    });
}

WebAppClient.prototype.close = function() {
    if (this.mode == this.CGIMODE)
    {
	this.CGIcontinue = false;
	this.onClose();
    }
    else if (this.mode == this.WSMODE)
    {
	// onClose will be called through wsock's callbacks above
	this.wsock.close();
	this.wsock = null;
    }
}

// msg is a protobuf obj
WebAppClient.prototype.send = function(msg) {
    if (this.mode == this.CGIMODE)
    {
	var config = {
            dataType : 'text',
            data : msg.encode64(),
            type : "POST",
            complete : function(jqxhr, status) {
		console.log("POST completed with status " + status);
            },
	}
	$.ajax(this.cgiurl, config);
    }
    else if (this.mode == this.WSMODE)
    {
	this.wsock.send(msg.toArrayBuffer());
    }
}

