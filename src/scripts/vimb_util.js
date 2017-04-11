/* Utility object injected into all frames. */
var vimb = Object.freeze((function(){
    'use strict';
    /* Checks if given element is editable */
    function isEditable(e) {
        if (e) {
            var name = e.tagName.toLowerCase();
            return e.isContentEditable
                || name == "textarea"
                || (name == "input" && /^(?:color|date|datetime|datetime-local|email|month|number|password|search|tel|text|time|url|week)$/i.test(e.type));
        }
        return false;
    }

    return {
        init: function() {
            var b = document.body;
            b.addEventListener(
                "focus",
                function(ev) {
                    var e = ev.target, n;
                    if (isEditable(e)) {
                        window.webkit.messageHandlers.focus.postMessage(1);
                    }
                },
                true
            );
            b.addEventListener(
                "blur",
                function() {
                    window.webkit.messageHandlers.focus.postMessage(0);
                },
                true
            );
        }
    };
})());
vimb.init();
