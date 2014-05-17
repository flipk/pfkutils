
var pfkChatDataModelLastModified = {
    file:"service-data.js", modified:"2013/10/01  22:48:14"
};

var pfkChatDataModel = function($rootScope, $sce) {
    var ret = {
        username   : ('PFK_Chat_Username' in localStorage) ?
            localStorage.PFK_Chat_Username : "",
        password   : ('PFK_Chat_Password' in localStorage) ?
            localStorage.PFK_Chat_Password : "",
        token      : ('PFK_Chat_Token'    in localStorage) ?
            localStorage.PFK_Chat_Token    : "",
        id : -1,

        savePersistent : function() {
            localStorage.PFK_Chat_Username = ret.username;
            localStorage.PFK_Chat_Password = ret.password;
            localStorage.PFK_Chat_Token    = ret.token;
        },

        connectionStatus : 'INITIALIZING',
        connectionStatusColor : 'red',

        setStatus : function(status,color) {
            this.connectionStatus = status;
            this.connectionStatusColor = color;
        }
    };

    pfkChatData = ret;
    return ret;
}

/*
  Local Variables:
  mode: javascript
  indent-tabs-mode: nil
  tab-width: 8
  eval: (add-hook 'write-file-hooks 'time-stamp)
  time-stamp-line-limit: 5
  time-stamp-start: "modified:\""
  time-stamp-format: "%:y/%02m/%02d  %02H:%02M:%02S\""
  time-stamp-end: "$"
  End:
*/
