/* TODO maybe it's better to inject this in the webview as method */
/* and to call it if needed */
var c = %d, on, nn, m = location.href.match(/(.*?)(\d+)(\D*)$/);
if (m) {
    on = m[2];
    nn = String(Math.max(parseInt(on) + c, 0));
    /* keep prepending zeros */
    if (/^0/.test(on)) {
        while (nn.length < on.length) {
            nn = "0" + nn;
        }
    }
    m[2] = nn;

    location.href = m.slice(1).join("");
}
