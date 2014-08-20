/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

/**
 * \file WebAppServer.h
 * \brief All definitions relevant to a user of libWebAppServer
 */

#ifndef __WEBAPPSERVER_H_
#define __WEBAPPSERVER_H_

#include <unistd.h>
#include <list>
#include <string>

#include "WebAppMessage.h"

namespace WebAppServer {

class WebServerConnectionBase;
class WebAppConnectionData;
/** base class for a user application connection. these are created
 * as browser clients connect, and are deleted when the browser goes
 * away. these objects are created by the user's supplied 
 * \ref WebAppConnectionCallback -derived object. */
class WebAppConnection {
    friend class WebServerConnectionBase;
public:
    /** user may implement a constructor for the derived object 
     * which will initialize the application's context, if desired.
     * this method may optionally take arguments, which
     * WebAppConnectionCallback::newConnection must understand.
     * Do not send messages from this constructor -- the back pointer
     * to the ConnectionData is not set up yet. you may send messages
     * from onConnect, though. */
    WebAppConnection(void);
    /** user may have a destructor for the derived object which will
     * clean up the application's context for this connection, if 
     * desired. */
    virtual ~WebAppConnection(void);
    /** handle connection */
    virtual void onConnect(void) { }
    /** handle disconnection */
    virtual void onDisconnect(void) { }
    /** handler for incoming message.  when the application in the browser
     * has sent a message to us, libWebAppServer will call this method
     * in your object. note that upon return, the message is freed, thus
     * the application must not retain references or pointers to this
     * object.
     * \param m the message just received.
     * \return  the application handler should return true if the connection
     *      should remain open, or false if libWebAppServer should close it.
     */
    virtual bool onMessage(const WebAppMessage &m) = 0;
    /** periodic poll for audit/mainenance purposes. in 
     * \ref WebAppServerConfig::addWebsocket and \ref
     * WebAppServerConfig::addFastCGI, the user may specify a
     * pollInterval. if specified, this method will be called on that
     * interval. the user's handler for this may do anything desired,
     * such as implementing timeouts or other maintenance activities.
     * \return  the application handler should return true if the connection
     *      should remain open, or false if libWebAppServer should close it.
     */
    virtual bool doPoll(void) = 0;
    /** send a message to the browser.  the application code may call
     * this to send a message to the browser.  this is thread-safe so any
     * thread in the application may call it without worry of interference
     * with onMessage or doPoll.
     * \param  m  the message to send. */
    void sendMessage(const WebAppMessage &m);
    WebAppConnectionData * connData;
};

/** base class for user new-connection callback. when a new browser
 * connection is estalished, libWebAppServer needs to know what 
 * \ref WebAppConnection -derived object to create. The user should supply
 * one of these for each \ref WebAppServerConfig record. */
class WebAppConnectionCallback {
public:
    virtual ~WebAppConnectionCallback(void) { /*placeholder*/ }
    /** user must provide an implementation of this function. when a new
     * browser connection is formed, the user's implementation of this
     * function must create a \ref WebAppConnectionCallback -derived object
     * to handle that connection. the user's implementation of this function
     * may optionally pass arguments to the newly constructed object,
     * to provide application context if required. */
    virtual WebAppConnection * newConnection(void) = 0;
};

struct WebAppServerConfigRecord;
/** configuration for a web server. this must be constructed and
 * initialized with services to support before a WebAppServer may 
 * be started. 
 * \note This object must live on for the duration of the WebAppServer.
 *      WebAppServer takes a reference, so don't free it. */
class WebAppServerConfig {
    std::list<WebAppServerConfigRecord*>  records;
    void clear(void);
public:
    WebAppServerConfig(void);
    ~WebAppServerConfig(void);
    /** add a websocket application.  
     * \param port the TCP port to listen for websocket connections. 
     *         this may be connected directly by a browser, or via 
     *         a proxy connection from the web server process.
     * \param route the URI path to this application, e.g.
     *        "/websocket/MYPROGRAM".  multiple websocket applications
     *        may be registered on the same TCP port as long as they
     *        have different routes.  this translates into a URL such
     *        as  "ws://ip_or_host:port/websocket/MYPROGRAM".
     * \param cb  a pointer to a user's WebAppConnectionCallback -derived
     *       object, which will be used to construct new user
     *       WebAppConnection -derived objects on new connections.
     * \param pollInterval  a number of milliseconds between calls to
     *       the user's WebAppConnection::doPoll method.  a value of -1
     *       means do not call doPoll at all.  this is the default value
     *       if this parameter is not specified.
     * \note the same TCP port cannot host both websocket and fastcgi 
     *       connections. once a port has been specified as websocket,
     *       then all future services added with that port must also be
     *       websocket.
     * \note this service does not support "wss://" protocol (SSL). if
     *       you wish to use SSL, you will need to proxy to this service
     *       using a web browser that supports secure websocket proxying
     *       (such as NGINX).
     */
    void addWebsocket(int port, const std::string route,
                      WebAppConnectionCallback *cb,
                      int pollInterval = -1);
    /** add a FastCGI application.
     * \param port the TCP port to listen for AJAX-style connections. 
     *         this is ONLY supported using a web server proxy which
     *         supports the FastCGI standard (as the name implies).
     * \param route the URI path to this application, e.g.
     *        "/cgi/MYPROGRAM.cgi".  multiple AJAX-style applications
     *        may be registered on the same TCP port as long as they
     *        have different routes.  this translates into a URL such
     *        as  "http://ip_or_host:port/cgi/MYPROGRAM.cgi".
     * \param cb  a pointer to a user's WebAppConnectionCallback -derived
     *       object, which will be used to construct new user
     *       WebAppConnection -derived objects on new connections.
     * \param pollInterval  a number of milliseconds between calls to
     *       the user's WebAppConnection::doPoll method.  a value of -1
     *       means do not call doPoll at all.  this is the default value
     *       if this parameter is not specified.
     * \note the same TCP port cannot host both websocket and FastCGI 
     *       connections. once a port has been specified as FastCGI,
     *       then all future services added with that port must also be
     *       FastCGI.
     * \note this service does not support "https://" protocol (SSL). if
     *       you wish to use SSL, you will need to proxy to this service
     *       using a web browser that supports secure proxying
     *       (such as NGINX).
     */
    void addFastCGI(int port, const std::string route,
                    WebAppConnectionCallback *cb,
                    int pollInterval = -1);
    std::list<WebAppServerConfigRecord*>::const_iterator getBeginC(void) const {
        return records.begin();
    }
    std::list<WebAppServerConfigRecord*>::const_iterator getEndC(void) const {
        return records.end();
    }
};


class serverPorts;
/** a web server. create a WebAppServerConfig and then call
 * start(config). your main thread should then call sleep in a 
 * while-loop or something to keep the process alive. */
class WebAppServer {
    const WebAppServerConfig *config;
    serverPorts * ports;
public:
    WebAppServer(void);
    ~WebAppServer(void);
    /** start a web server. this can only be called once.
     * \param config  the websocket/fastcgi configuration to use.
     * \return false if there was an error in starting (such as a
     *     problem with the config object, or TCP ports already in
     *     use) or true if the web server is now alive and listening
     *     for requests.
     * \note WebAppServer takes a reference to your WebAppServerConfig
     *    object, so don't free it or make it a function-local variable
     *    which goes away. Bad Mojo. */
    bool start(const WebAppServerConfig *config);
    /** stop the web server.  all ports are closed and all open
     * connection objects are destroyed.  or you could just exit main. */
    void stop(void);
};

/** \mainpage WebAppServer

\section Overview Overview

This is a server for handling WebSocket or FastCGI (Ajax long-polling) 
backend of a web application.

For a quick start, look at the \ref SampleCode and \ref SampleXHRCode.

For the brave, there is also \ref SampleWebAppClient.

\section InterestingDataStructures Interesting Data Structures

Here are some interesting data structures you should look at (referenced
in the sample code):

<ul>
<li> \ref WebAppMessage
<li> \ref WebAppConnection
<li> \ref WebAppConnectionCallback
<li> \ref WebAppServerConfig
<li> \ref WebAppServer::WebAppServer
</ul>

\bug A user's WebAppConnection needs a way to deal with cookies. Some
kind of method interface needs to be added whereby an application can
add or delete cookies, and have cookies presented by the browser 
presented to the application (a virtual callback perhaps).

\bug The FastCGI interface needs to be able to pass more than one
message thru per GET or POST request, perhaps by chaining multiple
base64-encoded messages back to back with a size-delimiter between
them. The reason for this is performance: any application which needs
to pass a significant amount of data needs to be able to optimize its
use of the link (think uploads and downloads).

\section HowTo How To

Everything begins with a WebAppServer.  In order to use WebAppServer,
it must be provided with a configuration, in the form of a
WebAppServerConfig. 
\note The WebAppServerConfig object you supply must exist at least as
      long as the WebAppServer exists.  The WebAppServer grabs a pointer
      to your WebAppServerConfig object and references it while running.

The WebAppServerConfig object must be provided a set of configurations
for applications to serve.  This is done via the
WebAppServerConfig::addWebsocket and WebAppServerConfig::addFastCGI
methods.

These methods must be supplied a TCP port number.  For WebSockets, it
is assumed this the port number specified in either the web server
config (if e.g. NGINX is using a WebSocket proxy) or provided directly
in the JavaScript client in the browser.  For FastCGI, it is the port
number specified in the web server config.  Multiple applications may
be attached to the same TCP port number, however their types cannot be
mixed (i.e. a TCP port must be either all FastCGI or all WebSocket).

Multiple applications on the same port must be distinguished using
different routes. (A 'route' in this context is referring to the
portion of the URL after the hostname, also known as the document
URI.)

The user must also supply a WebAppConnectionCallback for each
application to be served through the server. This object has only one
responsibility: to construct a new application object (a user's object
derived from the WebAppConnection base class) when a new unique
visitor connects to the server.

\section DefnUniqueVisitor Definition of Unique Visitor

<ul>
<li> WebSockets : A 'new unique visitor' for WebSockets means a 'new
      WebSocket' connection from a browser. The user's
      WebAppConnection -derived object exists as long as that
      JavaScript WebSocket object exists in the browser, and is
      deleted when that WebSocket is closed.
</ul>
<ul>
<li> FastCGI : A 'new unique visitor' for FastCGI means the first XHR
      (XMLHttpRequest) (or in jQuery, "$.ajax", or in Angular,
      "$http") connection from a host with no "visitorId" cookie set
      or an unknown visitorId cookie. The user's WebAppConnection
      -derived continues to exist beyond the closure of that XHR
      connection for a period of time, to wait for the next XHR
      connection (since XHR connections will come and go for every
      message). When a new XHR connection arrives with the same
      visitorId cookie, it is attached to an existing WebAppConnection
      object if possible. The user's WebAppConnection object will be
      deleted if a period of time passes with no XHR connection
      attached to it.
</ul>

\section XHRNotes  XHR Usage notes

To use XHR with this library, there are a couple of rules. First, use
GET methods in a long-polling technique to retrieve messages (server
to browser), but do NOT use GET to send messages
browser-to-server. Second, use POST to send browser-to-server, and do
not expect the server to include any server-to-browser messages in the
response.

Here is some \ref SampleXHRCode.

\cond INTERNAL

\section Internals

If you're interested in the internals of how this library works, read
\ref InternalsPage.

\endcond

*/

/**
\cond INTERNAL

 \page ClassDiagram Class Diagram for libWebAppServer

\dot

digraph WebAppServer {

    node [shape=record, fontname=Helvetica, fontsize=10, color=red];
    edge [fontname=Helvetica, fontsize=10];
    edge [arrowhead="open", style="dashed"];

    WebAppServer  [color=black label="WebAppServer"   URL="\ref WebAppServer"];
    WebAppServerConfig [color=black label="WebAppServerConfig" URL="\ref WebAppServerConfig"];
    WebAppServerConfigRecord [label="WebAppServerConfigRecord" URL="\ref WebAppServerConfigRecord"];
    serverPorts [label="serverPorts" URL="\ref serverPorts"];
    serverPort [label="serverPort" URL="\ref serverPort"];
    WebAppServerFastCGIConfigRecord [label="WebAppServerFastCGIConfigRecord" URL="\ref WebAppServerFastCGIConfigRecord"];
    WebServerConnectionBase [label="WebServerConnectionBase" URL="\ref WebServerConnectionBase"];
    WebAppConnectionCallback [color=black label="WebAppConnectionCallback" URL="\ref WebAppConnectionCallback"];
    UserAppConnCBClass [color=blue label="User's App Connection\nCallback Class"];
    WebAppConnection [color=black label="WebAppConnection" URL="\ref WebAppConnection"];

    WebAppConnectionData [color=red label="WebAppConnectionData" URL="\ref WebAppConnectionData"];
    WebAppConnectionDataWebsocket [color=red label="WebAppConnectionDataWebsocket" URL="\ref WebAppConnectionDataWebsocket"];
    WebAppConnectionDataFastCGI [color=red label="WebAppConnectionDataFastCGI" URL="\ref WebAppConnectionDataFastCGI"];
    
    fdThreadLauncher  [color=black label="fdThreadLauncher" URL="\ref fdThreadLauncher"];
    WebFastCGIConnection [label="WebFastCGIConnection" URL="\ref WebFastCGIConnection"];
    UserAppConnClass [color=blue label="User's App\nConnection Class"];
    WebSocketConnection [label="WebSocketConnection" URL="\ref WebSocketConnection"];
    CircularReader [label="CircularReader" URL="\ref CircularReader"];
    CircularReaderSubstr [label="CircularReaderSubstr" URL="\ref CircularReaderSubstr"];

    WebAppServer -> WebAppServerConfig [label="config"];
    WebAppServer -> serverPorts [label="ports"];
    serverPorts -> serverPort  [label="map<portNum> portMap"];
    WebAppServerConfig -> WebAppServerConfigRecord [label="list<> records"];
    serverPort -> WebAppServerConfigRecord [label="list<> configs"];
    WebAppServerFastCGIConfigRecord -> WebAppServerConfigRecord  [style="solid" dir="back"];
    serverPort -> WebServerConnectionBase [label="list<> connections\ncreates"];
    WebAppServerConfigRecord -> WebAppConnectionCallback [label="cb"];
    WebAppConnectionCallback -> WebAppConnection [label="newConnection()\ncreates"];

    WebAppServerFastCGIConfigRecord -> WebAppConnection [label="map<visitorId> conns"];

    WebAppConnection -> UserAppConnClass [style="solid" dir="back"];

    WebAppConnection -> WebAppConnectionData [label="connData"];
    WebAppConnectionData -> WebAppConnectionDataWebsocket [style="solid" dir="back"];
    WebAppConnectionData -> WebAppConnectionDataFastCGI [style="solid" dir="back"];

    WebAppConnectionDataWebsocket -> WebSocketConnection [label="connBase"];
    WebAppConnectionDataFastCGI -> WebFastCGIConnection [label="waiter"];

    WebServerConnectionBase -> WebAppConnection [label="wac"];
    WebServerConnectionBase -> WebSocketConnection  [style="solid" dir="back"];
    WebServerConnectionBase -> WebFastCGIConnection  [style="solid" dir="back"];

    fdThreadLauncher -> WebServerConnectionBase  [style="solid" dir="back"];
    WebServerConnectionBase -> CircularReader [label="readbuf"];
    WebServerConnectionBase -> WebAppServerConfigRecord [label="config"];
    fdThreadLauncher -> serverPort  [style="solid" dir="back"];

    WebAppConnectionCallback -> UserAppConnCBClass [style="solid" dir="back"];

    CircularReader -> CircularReaderSubstr [style="solid" dir="back"];

    PublicBlock [color=black label="Public class"];
    PrivateBlock [label="Private class"];
    UsersBlock [color=blue label="User's class"];

    { rank = same; WebAppServerConfig serverPorts }
    { rank = same; WebAppServerConfigRecord serverPort }

}

\enddot

\endcond

*/

/** \page SampleCode Sample C++ code

sample code to set up a server:

\code

class cgiTestAppConn : public WebAppConnection {
public:
    cgiTestAppConn(void) {
        cout << "new cgi test app connection" << endl;
    }
    //virtual
    ~cgiTestAppConn(void) {
        cout << "cgi test app destroyed" << endl;
    }
    //virtual
    bool onMessage(const WebAppMessage &m) {
        cout << "got msg: " << m.buf << endl;
        return true;
    }
    //virtual
    bool doPoll(void) {
        cout << "test app doPoll" << endl;
        return true;
    }
};

class cgiTestAppCallback : public WebAppConnectionCallback {
public:
    cgiTestAppCallback(void) {
        cout << "cgiTestAppCallback constructor" << endl;
    }
    //virtual
    ~cgiTestAppCallback(void) {
        cout << "cgiTestAppCallback destructor" << endl;
    }
    //virtual
    WebAppConnection * newConnection(void) {
        return new cgiTestAppConn;
    }
};

int
main()
{
    WebAppServerConfig  serverConfig;
    WebAppServer      server;

    cgiTestAppCallback testAppCallback;

    signal( SIGPIPE, SIG_IGN );

    serverConfig.addWebsocket(1081, "/websocket/test", &testAppCallback, 1000);
    serverConfig.addFastCGI(1082, "/cgi-bin/test.cgi", &testAppCallback, 1000);

    if (server.start(&serverConfig) == false)
    {
        printf("failure to start server\n");
    }

    while (1)
        sleep(1);

    return 0;
}


\endcode

 */

/** \page SampleXHRCode Sample XHR Javascript code

HTML code:

\code
  <input type=button value="Send Message" id=sendmessage>
  <script src=/js/jquery.js> </script>
  <script src=test.js> </script>
\endcode

Javascript code:

\code

var cgiuri = "/cgi/test.cgi";

function sendMessage (data) {
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
	data : 'GETMSG', // dummy, not used
	type : 'GET',
	complete : function() {
	    console.log("GET complete callback sleeping before restarting");
	    setTimeout(getNextMsg, 250);
	},
	timeout : 30000
    });
})();

\endcode


 */

/** \page SampleWebAppClient a simple WebAppClient library

Here is a library for interfacing to libWebAppServer in a generic fashion:

\code

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

\endcode

Here is a sample app which uses the above library:

\code

var wsurl = "wss://host:port/websocket/test";
var cgiurl = "http://host:port/cgi/test.cgi";

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

\endcode

 */

/** 

\cond INTERNAL

\page InternalsPage Internal Workings

There is a nice \ref ClassDiagram that is a good place to start. Best
to open this in another tab, as the text below will make frequent
references to it.

Setting up a server involves building a WebAppServerConfig
object by adding WebAppServerConfigRecord objects to it.

Each call to WebAppServerConfig::addWebsocket and
WebAppServerConfig::addFastCGI adds a new
WebAppServerConfigRecord to the config object.  Each config record
describes one unique route.

\note FastCGI connections actually get a WebAppServerFastCGIConfigRecord
      object, which is derived from WebAppServerConfigRecord.  This is
      because FastCGI connections need to store a little extra data about
      unique visitorId cookies (WebAppServerFastCGIConfigRecord::conns).

\section OperationStartup Startup

When WebAppServer::start is called, a single serverPorts object is 
created. This object will be a container of a set of serverPort objects.
The WebAppServerConfig is examined. One serverPort is created for each
unique TCP port number found among all the config records. Each serverPort
is then given a list of all config records which apply to that TCP port.
Finally, a thread is created for each serverPort. Each thread creates a
listening TCP socket on the specified port, and waits for an incoming
connection. The fdThreadLauncher base class is used to facilitate this.

Each new inbound connection on a serverPort causes the creation of a new
object derived from WebServerConnectionBase (either WebFastCGIConnection
or WebSocketConnection, depending on the config records given to that
serverPort instance--recall that a given serverPort must contain only one
type or the other, not a mix).

Since WebServerConnectionBase is derived from fdThreadLauncher, this creation
of a new object results in a new thread as well. This thread handles all 
incoming traffic through a WebServerConnectionBase::handleSomeData virtual
method.

\section NewConn New Connection

A new connection causes fdThreadLauncher::threadEntry to leave select and
call the fdThreadLauncher::handleReadSelect virtual method, which in this
case is serverPort::handleReadSelect.  This method accepts a new connection
and then creates either a WebFastCGIConnection or WebSocketConnection 
(depending on the configuration of this serverPort).

WebFastCGIConnection and WebSocketConnection are both derived from a 
base class WebServerConnectionBase, which includes fdThreadLauncher to
handle the data motion.  Data arriving on the connection triggers
WebServerConnectionBase::handleReadSelect method, which uses a
CircularReader object to gather data fragments into messages.  When
some data has arrived, it then calls a virtual method
WebServerConnectionBase::handleSomeData which vectors to the corresponding
method in either the WebFastCGIConnection or WebSocketConnection derived
object.

MIME headers and message contents are parsed by those objects.  Both types
search for resource (URI) paths in the MIME headers.  The URI paths are
then used to search the "routes" in the various config records attached
to the serverPort.  Once a matching config record is found, the user's
WebAppConnectionCallback -derived object is used to create or locate
the correct WebAppConnection -derived object to attach to.  A pointer
to the WebAppConnection is stored in WebServerConnectionBase::wac.

\section wacOpaque WebAppConnection opaque extra data

A WebAppConnection contains another object which is opaque to the user.
WebAppConnection::connData is a pointer to either a
WebAppConnectionDataWebsocket or a WebAppConnectionDataFastCGI.

A WebAppConnectionDataWebsocket contains very little data.

A WebAppConnectionDataFastCGI helps deal with the fact that Ajax polling
connections are ephemeral with respect to a WebAppConnection, that is to
say, a WebAppConnection must survive longer than a single Ajax connection,
and WebAppConnectionDataFastCGI helps maintain that relationship.  More
on that below.

\section WebSocketConn A WebSocketConnection

When WebSocketConnection discovers a route, it follows the configs
list given to it by the serverPort to find a matching route.  When a
matching route is found, the WebAppConnectionCallback provided by the
user is used to construct a new user's WebAppConnection -derived
object. This object will live as long as this TCP connection.
WebSocket messages will be passed in both directions over this
connection for as long as this connection exists.  If the browser
closes the WebSocket object, this WebServerConnectionBase thread will
exit and the corresponding WebAppConnection will also be destroyed.

The sequence for a server-to-browser message is as follows:

<ul>
<li> The user's application (in a WebAppConnection -derived object) 
     calls WebAppConnection::sendMessage.
<li> WebAppConnection::sendMessage routes the data to the connData
     member, in this case WebAppConnectionDataWebsocket::sendMessage.
<li> since there is always a 1-to-1 mapping between a WebAppConnection
     and a WebSocketConnection, DataWebsocket has an easy job. It simply
     routes the data to WebSocketConnection::sendMessage, which encodes
     the data in a WebSocket packet, and sends it.
<li> the WebSocket class in the browser delivers it to the Javascript
     application's registered "onmessage" callback function.
</ul> 

The sequence for a browser-to-server message is as follows.

<ul>
<li> The Javascript application in the browser calls the "send" method
     on the WebSocket object.
<li> fdThreadLauncher::threadEntry returns from select and calls
     fdThreadLauncher::handleReadSelect virtual method, which is actually
     WebServerConnectionBase::handleReadSelect.
<li> That method uses a CircularReader to read data.  It then calls
     WebServerConnectionBase::handleSomeData virtual method, which is
     actually (in this case) WebSocketConnection::handleSomeData.
<li> That method has a small state machine to handle either MIME headers
     or message bodies. In this case it routes to 
     WebSocketConnection::handle_message.
<li> handle_message parses a message body, builds a WebAppMessage, and
     (using WebServerConnectionBase::wac) calls the user's virtual method
     attached through WebAppConnection::onMessage.
</ul>

\section FastCGIConn WebFastCGIConnection

FastCGI connections work differently from WebSockets. A new XHR
(XMLHttpRequest) connection is established for each message.  An XHR
connection follows the following state diagram:

\dot

digraph XHRStates {
    layout = "dot";
    overlap = "false";
    mode = "maxent";

    node [fontname=Helvetica, fontsize=10];
    edge [len=1.5];

    aNoConnection [label="No Connection"];
    aB2SMime [label="GET: Browser to\nserver MIME\nheaders"];
    aS2BMime [label="Server to\nbrowser MIME\nheaders"];
    aS2BMsg [label="Server to\nbrowser messages"];

    bNoConnection [label="No Connection"];
    bB2SMime [label="POST: Browser to\nserver MIME\nheaders"];
    bB2SMsg  [label="Browser to\nserver messages"];
    bS2BMime [label="Server to\nbrowser MIME\nheaders"];

    aNoConnection -> aB2SMime;
    aB2SMime -> aS2BMime;
    aS2BMime -> aS2BMsg;
    aS2BMsg -> aNoConnection;

    bNoConnection -> bB2SMime;
    bB2SMime -> bB2SMsg;
    bB2SMsg -> bS2BMime;
    bS2BMime -> bNoConnection;
}

\enddot

There are two types of HTTP requests that can be performed on an
XHR request: a GET, or a POST.  This library assumes a GET is for polling
for server-to-browser messages, and a POST is for browser-to-server.

(This seems to correspond to various documentation found around the web.)

A GET transaction proceeds to the final state and then waits there. If
the server has a message to send, it is sent and the connection
closed. If however the server has no message to send, the connection
freezes here and waits. It will wait for as long as the "timeout"
parameter to the AJAX configuration in the Javascript code, and then
timeout. If a message is created during that time, it will be sent and
the connection closes.  If no message is created during that time, the
connection is closed without data and a new one opened.

This means there should be, at all times, one GET connection
open. Each time a GET completes (with or without a message) a new one
should be immediately started by the Javascript application.

A POST transaction is only created by the Javascript code when a
message to send has been created.  A POST transaction may run in
parallel with a GET transaction. It will not carry any
server-to-browser messages, only browser-to-server messages.  It will
run through the above states as quickly as possible and return to the
No Connection state, waiting for the next message generated by the
Javascript code.

\note XHR connections come and go frequently, basically for every
      message sent. Thus it doesn't make sense for a WebAppConnection
      -derived object to be created and destroyed for every XHR
      connection, like it does with a WebSocketConnection.

When WebFastCGIConnection discovers a route, it follows the configs
list given by the serverPort to find a matching route.  When a matching
route is found, the WebAppServerConfigRecord is converted to its derived
WebAppServerFastCGIConfigRecord. The MIME headers for this connection 
are then examined to find if there is a visitorId cookie set. 

<ul>
<li> If no cookie is set:
    <ul>
    <li> A brand-new unique visitorId cookie is created. </li>
    <li> The user's WebAppConnectionCallback -derived object is used to
         create a new user's WebAppConnection -derived object. This object
         is inserted into the WebAppServerFastCGIConfigRecord::conns
         map, with the new cookie used as the key to the map. </li>
    <li> This new WebAppConnection is given to the WebServerConnectionBase
         for future message exchange. </li>
    </ul> </li>
<li> If a cookie is set:
    <ul> 
    <li> The WebAppServerFastCGIConfigRecord::conns map is searched for the
         visitorId cookie.
       <ul>
       <li> If the cookie is not found, create a new object as above. </li>
       <li> If the cookie is found, that WebAppConnection is used. </li>
       </ul> </li>
    </ul> </li>
</ul>

The sequence for a server-to-browser message is as follows:

<ul>
<li> The user's application (in a WebAppConnection -derived object) 
     calls WebAppConnection::sendMessage.
<li> WebAppConnection::sendMessage routes the data to the connData
     member, in this case WebAppConnectionDataFastCGI::sendMessage.
<li> This method converts the data to a base64 stream using 
     b64_encode_quantum. The result is then pushed to the list
     WebAppConnectionDataFastCGI::outq. 
<li> If there is currently an Ajax GET transaction in progress, there
     would be an open WebFastCGIConnection currently in the BLOCKING state,
     and it would have installed a pointer to itself in
     WebAppConnectionDataFastCGI::waiter.  Otherwise waiter is null.
<li> If WebAppConnectionDataFastCGI::waiter is currently null (no Ajax
     GET transaction is currently in progress):
    <ul>
    <li> The base64-encoded message is left on the outq.
    <li> On the next Ajax GET request, during
          WebFastCGIConnection::startOutput, the outq is consulted.
    <li> A message is found on outq, so
         WebAppConnectionDataFastCGI::sendFrontMessage is called.
    </ul>
<li> If WebAppConnectionDataFastCGI::waiter is currently not null (an 
     Ajax GET is currently open and in the BLOCKED state):
    <ul>
    <li> WebAppConnectionDataFastCGI::sendFrontMessage is immediately called.
    </ul>
<li> in either case, sendFrontMessage builds a WebAppMessage and calls
     WebFastCGIConnection::sendMessage.
<li> This method builds a FastCGIRecord and sends it to the web server,
     and then invokes fdThreadLauncher::stopFdThread to close the 
     connection.
<li> The web server sends it on to the browser and closes the GET
     transaction.
<li> The Javascript code invokes the success callback for the Ajax
     transaction and consumes the data (base64 decoding, etc).
</ul>

The sequence for a browser-to-server message is as follows:

<ul>
<li> the Javascript code creates an Ajax transaction (usually
     XMLHttpRequest) and packs up the base64-encoded message data
     for it.
<li> The web server process opens a new FastCGI connection to the
     corresponding serverPort. 
<li> in fdThreadLauncher::threadEntry, select returns and calls the 
     virtual method serverPort::handleReadSelect.
<li> This method constructs a new WebFastCGIConnection.
<li> WebFastCGIConnection reads the FastCGIRecord messages streaming
     from the web server, and locates and parses the URI path, query
     string, query type, and cookie.
<li> WebFastCGIConnection::startWac calls
     WebServerConnectionBase::findResource to
     locate a matching WebAppServerConfigRecord (matched on the 'route')
     which is then dynamic_cast to WebAppConnectionDataFastCGI.
<li> The WebAppConnectionDataFastCGI::conns map is searched for a matching
     cookie. If one is found, then a WebAppConnection object has been 
     located.  If not, the user's WebAppConnectionCallback -derived object
     is invoked to make a new WebAppConnection -derived object.
<li> Later, during WebFastCGIConnection::decodeInput, the FastCGIRecord
     data from the web server is base64-decoded.  A WebAppMessage is
     constructed, and the user's WebAppConnection::onMessage virtual 
     method is called.
</ul>

\endcond

 */

} // namespace WebAppServer

#endif /* __WEBAPPSERVER_H_ */
