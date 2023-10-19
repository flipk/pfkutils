
var wsStatusCtlrLastModified = {
    file:"controller-ws.js", modified:"2022/02/23  22:08:03"
};

var wsStatusCtlr = function($scope, depData, depWebsocket) {
    $scope.data = depData;
    $scope.webSocket = depWebsocket;

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
  eval: (add-hook 'write-file-functions 'time-stamp)
  time-stamp-line-limit: 5
  time-stamp-start: "modified:\""
  time-stamp-format: "%:y/%02m/%02d  %02H:%02M:%02S\""
  time-stamp-end: "$"
  End:
*/
