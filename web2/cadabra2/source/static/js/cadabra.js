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
        document.querySelector(".man-noresults").hidden = !filtering || shown > 0;
    };
    manFilter.addEventListener("input", applyFilter);
    applyFilter();
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
