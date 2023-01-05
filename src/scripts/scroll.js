function vbscroll(mode, scrollStep, count) {
    var w = window,
        d = document,
        x = y = 0,
        ph = w.innerHeight,
        de = d.documentElement,
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
                y = c * ((d.documentElement.scrollHeight - ph) / 100);
            } else {
                y = 'G' == mode
                    ? Math.max(
                        d.body.scrollHeight, de.scrollHeight,
                        d.body.offsetHeight, de.offsetHeight,
                        d.body.clientHeight, de.clientHeight
                    )
                    : 0;
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
}
