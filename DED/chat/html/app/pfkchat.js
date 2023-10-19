
'use strict';

var pfkChatLastModified = {
    file:"pfkchat.js", modified:"2022/02/23  22:07:27"
};

angular.module("pfkChatApp.services", [])
    .factory('Data',            ['$rootScope', '$sce', pfkChatDataModel ])
    .factory('webSocket',       ['$rootScope', 'Data', webSocketService ])
;

angular.module("pfkChatApp.directives", ['ngAnimate'])
    .directive("chatbox",   ['$sce', chatBoxDirective])
    .directive("pchatbox",  ['Data', '$http', '$compile', '$animate',
                             pchatBoxDirective])
;

angular.module("pfkChatApp.controllers", [])
    .controller('wsStatusCtlr',     ['$scope', 'Data', 'webSocket',
                                     wsStatusCtlr     ])
    .controller('pfkChatCtlr',      ['$scope', 'Data', pfkChatCtlr      ])
    .controller('pfkVersionCtlr',   ['$scope', pfkVersionCtlr])
;

// could also have filters here

angular.module('pfkChatApp', ['pfkChatApp.services',
                              'pfkChatApp.controllers',
                              'pfkChatApp.directives',
                              'ngRoute', 'ngSanitize'])
/* no longer needed
    .config(['$routeProvider', function($routeProvider) {
        $routeProvider.when('/chat.view', {
            templateUrl: 'view-chat.html',  controller: 'pfkChatCtlr'      });
        $routeProvider.when('/login.view', {
            templateUrl: 'view-login.html', controller: 'pfkChatLoginCtlr' });
        $routeProvider.otherwise({redirectTo: '/login.view'});
    }]) */
;

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
