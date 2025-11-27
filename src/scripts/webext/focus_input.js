// Focus the first visible input element on the page
// Returns: true if an input was focused, false otherwise
(function() {
    function isVisible(e) {
        if (!e) return false;
        var r = e.getBoundingClientRect();
        if (!r || r.width === 0 || r.height === 0) return false;
        var s = window.getComputedStyle(e);
        return s.display !== 'none' && s.visibility === 'visible';
    }
    var xpath = '//input[not(@type) or @type="text" or @type="password" or @type="email" or @type="search"] | //textarea';
    var result = document.evaluate(xpath, document.documentElement, null, XPathResult.ORDERED_NODE_SNAPSHOT_TYPE, null);
    for (var i = 0; i < result.snapshotLength; i++) {
        var e = result.snapshotItem(i);
        if (isVisible(e)) {
            e.focus();
            return true;
        }
    }
    return false;
})()
