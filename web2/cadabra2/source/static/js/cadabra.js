var x = document.getElementsByClassName("folding");
var i;
for (i = 0; i < x.length; i++) {
    console.log(x[i]);
    x[i].onclick = function(el) {
        if(el.currentTarget.classList.contains('uncovered')) {
            el.currentTarget.classList.remove('uncovered');
        } else {
	    el.currentTarget.classList.add('uncovered');
        }
    }
}


