
var wsStatusCtlrLastModified = {
    file:"controller-ws.js", modified:"2013/10/28  13:30:48"
};

var wsStatusCtlr = function($scope, depData, depWebsocket) {
    $scope.data = depData;
    $scope.webSocket = depWebsocket;

    $scope.logoutButton = function() {
        $scope.data.token="";
        document.cookie="CHATAUTHTOKEN=0; max-age=0";
        $scope.data.savePersistent();
        $scope.$root.$broadcast('sendLogout');
        while ($scope.data.personalChats.length > 0)
            $scope.data.personalChats.pop();
        while ($scope.data.immsgs_bcast.length > 0)
            $scope.data.immsgs_bcast.pop();
        while ($scope.data.userList.length > 0)
            $scope.data.userList.pop();
        location.reload(true);
    }

    $scope.$on('roundTripDelay', function(scope,time) {
        //xxx
    });

    debugWsCtlr = $scope;
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
