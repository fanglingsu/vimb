// Check if the currently focused element is editable
// Returns: true if active element is editable, false otherwise
(function() {
    var a = document.activeElement;
    if (!a) return false;
    if (a.isContentEditable) return true;
    if (a.tagName === 'TEXTAREA') return true;
    if (a.tagName === 'INPUT') {
        var t = (a.type || '').toLowerCase();
        return ['', 'text', 'password', 'email', 'search', 'tel', 'url', 'number', 'date', 'time'].indexOf(t) >= 0;
    }
    return false;
})()
