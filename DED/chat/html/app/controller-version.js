
var pfkVersionCtlrLastModified = {
    file:"controller-version.js", modified:"2022/02/23  22:05:51"
};

var pfkVersionCtlr = function($scope) {

    $scope.lastModifieds = [
	indexHtmlLastModified,
        pfkChatCtlrLastModified,
        pfkVersionCtlrLastModified,
        wsStatusCtlrLastModified,
        chatBoxDirectiveHTMLlastModified,
        chatBoxDirectiveLastModified,
        pfkChatDataModelLastModified,
        webSocketServiceLastModified,
        pchatBoxDirectiveLastModified,
        pchatBoxDirectiveHTMLlastModified,
        pfkChatLastModified
    ];

    lastModifieds = $scope.lastModifieds;

    $scope.protoversion = PFK.Chat.CurrentProtoVersion;

    $scope.buttonRotation = 0;

    $("#version-table").hide();

    // a good way to introduce a bug would be to call this
    // with a step function that does not evenly divide from-to
    var animateRotation = function(from,to,step,interval) {
        var config = {
            start : from,
            finish : to,
            current : from,
            step : step,
            interval : interval
        };
        var func = function() {
            config.current += config.step;
            $scope.$apply($scope.buttonRotation = config.current);
            if (config.current != config.finish)
                window.setTimeout(config.func, config.interval);
        }
        config.func = func;
        window.setTimeout(func, config.interval);
    };

    $scope.button = function() {
        if ($scope.buttonRotation == 0)
        {
            animateRotation(0, 90, 10, 20);
            $("#version-table").slideDown();
        }
        else
        {
            animateRotation(90, 0, -10, 20);
            $("#version-table").slideUp();
        }
    };

    function resort() {
        $scope.lastModifieds.sort(function(a,b) {
            if (a.modified[0] == '<')
                return 1;
            if (b.modified[0] == '<')
                return -1;
            if (a.modified < b.modified)
                return 1;
            else
                return -1;
        });
    };

    [

        chatBoxDirectiveHTMLlastModified,
        pchatBoxDirectiveHTMLlastModified

    ].forEach(function(item) {
        $scope.$watch(function() {
            return item.modified;
        }, resort)});

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
