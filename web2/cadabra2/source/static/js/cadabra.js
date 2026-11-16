var x = document.querySelectorAll(".folding .package-name");
var i;
for (i = 0; i < x.length; i++) {
    console.log(x[i]);
    x[i].onclick = function(el) {
        if(el.currentTarget.parentElement.classList.contains('uncovered')) {
	    el.currentTarget.parentElement.classList.remove('uncovered');
        } else {
	    el.currentTarget.parentElement.classList.add('uncovered');
        }
    }
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
