// Lock (disable) an input element by ID
// Usage: g_strdup_printf(JS_LOCK_INPUT, escaped_id)
// Returns: true if element found and locked, false otherwise
(function() {
    var e = document.getElementById('%s');
    if (e) {
        e.disabled = true;
        return true;
    }
    return false;
})()
