
var chatBoxDirectiveLastModified = {
    file:"directive-chatbox.js", modified:"2013/09/25  00:41:26"
};
var chatBoxDirectiveHTMLlastModified = {
    file : "directive-chatbox.html", modified : "<not loaded>"
};

var chatBoxDirective = function($sce) {

    var scrollbox = null;
    var inputbox = null;

//    var newelement = angular.element('<some html />');

    var preLink = function(scope, iElement, iAttrs, controller) {
    }
    var postLink = function(scope, iElement, iAttrs, controller) {
        scrollbox = iElement.find("#chatbox_scrollable");
        inputbox = iElement.find("#chatbox_input");
        chatBoxDirectiveHTMLlastModified.modified =
            iElement.attr("last-modified");
    }

    var ret = {
        restrict: "E",
        replace : true,
        scope : {
            // variables here can be substituted in the template!
            // '@' means look at <chatbox> attributes and copy.
            // '=' means set up 2-way binding with variable name in
            // parent scope (controller in this case)
            // '&' means hook for function execution.
            boxTitle : '@',
            boxTitleTooltip : '@',
            messages : '=',
            myusername : '=',
            msgentry : '=',
            msgentrykeyup : '=',
            clearbutton : '='
        },
// require: 'sibling' or '^parent'  (note new arg to link funcs)
        templateUrl : 'directive-chatbox.html',
        // scope is this object's scope
        // element is the template
        controller : function($scope, $element, $attrs, $transclude
//                              , otherInjectables
                             ) {

            $scope.boxWidth = ('PFK_Chat_Public_Chat_Size_X' in localStorage) ?
                parseInt(localStorage.PFK_Chat_Public_Chat_Size_X) : 600;
            $scope.boxHeight = ('PFK_Chat_Public_Chat_Size_Y' in localStorage) ?
                parseInt(localStorage.PFK_Chat_Public_Chat_Size_Y) : 400;
            $scope.inputHeight = 20;

            $scope.$watch('msgentry', function(newVals,oldVals) {
                window.setTimeout(function() {
                    var ss = 20;
                    if ($scope.msgentry.length > 0)
                    {
                        ss = inputbox.prop("scrollHeight");
                        ss -= 4;
                        if (ss < 20) // firefox returns small numbers
                            ss = 20;
                    }
                    $scope.$apply($scope.inputHeight = ss);
//                    inputbox.css("height", ss.toString() + "px");
                },10)});

            $scope.$watchCollection('messages',function(newVals,oldVals) {
                // for some reason setting scrollTop doesn't work
                // inside watchCollection -- it's always off by some,
                // probably because the box needs to redraw for scrollHeight
                // to be updated? it works from a timer though.
                window.setTimeout(function() {
                    scrollbox[0].scrollTop = scrollbox[0].scrollHeight;
                }, 10);
            });

            pfkSce = $sce;

            $scope.trustHtml = function(data) {
                return data;
//                return $sce.trustAsHtml(data);
            };

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
            };
            $scope.resizeUp = function(evt) {
                $scope.$apply(function() {
                    $scope.mouseDown = false;
                    window.onmouseup = $scope.oldMouseUp;
                    window.onmousemove = $scope.oldMouseMove;
                });
                localStorage.PFK_Chat_Public_Chat_Size_X =
                    $scope.boxWidth.toString();
                localStorage.PFK_Chat_Public_Chat_Size_Y =
                    $scope.boxHeight.toString();
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
            $scope.focusSelect = function() {
                inputbox[0].focus();
            }
        },

        // templ is the chatbox.html elements.
        // attrs is the attributes passed to <chatbox>
        compile : function(templ,attrs) {
            //templ.append(new elements?)
            return {
                pre : preLink,
                post : postLink
            }
        }

    };

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
