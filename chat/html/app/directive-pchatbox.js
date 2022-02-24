
var pchatBoxDirectiveLastModified = {
    file:"directive-pchatbox.js", modified:"2022/02/23  22:07:02"
};
var pchatBoxDirectiveHTMLlastModified = {
    file : "directive-pchatbox.html", modified : "<not loaded>"
};

var pchatBoxDirective = function(data, $http, $compile, $animate) {

    pchatBoxHTMLTemplateData = null;
    pchatBoxHTMLTemplateStatus = null;
    pchatBoxHTMLTemplateHeaders = null;
    pchatBoxHTMLTemplateConfig = null;

    $http.get('directive-pchatbox.html').success(
        function(data, status, headers, config) {
            pchatBoxHTMLTemplateData = data;
        }).error(function(data, status, headers, config) {
            console.log('error getting directive-pchatbox.html');
            pchatBoxHTMLTemplateData = data;
            pchatBoxHTMLTemplateStatus = status;
            pchatBoxHTMLTemplateHeaders = headers;
            pchatBoxHTMLTemplateConfig = config;
        });

    var preLink = function(scope, element, attrs, controller) {
    }
    var postLink = function(scope, element, attrs, controller) {
        var newElement = $compile(pchatBoxHTMLTemplateData);
        element.append(newElement(scope));
        element.hide();
        element.slideDown();
        var htmlLm = element.find("#pchat-box-directive")
            .attr("last-modified");
        if (htmlLm)
            pchatBoxDirectiveHTMLlastModified.modified = htmlLm;
    }

    var ret = {
        restrict : "E",
        replace : true,
        scope : {
            pchat : '=',
            boxTitle : '@',
            boxTitleTooltip : '@',
            myusername : '='
        },

// there is a problem with using templateUrl in this case.
// https://github.com/angular/angular.js/issues/3792
// the workaround is to use an inline template, gross.

        template : '<div></div>',

//        templateUrl : 'directive-pchatbox.html',

        controller : function($scope, $element, $attrs, $transclude) {

            $scope.data = data;

            $scope.boxWidth = 400;
            $scope.boxHeight = 200;
            $scope.focusSelect = function() {
                console.log('private chat focus select stub');
                // xxx
            }
            $scope.clearButton = function() {
                console.log('private chat clear box stub');
                // xxx
            }
            $scope.mouseDown = false;
            $scope.oldMouseUp = null;
            $scope.oldMouseMove = null;
            $scope.startpos = { x : 0, y : 0,
                                px : 0, py : 0 };
            $scope.resizeDown = function(evt) {
                $scope.oldMouseUp = window.onmouseup;
                $scope.oldMouseMove = window.onmousemove;
                window.onmouseup = $scope.resizeUp;
                window.onmousemove = $scope.resizeMove;
                $scope.mouseDown = true;
                $scope.startpos.x = evt.clientX;
                $scope.startpos.y = evt.clientY;
                $scope.startpos.px = $scope.boxWidth;
                $scope.startpos.py = $scope.boxHeight;
            }
            $scope.resizeUp = function(evt) {
                $scope.$apply(function() {
                    $scope.mouseDown = false;
                    window.onmouseup = $scope.oldMouseUp;
                    window.onmousemove = $scope.oldMouseMove;
                });
            };
            $scope.resizeMove = function(evt) {
                $scope.$apply(function() {
                    if ($scope.mouseDown)
                    {
                        var dx = evt.clientX - $scope.startpos.x;
                        var dy = evt.clientY - $scope.startpos.y;
                        
                        $scope.boxWidth = $scope.startpos.px + dx;
                        $scope.boxHeight = $scope.startpos.py + dy;
                    }
                });
            };

            $scope.userClosePChat = function(id) {
                $element.slideUp(400, function() {
                    var ind = $scope.data.personalChats.findId(id);
                    if (ind == -1)
                        console.log('chat not found!!');
                    else
                        $scope.data.personalChats.splice(ind,1);
                    
                    ind = $scope.data.userList.findId(id);
                    if (ind == -1)
                        console.log('user not found!!');
                    else
                        $scope.data.userList[ind].pchatOpen = false;
                });
            };
        },
        compile : function(templ,attrs) {
            return {
                pre : preLink,
                post : postLink
            }
        }
    };

    return ret;
};

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
