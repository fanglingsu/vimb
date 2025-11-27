// Unlock (enable) an input element by ID and focus it
// Usage: g_strdup_printf(JS_UNLOCK_INPUT, escaped_id)
// Returns: true if element found and unlocked, false otherwise
(function() {
    var e = document.getElementById('%s');
    if (e) {
        e.disabled = false;
        e.focus();
        return true;
    }
    return false;
})()
