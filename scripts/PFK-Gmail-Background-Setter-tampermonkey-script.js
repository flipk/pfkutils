// ==UserScript==
// @name         PFK Gmail Background Setter
// @namespace    http://tampermonkey.net/
// @version      0.1
// @description  stupid corp config won't let you set a custom background in gmail tab, so this script will set one for you.
// @author       PFK
// @version      0.2
// @date         2017-04-22
// @match        https://mail.google.com/*
// @run-at       document-end
// @noframes     true
// @grant        none
// @license      MIT License
// ==/UserScript==

(function() {
    'use strict';
    var counter = 0;
    var BGURL = "https://www.pfk.org/park/batarang_2_1280x1024.png";
    var TITLE = "PFK Gmail Background Setter";
    console.warn(TITLE+" started");
    var setBackground = function() {
        console.warn(TITLE+" timer fired");
        var bgSet = 0;
        var pfkDivs = document.getElementsByClassName("no");
        if (pfkDivs !== undefined)
        {
            var pfkDiv = pfkDivs[1];
            if (pfkDiv !== undefined)
            {
//                console.warn(pfkDiv);
//                pfkDiv.setAttribute("style","background:#203040;");
                pfkDiv.setAttribute("style","background-image:url("+BGURL+")");
                console.warn(TITLE+" has set the background");
                bgSet = 1;
            }
        }

        if (bgSet === 0)
        {
            console.warn(TITLE+" did not find the right div");
            counter++;
            if (counter < 60)
                window.setTimeout(setBackground,1000);
            else
                console.warn(TITLE+" giving up");
        }
    };
    setBackground();
})();
