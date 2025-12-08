document.addEventListener('focusin', function(e) {
    var tag = e.target.tagName;
    var editable = (tag === 'INPUT' || tag === 'TEXTAREA' || e.target.isContentEditable);
    if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.focus) {
        window.webkit.messageHandlers.focus.postMessage({isEditable: editable});
    }
});
document.addEventListener('focusout', function(e) {
    setTimeout(function() {
        var a = document.activeElement;
        var tag = a ? a.tagName : '';
        var editable = (tag === 'INPUT' || tag === 'TEXTAREA' || (a && a.isContentEditable));
        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.focus) {
            window.webkit.messageHandlers.focus.postMessage({isEditable: editable});
        }
    }, 0);
});
