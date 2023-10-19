
var pfkChatDataModelLastModified = {
    file:"service-data.js", modified:"2022/02/23  22:07:34"
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
        msgentry   : "",
        userList : [],
        immsgs_bcast : [],
        idleTime : 0,
        unreadCount : 0,
        origDocumentTitle : document.title,
        flashDocumentTitle : document.title,
        blurred : false,
        titleFlashTimer : null,
        personalChats : [],

        savePersistent : function() {
            localStorage.PFK_Chat_Username = ret.username;
            localStorage.PFK_Chat_Password = ret.password;
            localStorage.PFK_Chat_Token    = ret.token;
            localStorage.PFK_Chat_MsgEntry = ret.msgentry;
        },

        postmsg : function(uname, message, fromMe) {
            this.immsgs_bcast.push(
                { username : uname,
                  msg : $sce.trustAsHtml(message) });
            if (!fromMe)
            {
                ret.unreadCount ++;
                ret.flashDocumentTitle = '(' + ret.unreadCount + ') ' +
                    ret.origDocumentTitle;
                document.title = ret.flashDocumentTitle;
            }
            if (this.immsgs_bcast.length > 1000)
                this.immsgs_bcast.shift();
        },

        connectionStatus : 'INITIALIZING',
        connectionStatusColor : 'red',

        setStatus : function(status,color) {
            this.connectionStatus = status;
            this.connectionStatusColor = color;
        }
    };

//    could set Array.prototype.findUserById if i wanted to.

    ret.personalChats.findId = function(id) {
        var ind = 0;
        if (this.some(function(u) {
            if (u.id == id)
                return true;
            ind++;
            return false;
        }) == false)
        {
            return -1;
        }
        return ind;
    };

    ret.userList.findId = function(id) {
        var ind = 0;
        if (this.some(function(u) {
            if (u.id == id)
                return true;
            ind++;
            return false;
        }) == false)
        {
            return -1;
        }
        return ind;
    };

    window.setInterval(function() {
        $rootScope.$apply(function() {
            ret.idleTime ++;
            for (userInd in ret.userList) {
                ret.userList[userInd].idle++;
            }
        });
    }, 1000);
    window.onmousemove = function() {
        var timeWas = ret.idleTime;
        var countWas = ret.unreadCount;
        ret.idleTime = 0;
        ret.unreadCount = 0;
        document.title = ret.origDocumentTitle;
        ret.flashDocumentTitle = ret.origDocumentTitle;
        if (timeWas > 60 || countWas > 0)
            $rootScope.$broadcast('notIdle');
    }
    window.onkeydown = function() {
        var timeWas = ret.idleTime;
        var countWas = ret.unreadCount;
        ret.idleTime = 0;
        ret.unreadCount = 0;
        document.title = ret.origDocumentTitle;
        ret.flashDocumentTitle = ret.origDocumentTitle;
        if (timeWas > 60 || countWas > 0)
            $rootScope.$broadcast('notIdle');
    }
    window.onblur = function() {
        ret.blurred = true;
        if (ret.titleFlashTimer == null)
            ret.titleFlashTimer = window.setInterval(function() {
                if (document.title === ret.origDocumentTitle)
                    document.title = ret.flashDocumentTitle;
                else
                    document.title = ret.origDocumentTitle;
            }, 1000);
    }
    window.onfocus = function() {
        ret.blurred = false;
        if (ret.titleFlashTimer != null)
        {
            clearInterval(ret.titleFlashTimer);
            ret.titleFlashTimer = null;
        }
        document.title = ret.flashDocumentTitle;
    }
    pfkChatData = ret;
    return ret;
}

/*
  Local Variables:
  mode: javascript
  indent-tabs-mode: nil
  tab-width: 8
  eval: (add-hook 'write-file-functions 'time-stamp)
  time-stamp-line-limit: 5
  time-stamp-start: "modified:\""
  time-stamp-format: "%:y/%02m/%02d  %02H:%02M:%02S\""
  time-stamp-end: "$"
  End:
*/
