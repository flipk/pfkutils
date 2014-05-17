
var pfkChatCtlrLastModified = {
    file:"controller-chat.js", modified:"2013/10/01  22:19:41"
};

var pfkChatCtlr = function($scope, depData) {
    $scope.data = depData;

    $scope.sendMessage = function(msg) {
        $scope.$root.$broadcast('send_IM', msg);
    };

    $scope.stateEmpty = PFK.Chat.TypingState.STATE_EMPTY;
    $scope.stateTyping = PFK.Chat.TypingState.STATE_TYPING;
    $scope.stateEntered = PFK.Chat.TypingState.STATE_ENTERED_TEXT;

    $scope.idle2min = function(secs) {
        if (secs < 60)
            return "";
        var hours = Math.floor(secs / 3600);
        var left = secs - (hours * 3600);
        var mins = Math.floor(left / 60);
        if (secs < 3600)
            return mins.toString() + "m";
        if (mins > 0)
            return hours.toString() + "h" + mins.toString() + "m";
        return hours.toString() + "h";
    };

    $scope.typing2opacity = function(user) {
        if (user.typing == $scope.stateTyping || user.idle > 60)
            return "1";
        return "0";
    }

    $scope.typingIdle2str = function(user) {
        if (user.typing == $scope.stateTyping)
        {
            user.lasttyping2Idlestr = "typing";
            return user.lasttyping2Idlestr;
        }
        if (user.idle > 60)
        {
            user.lasttyping2Idlestr = $scope.idle2min(user.idle);
            return user.lasttyping2Idlestr;
        }
        if (user.lasttyping2Idlestr == null)
            user.lasttyping2Idlestr = "typing";
        return user.lasttyping2Idlestr;
    }

    $scope.typingIdleTitle = function(user) {
        if (user.typing == $scope.stateTyping)
            return "User " + user.name + " is currently typing a message";
        if (user.idle > 60)
            return "User " + user.name + " is currently idle";
        return "why are you seeing this?"
    }

    $scope.unread2opacity = function(user) {
        if (user.unread == 0)
            return "0";
        return 1;
    }

    var lastSent = null;
    $scope.sendTypingInd = function(state) {
        if (lastSent == null || lastSent != state)
        {
            $scope.$root.$broadcast('sendTypingInd', state);
        }
        lastSent = state;
    };

    $scope.msgentryKeyup = function(key) {
        if (key.which == 13) // return key
        {
            var msg = $scope.data.msgentry.trim();
            if (msg.length > 0)
            {
                $scope.sendMessage(msg);
                $scope.data.postmsg($scope.data.username, msg, true);
            }
            $scope.data.msgentry = "";
            $scope.sendTypingInd($scope.stateEmpty);
        }
        else
        {
            if ($scope.data.msgentry.length > 0)
                $scope.sendTypingInd($scope.stateTyping);
            else
                $scope.sendTypingInd($scope.stateEmpty);
            //            $scope.sendTypingInd($scope.stateEnteredText);
        }
    }

    $scope.clearButton = function() {
        $scope.data.immsgs_bcast = [];
    }

    if ($scope.data.msgentry == "")
        $scope.sendTypingInd($scope.stateEmpty);
    else
        $scope.sendTypingInd($scope.stateTyping);

    $scope.mouseEnterUser = function(id) {
        $scope.data.userList.forEach(function(u) {
            if (u.id == id)
            {
                if (u.pchatOpen == false)
                    u.plusopacity = "1";
                else
                    u.plusopacity = "0";
            }
        });
    };

    $scope.mouseLeaveUser = function(id) {
        $scope.data.userList.forEach(function(u) {
            if (u.id == id)
                u.plusopacity = "0";
        });
    };

    $scope.userClickPlus = function(id,name,pchatOpen) {
        if (pchatOpen)
            console.log('already open');
        else
        {
            $scope.data.userList.forEach(function(u) {
                if (u.id == id)
                {
                    u.pchatOpen = true;
                    u.plusopacity = "0";
                }
            });
            var chatDesc = {
                id : id,
                status : true,
                name : name
            };
            $scope.data.personalChats.push(chatDesc);
        }
    };

    debugChatCtlr = $scope;
};

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
