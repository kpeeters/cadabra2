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


