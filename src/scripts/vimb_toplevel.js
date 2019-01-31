/* This script is injected only into toplevel frame. */
var VimbToplevel = {};

VimbToplevel.scroll = (mode, scrollStep, count) => {
    let w = window,
        d = document,
        x = y = 0,
        ph = w.innerHeight,
        c = count||1,
        rel = true;
    switch (mode) {
        case 'j':
            y = c * scrollStep;
            break;
        case 'h':
            x = -c * scrollStep;
            break;
        case 'k':
            y = -c * scrollStep;
            break;
        case 'l':
            x = c * scrollStep;
            break;
        case '\x04': /* ^D */
            y = c * ph / 2;
            break;
        case '\x15': /* ^U */
            y = -c * ph / 2;
            break;
        case '\x06': /* ^F */
            y = c * ph;
            break;
        case '\x02': /* ^B */
            y = -c * ph;
            break;
        case 'G': /* fall through - gg and G differ only in y value when no count is given */
        case 'g':
            x = w.scrollX;
            if (count) {
                y = c * ((d.documentElement.scrollHeight - w.innerHeight) / 100);
            } else {
                y = 'G' == mode ? d.body.scrollHeight : 0;
            }
            rel = false;
            break;
        case '0':
            y   = w.scrollY;
            rel = false;
            break;
        case '$':
            x   = d.body.scrollWidth;
            y   = w.scrollY;
            rel = false;
            break;
        default:
            return 1;
    }
    if (rel) {
        w.scrollBy(x, y);
    } else {
        w.scroll(x, y);
    }
    return 0;
};

VimbToplevel.incrementUriNumber = (count) => {
    let on, nn, match = location.href.match(/(.*?)(\d+)(\D*)$/);
    if (match) {
        on = match[2];
        nn = String(Math.max(parseInt(on) + count, 0));
        /* keep prepending zeros */
        if (/^0/.test(on)) {
            while (nn.length < on.length) {
                nn = "0" + nn;
            }
        }
        match[2] = nn;

        location.href = match.slice(1).join("");
    }
};

VimbToplevel.applyScrollObservers = (pageId, serializer) => {
    let doc = document;

    let onScroll = () => {
        let body    = doc.body,
            de      = doc.documentElement,
            percent = 0,
            fromTop = 0,
            max;

        if (!body || !de) {
            return;
        }

        let scrollTop    = Math.max(de.scrollTop, body.scrollTop),
            clientHeight = window.innerHeight,
            scrollHeight = Math.max(de.scrollHeight, body.scrollHeight);

        /* Get the maximum scrollable page size. This is the size of the */
        /* whole document - height of the viewport. */
        max = scrollHeight - clientHeight;
        if (max > 0) {
            percent = Math.round(scrollTop * 100 / max);
            fromTop = scrollTop;
        }

        window.webkit.messageHandlers.verticalScroll.postMessage(
            serializer(pageId, max, percent, fromTop)
        );
    };

    doc.addEventListener('scroll', onScroll);
    onScroll();
};
