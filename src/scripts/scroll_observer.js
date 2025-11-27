(function() {
    'use strict';

    function updateScrollPosition() {
        var de = document.documentElement;
        var body = document.body;

        if (!de || !body) {
            return;
        }

        var scrollTop = Math.max(de.scrollTop || 0, body.scrollTop || 0);
        var scrollHeight = Math.max(de.scrollHeight || 0, body.scrollHeight || 0);
        var clientHeight = window.innerHeight || 0;
        var max = scrollHeight - clientHeight;
        var percent = 0;

        if (max > 0) {
            percent = Math.round(scrollTop * 100 / max);
        }

        if (window.webkit && window.webkit.messageHandlers && window.webkit.messageHandlers.scroll) {
            window.webkit.messageHandlers.scroll.postMessage({
                max: max,
                percent: percent,
                top: scrollTop
            });
        }
    }

    var scrollTimeout;
    window.addEventListener('scroll', function() {
        if (scrollTimeout) {
            clearTimeout(scrollTimeout);
        }
        scrollTimeout = setTimeout(updateScrollPosition, 50);
    }, {passive: true});

    window.addEventListener('resize', updateScrollPosition, {passive: true});

    if (document.readyState === 'complete') {
        setTimeout(updateScrollPosition, 100);
    } else {
        window.addEventListener('load', function() {
            setTimeout(updateScrollPosition, 100);
        });
    }
})();
