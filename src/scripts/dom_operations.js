/* vimb - DOM Operations Module
 *
 * This module replaces the deprecated WebKitDOM API with JavaScript-based
 * DOM manipulation functions. */
var vimbDomOps = Object.freeze((function() {
    'use strict';

    /* List of input types that are considered editable. */
    var EDITABLE_INPUT_TYPES = [
        '', 'text', 'password', 'color', 'date', 'datetime',
        'datetime-local', 'email', 'month', 'number', 'search',
        'tel', 'time', 'url', 'week'
    ];

    /* Check if an element is editable.
     *
     * @param {Element} element - The element to check
     * @return {boolean} true if element is editable */
    function isEditable(element) {
        if (!element) {
            return false;
        }

        // Check if element has contentEditable attribute
        if (element.isContentEditable) {
            return true;
        }

        // Check if element is a textarea
        if (element.tagName === 'TEXTAREA') {
            return true;
        }

        // Check if element is an input with editable type
        if (element.tagName === 'INPUT') {
            var type = (element.type || '').toLowerCase();
            return EDITABLE_INPUT_TYPES.indexOf(type) >= 0;
        }

        return false;
    }

    /* Check if an element is visible.
     *
     * @param {Element} element - The element to check
     * @return {boolean} true if element is visible */
    function isVisible(element) {
        if (!element) {
            return false;
        }

        var rect = element.getBoundingClientRect();
        if (!rect || rect.width === 0 || rect.height === 0) {
            return false;
        }

        var style = window.getComputedStyle(element);
        return style.display !== 'none' && style.visibility === 'visible';
    }

    /* Find and focus the first editable input element in the document.
     *
     * This function searches for input fields and textareas using XPath,
     * checks if they are visible, and focuses the first visible one.
     * It also recursively searches in iframes.
     *
     * @param {Document} doc - The document to search (defaults to current document)
     * @return {boolean} true if an input was focused, false otherwise */
    function focusFirstInput(doc) {
        doc = doc || document;

        /* XPath expression to find editable elements.
         * Uses translate() for case-insensitive matching. */
        var xpath = "//input[not(@type) " +
            "or translate(@type,'ETX','etx')='text' " +
            "or translate(@type,'ADOPRSW','adoprsw')='password' " +
            "or translate(@type,'CLOR','clor')='color' " +
            "or translate(@type,'ADET','adet')='date' " +
            "or translate(@type,'ADEIMT','adeimt')='datetime' " +
            "or translate(@type,'ACDEILMOT','acdeilmot')='datetime-local' " +
            "or translate(@type,'AEILM','aeilm')='email' " +
            "or translate(@type,'HMNOT','hmnot')='month' " +
            "or translate(@type,'BEMNRU','bemnru')='number' " +
            "or translate(@type,'ACEHRS','acehrs')='search' " +
            "or translate(@type,'ELT','elt')='tel' " +
            "or translate(@type,'EIMT','eimt')='time' " +
            "or translate(@type,'LRU','lru')='url' " +
            "or translate(@type,'EKW','ekw')='week' " +
            "]|//textarea";

        try {
            /* Evaluate XPath expression */
            var result = doc.evaluate(
                xpath,
                doc.documentElement,
                null,
                XPathResult.ORDERED_NODE_SNAPSHOT_TYPE,
                null
            );

            /* Iterate through results and focus first visible element */
            for (var i = 0; i < result.snapshotLength; i++) {
                var element = result.snapshotItem(i);
                if (isVisible(element)) {
                    element.focus();
                    return true;
                }
            }

            /* If no input found in main document, search in iframes */
            var iframes = doc.querySelectorAll('iframe');
            for (var j = 0; j < iframes.length; j++) {
                try {
                    var frameDoc = iframes[j].contentDocument;
                    if (frameDoc && focusFirstInput(frameDoc)) {
                        return true;
                    }
                } catch (e) {
                    /* Cross-origin iframe, skip it */
                    continue;
                }
            }

        } catch (e) {
            console.error("focusFirstInput error:", e);
        }

        return false;
    }

    /* Get the value of an editable element.
     *
     * Note: This function is defined in ext-dom.c but not actually used
     * anywhere in the codebase. Included for completeness.
     *
     * @param {Element} element - The element to get value from
     * @return {string} The element's value or empty string */
    function getEditableValue(element) {
        if (!element) {
            return '';
        }

        try {
            if (element.isContentEditable) {
                return element.innerText || '';
            } else if (element.tagName === 'INPUT') {
                return element.value || '';
            } else if (element.tagName === 'TEXTAREA') {
                return element.value || '';
            }
        } catch (e) {
            console.error("getEditableValue error:", e);
        }

        return '';
    }

    /* Lock (disable) an input element by ID.
     *
     * @param {string} elementId - The ID of the element to lock
     * @return {boolean} true if element was locked, false if not found */
    function lockInput(elementId) {
        try {
            var elem = document.getElementById(elementId);
            if (elem) {
                elem.disabled = true;
                return true;
            }
        } catch (e) {
            console.error("lockInput error:", e);
        }
        return false;
    }

    /* Unlock (enable) and focus an input element by ID.
     *
     * @param {string} elementId - The ID of the element to unlock
     * @return {boolean} true if element was unlocked, false if not found */
    function unlockInput(elementId) {
        try {
            var elem = document.getElementById(elementId);
            if (elem) {
                elem.disabled = false;
                elem.focus();
                return true;
            }
        } catch (e) {
            console.error("unlockInput error:", e);
        }
        return false;
    }

    /* Get the current scroll position and calculate percentage.
     *
     * Returns an object with scroll information that can be serialized
     * and sent to the main process.
     *
     * @return {Object} Object with properties: max, percent, top */
    function getScrollPosition() {
        try {
            var de = document.documentElement;
            var body = document.body;

            if (!de || !body) {
                return {max: 0, percent: 0, top: 0};
            }

            /* Get scroll position (use maximum of documentElement and body) */
            var scrollTop = Math.max(
                de.scrollTop || 0,
                body.scrollTop || 0
            );

            /* Get total scrollable height */
            var scrollHeight = Math.max(
                de.scrollHeight || 0,
                body.scrollHeight || 0
            );

            /* Get viewport height */
            var clientHeight = window.innerHeight || 0;

            /* Calculate maximum scroll distance */
            var max = scrollHeight - clientHeight;

            var percent = 0;
            var top = 0;

            if (max > 0) {
                percent = Math.round(scrollTop * 100 / max);
                top = scrollTop;
            }

            return {
                max: max,
                percent: percent,
                top: top
            };

        } catch (e) {
            console.error("getScrollPosition error:", e);
            return {max: 0, percent: 0, top: 0};
        }
    }

    /* Check if the currently active element is editable.
     *
     * This is used for focus event handling to determine if input mode
     * should be activated.
     *
     * @param {Document} doc - The document to check (defaults to current document)
     * @return {boolean} true if active element is editable */
    function checkActiveElement(doc) {
        doc = doc || document;

        try {
            var active = doc.activeElement;
            if (!active) {
                return false;
            }

            /* If active element is an iframe, check inside it */
            if (active.tagName === 'IFRAME') {
                try {
                    return checkActiveElement(active.contentDocument);
                } catch (e) {
                    /* Cross-origin iframe */
                    return false;
                }
            }

            /* Check if the active element is editable */
            return isEditable(active);

        } catch (e) {
            console.error("checkActiveElement error:", e);
            return false;
        }
    }

    /* Setup event listeners for focus tracking.
     *
     * This should be called when a page loads to set up listeners
     * that will notify the extension when editable elements gain/lose focus.
     *
     * @param {number} pageId - The WebKit page ID */
    function setupFocusTracking(pageId) {
        /* Focus event - check if focused element is editable */
        window.addEventListener('focus', function(event) {
            try {
                var isEditable = checkActiveElement();
                /* Send message to extension via message handler */
                if (window.webkit && window.webkit.messageHandlers &&
                    window.webkit.messageHandlers.focus) {
                    window.webkit.messageHandlers.focus.postMessage(
                        JSON.stringify({
                            pageId: pageId,
                            isEditable: isEditable
                        })
                    );
                }
            } catch (e) {
                console.error("focus event error:", e);
            }
        }, true); /* Use capture phase */

        /* Blur event - element lost focus */
        window.addEventListener('blur', function(event) {
            try {
                /* Send message indicating no editable element has focus */
                if (window.webkit && window.webkit.messageHandlers &&
                    window.webkit.messageHandlers.focus) {
                    window.webkit.messageHandlers.focus.postMessage(
                        JSON.stringify({
                            pageId: pageId,
                            isEditable: false
                        })
                    );
                }
            } catch (e) {
                console.error("blur event error:", e);
            }
        }, true); /* Use capture phase */
    }

    /* Setup event listeners for scroll tracking.
     *
     * This should be called when a page loads to set up listeners
     * that will notify the extension of scroll position changes.
     *
     * @param {number} pageId - The WebKit page ID */
    function setupScrollTracking(pageId) {
        window.addEventListener('scroll', function() {
            try {
                var scrollData = getScrollPosition();
                /* Send message to extension via message handler or signal */
                /* Note: This might need to use a different communication method
                 * depending on how the extension is set up */
                if (window.webkit && window.webkit.messageHandlers &&
                    window.webkit.messageHandlers.scroll) {
                    window.webkit.messageHandlers.scroll.postMessage(
                        JSON.stringify({
                            pageId: pageId,
                            max: scrollData.max,
                            percent: scrollData.percent,
                            top: scrollData.top
                        })
                    );
                }
            } catch (e) {
                console.error("scroll event error:", e);
            }
        }, false);
    }

    /* Initialize all event tracking for a page.
     *
     * This should be called once when a document is loaded.
     *
     * @param {number} pageId - The WebKit page ID */
    function init(pageId) {
        try {
            setupFocusTracking(pageId);
            setupScrollTracking(pageId);

            /* Check initial state */
            checkActiveElement();

            /* Get initial scroll position */
            getScrollPosition();

            return true;
        } catch (e) {
            console.error("vimbDomOps.init error:", e);
            return false;
        }
    }

    /* Public API */
    return {
        isEditable: isEditable,
        isVisible: isVisible,
        focusFirstInput: focusFirstInput,
        getEditableValue: getEditableValue,
        lockInput: lockInput,
        unlockInput: unlockInput,
        getScrollPosition: getScrollPosition,
        checkActiveElement: checkActiveElement,
        setupFocusTracking: setupFocusTracking,
        setupScrollTracking: setupScrollTracking,
        init: init
    };
})());
