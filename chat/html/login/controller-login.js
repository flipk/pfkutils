
var pfkChatLoginCtlrLastModified = {
    file:"controller-login.js", modified:"2013/10/01  22:17:53"
};

var pfkChatLoginCtlr = function($scope, depData) {
    $scope.data = depData;

    $scope.loginFeedback = "";
    $scope.createFeedback = "";

    $scope.newusername = "";
    $scope.newpassword = "";
    $scope.newpassword2 = "";

    $scope.doLogin = function() {
        $scope.$root.$broadcast('sendLogin',
                                $scope.data.username,
                                $scope.data.password);
    };

    $scope.doRegister = function() {
        if ($scope.newpassword != $scope.newpassword2)
        {
            $scope.createFeedback = "PASSWORDS DON'T MATCH";
            return;
        }
        if ($scope.newpassword.length < 4)
        {
            $scope.createFeedback = "PASSWORD TOO SHORT";
            return;
        }
        $scope.data.token = "";
        $scope.data.username = $scope.newusername;
        $scope.data.password = $scope.newpassword;
        $scope.$root.$broadcast('sendRegister',
                                $scope.newusername,
                                $scope.newpassword);
    };

    $scope.loginButton = $scope.doLogin;
    $scope.registerButton = $scope.doRegister;

    $scope.passwdKeypress = function(evt) {
        if (evt.which == 13)
            $scope.doLogin();
    }

    $scope.regPasswdKeypress = function(evt) {
        if (evt.which == 13)
            $scope.doRegister();
    }

    $scope.$on('registerFailure', function(scope, reason) {
        $scope.createFeedback = reason;
    });

    $scope.data.messages = "";
    $scope.data.msgentry = "";
    $scope.data.savePersistent();

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
