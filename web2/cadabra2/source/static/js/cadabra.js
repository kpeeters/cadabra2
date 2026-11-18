// Reference manual (man.html): hide entries which do not match the
// text in the filter box, and topics left without visible entries.
var manFilter = document.getElementById("man-filter");
if(manFilter) {
    var applyFilter = function() {
        var words = manFilter.value.toLowerCase().split(/\s+/).filter(Boolean);
        var entries = document.querySelectorAll(".man-entry");
        var shown = 0;
        for (var j = 0; j < entries.length; j++) {
            var text = entries[j].getAttribute("data-search");
            var match = words.every(function(w) { return text.indexOf(w) >= 0; });
            entries[j].hidden = !match;
            if(match) shown++;
        }
        var topics = document.querySelectorAll(".man-topic");
        for (var j = 0; j < topics.length; j++)
            topics[j].hidden = !topics[j].querySelector(".man-entry:not([hidden])");
        var filtering = words.length > 0;
        var az = document.getElementById("a-z");
        if(az) az.hidden = filtering;
        // Hide sections without matches, and the pills linking to them.
        var sections = document.querySelectorAll(".man-section, #a-z");
        for (var j = 0; j < sections.length; j++) {
            var sec = sections[j];
            if(sec.classList.contains("man-section"))
                sec.hidden = !sec.querySelector(".man-entry:not([hidden])");
            var pills = document.querySelectorAll('.chips a[href="#' + sec.id + '"]');
            for (var k = 0; k < pills.length; k++)
                pills[k].hidden = sec.hidden;
        }
        document.querySelector(".man-noresults").hidden = !filtering || shown > 0;
    };
    manFilter.addEventListener("input", applyFilter);
    applyFilter();
}

// Notebook pages (tutorials, manual): a copy-to-clipboard button on
// every input cell.
var copyIcon = '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.4" aria-hidden="true">'
    + '<rect x="5.5" y="5.5" width="8" height="8" rx="1.5"/><path d="M3.5 10.5h-1v-8h8v1"/></svg>';
var doneIcon = '<svg viewBox="0 0 16 16" fill="none" stroke="currentColor" stroke-width="1.8" aria-hidden="true">'
    + '<path d="M3 8.5l3 3 7-7"/></svg>';

function copyText(text) {
    if(navigator.clipboard && window.isSecureContext)
        return navigator.clipboard.writeText(text);
    // Fallback for pages viewed over plain http or from a file.
    return new Promise(function(resolve, reject) {
        var area = document.createElement("textarea");
        area.value = text;
        area.style.position = "fixed";
        area.style.opacity = "0";
        document.body.appendChild(area);
        area.select();
        var ok = document.execCommand("copy");
        document.body.removeChild(area);
        ok ? resolve() : reject();
    });
}

var sources = document.querySelectorAll("div.python > div.source");
for (var i = 0; i < sources.length; i++) {
    (function(source) {
        var code = source.textContent.replace(/\s+$/, "");
        if(code === "")
            return;
        var button = document.createElement("button");
        button.type = "button";
        button.className = "copy-code";
        button.title = "Copy to clipboard";
        button.setAttribute("aria-label", "Copy to clipboard");
        button.innerHTML = copyIcon;
        button.addEventListener("click", function() {
            copyText(code).then(function() {
                button.classList.add("copied");
                button.innerHTML = doneIcon;
                button.title = "Copied";
                setTimeout(function() {
                    button.classList.remove("copied");
                    button.innerHTML = copyIcon;
                    button.title = "Copy to clipboard";
                }, 1500);
            });
        });
        source.appendChild(button);
    })(sources[i]);
}

// Close open menus in the top navigation bar when clicking outside
// them, or when pressing Escape.
function closeMenus(except) {
    var open = document.querySelectorAll(".mainnav details[open]");
    for (var j = 0; j < open.length; j++) {
        if(open[j] !== except)
            open[j].removeAttribute("open");
    }
}
document.addEventListener("click", function(ev) {
    closeMenus(ev.target.closest(".mainnav details"));
});
document.addEventListener("keydown", function(ev) {
    if(ev.key === "Escape")
        closeMenus(null);
});
