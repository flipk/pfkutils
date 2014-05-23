/* -*- Mode:c++; eval:(c-set-style "BSD"); c-basic-offset:4; indent-tabs-mode:nil; tab-width:8 -*-  */

#ifndef __WEBAPPSERVER_H_
#define __WEBAPPSERVER_H_

#include <unistd.h>
#include <list>
#include <string>

namespace WebAppServer {

enum WebAppMessageType { 
    WS_TYPE_INVALID,
    WS_TYPE_TEXT,
    WS_TYPE_BINARY,
    WS_TYPE_CLOSE
};

struct WebAppMessage {
    WebAppMessage(WebAppMessageType _type, 
                  const std::string &_buf) : type(_type), buf(_buf) { }
    WebAppMessageType type;
    const std::string &buf;
};

class WebServerConnectionBase;
class WebAppConnectionData;
class WebAppConnection {
    friend class WebServerConnectionBase;
public:
    WebAppConnection(void);
    virtual ~WebAppConnection(void);
    // return false to close
    virtual bool onMessage(const WebAppMessage &m) = 0;
    virtual bool doPoll(void) = 0;
    void sendMessage(const WebAppMessage &m);
    WebAppConnectionData * connData;
};

class WebAppConnectionCallback {
public:
    virtual ~WebAppConnectionCallback(void) { /*placeholder*/ }
    virtual WebAppConnection * newConnection(void) = 0;
};

struct WebAppServerConfigRecord;
class WebAppServerConfig {
    std::list<WebAppServerConfigRecord*>  records;
    void clear(void);
public:
    WebAppServerConfig(void);
    ~WebAppServerConfig(void);
    void addWebsocket(int port, const std::string route,
                      WebAppConnectionCallback *cb,
                      int pollInterval = -1);
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
class WebAppServer {
    const WebAppServerConfig *config;
    serverPorts * ports;
public:
    WebAppServer(void);
    ~WebAppServer(void);
    // return false if failure to start
    bool start(const WebAppServerConfig *config);
    void stop(void);
};

} // namespace WebAppServer

#endif /* __WEBAPPSERVER_H_ */

/** \mainpage WebAppServer

<ul>
<li> \ref ClassDiagram
<li> \ref SampleCode
<li> \ref OperationalDescription
</ul>

*/

/** \page ClassDiagram Class Diagram for libWebAppServer

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

*/

/** \page SampleCode Sample code snippets

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

/** \page OperationalDescription Description of Operation

This page describes the internal operation of libWebAppServer.

\section OperationSetup  Setup

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
visitor connects to the server. (See below for definition of a 'new
unique visitor'.)

Each call to WebAppServerConfig::addWebsocket and
WebAppServerConfig::addFastCGI adds a new WebAppServerConfigRecord to
the config object.  Each config record describes one unique route.

\note FastCGI connections actually get a WebAppServerFastCGIConfigRecord
      object, which is derived from WebAppServerConfigRecord.  This is
      because FastCGI connections need to store a little extra data about
      unique visitorId cookies.

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

Once the WebFastCGIConnection or WebSocketConnection is created, it handles
the HTTP MIME headers and data passing using a CircularReader object.
Once the headers have been parsed, cookies and resource paths are parsed
out, and an attachment is formed to an appropriate WebAppConnection object
(which is created by the user's WebAppConnectionCallback, which was attached
to a WebAppServerConfigRecord describing the user's routes).

\section OperationConnection The Web*Connection objects

When WebSocketConnection discovers a route, it follows the configs
list given to it by the serverPort to find a matching route.  When a
matching route is found, the WebAppConnectionCallback provided by the
user is used to construct a new user's WebAppConnection -derived
object. This object will live as long as this TCP
connection. WebSocket messages will be passed in both directions over
this connection for as long as this connection exists.  If the browser
closes the WebSocket object, this WebServerConnectionBase thread will
exit and the corresponding WebAppConnection will also be destroyed.

FastCGI connections do not work this way. A new XHR (XMLHttpRequest) 
connection is established for each message.  An XHR connection follows
the following state diagram:

\dot

digraph XHRStates {
	layout = "dot";
        overlap = "false";
        mode = "maxent";

    node [fontname=Helvetica, fontsize=10];
    edge [len=1.5];

        NoConnection [label="No Connection"];
        B2SMime [label="Browser to\nserver MIME\nheaders"];
        B2SMsg  [label="Browser to\nserver messages"];
        S2BMime [label="Server to\nbrowser MIME\nheaders"];
        S2BMsg [label="Server to\nbrowser messages"];

        NoConnection -> B2SMime;
        B2SMime -> B2SMsg;
        B2SMsg -> S2BMime;
        S2BMime -> S2BMsg;
        S2BMsg -> NoConnection;
}

\enddot

When there are no messages to send, the connection spends most of its
time in the "Server to browser messages" state. The server sends the
MIME headers but does not send any message data until there is
actually a message to send. If the browser decides it needs to send a
message, it has two choices: (1) open a new XHR connection, or (2)
abort this connection and start a new one.

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
         is inserted into the WebAppServerFastCGIConfigRecord "conns"
         map, with the new cookie used as the key to the map. </li>
    <li> This new WebAppConnection is given to the WebServerConnectionBase
         for future message exchange. </li>
    </ul> </li>
<li> If a cookie is set:
    <ul> 
    <li> The WebAppServerFastCGIConfigRecord "conns" map is searched for the
         visitorId cookie.
       <ul>
       <li> If the cookie is not found, create a new object as above. </li>
       <li> If the cookie is found, that WebAppConnection is used. </li>
       </ul> </li>
    </ul> </li>
</ul>

\section OperationUserConnection User's WebAppConnection -derived object

\note A 'new unique visitor' for WebSockets means a 'new WebSocket' 
      connection from a browser. The user's WebAppConnection -derived
      object exists as long as that JavaScript WebSocket object exists
      in the browser, and is deleted when that WebSocket is closed.

\note A 'new unique visitor' for FastCGI means the first XHR
      (XMLHttpRequest) (or in jQuery, "$.ajax", or in Angular,
      "$http") connection from a host with no "visitorId" cookie set
      or an unknown visitorId cookie. The user's WebAppConnection
      -derived continues to exist beyond the closure of that XHR
      connection for a period of time, to wait for the next XHR
      connection (since XHR connections will come and go for every
      message). When a new XHR connection arrives with the same visitorId
      cookie, it is attached to an existing WebAppConnection object if
      possible. The user's WebAppConnection object will be deleted if
      a period of time passes with no XHR connection attached to it.



 */
