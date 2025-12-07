var vbscroll = (function() {
    var w = window,
        d = document,
        origin = {x: 0, y: 0},
        target = {x: 0, y: 0},
        previousScrollTime = 0,
        previousTarget = {x: 0, y: 0},
        previousScrollDuration = 0,
        offset = 0;

    return function (mode, scrollStep, count, smooth) {
        var ph = w.innerHeight,
            de = d.documentElement,
            c = count||1,
            maxY = Math.max(
                d.body.scrollHeight, de.scrollHeight,
                d.body.offsetHeight, de.offsetHeight,
                d.body.clientHeight, de.clientHeight
            );

        if (smooth && performance.now() - previousScrollTime < previousScrollDuration) {
            origin.x = previousTarget.x;
            origin.y = previousTarget.y;
        } else {
            origin.x = w.scrollX;
            origin.y = w.scrollY;
        }

        target.x = origin.x;
        target.y = origin.y;

        switch (mode) {
            case 'j':
                target.y += c * scrollStep;
                break;
            case 'h':
                target.x -= c * scrollStep;
                break;
            case 'k':
                target.y -= c * scrollStep;
                break;
            case 'l':
                target.x += c * scrollStep;
                break;
            case '\x04': /* ^D */
                target.y += c * ph / 2;
                break;
            case '\x15': /* ^U */
                target.y -= c * ph / 2;
                break;
            case '\x06': /* ^F */
                target.y += c * ph;
                break;
            case '\x02': /* ^B */
                target.y -= c * ph;
                break;
            case 'G': /* fall through - gg and G differ only in y value when no count is given */
            case 'g':
                if (count) {
                    target.y = (d.documentElement.scrollHeight - ph) * c / 100;
                } else {
                    target.y = mode == 'G' ? maxY : 0;
                }
                break;
            case '0':
                target.x = 0;
                break;
            case '$':
                target.x = d.body.scrollWidth;
                break;
            default:
                return 1;
        }

        target.x = Math.max(0, Math.min(d.body.scrollWidth, target.x));
        target.y = Math.max(0, Math.min(maxY, target.y));

        offset = Math.max(
            Math.abs(target.x - origin.x),
            Math.abs(target.y - origin.y),
        );

        if (target.x == origin.x && target.y == origin.y) {
            return 0;
        }

        previousTarget.x = target.x;
        previousTarget.y = target.y;
        previousScrollDuration = Math.min(offset, 100);
        previousScrollTime = performance.now();

        w.scrollTo({
            top: target.y,
            left: target.x,
            behavior: smooth ? 'smooth' : 'instant',
        });
        return 0;
    };
})();
