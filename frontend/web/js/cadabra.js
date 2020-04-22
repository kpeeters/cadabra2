
var ws=null;
var token="";

var evaluate_cell = function(txt) {
    req = {};
    header = {};
    content = {};
    header["uuid"]="none";
    header["cell_id"]=1;
    header["cell_origin"]="client";
    header["msg_type"]="execute_request";
    content["code"]=txt;
    req["auth_token"]=token;
    req["header"]=header;
    req["content"]=content;
    console.log(req);
    ws.send(JSON.stringify(req));
};

var init_notebook = function() {
    $("#notebook").on('keypress', function(event) {
        if(event.originalEvent.shiftKey && event.originalEvent.charCode==13) {
            event.preventDefault();
            let val = $(event.target).val();
            if(val!="") {
                console.log("Sending "+val);
                evaluate_cell(val);
            }
        }
    });
    add_cell();
};

var add_cell = function() {
    cell_id = Math.floor((Math.random() * 100000) + 1);
    $("<div id='"+cell_id+"' class='cell'></div>")
        .appendTo("#notebook")
        .append("<textarea></textarea>");
    $("#"+cell_id+" textarea").on("keydown keyup", function(){
        this.style.height = "1px";
        this.style.height = (this.scrollHeight+2) + "px"; 
    }).focus();
};

var connect_to_kernel = function() {
    if(ws!=null) return;
    
    let url="ws:"+window.location.origin.substr(window.location.protocol.length)+"/ws";
    console.log(url);
    ws = new WebSocket(url);
    ws.onopen = function() {
        console.log("Connection to Cadabra server open.");
        init_notebook();
    };
    ws.onmessage = function(wsm) {
        console.log("Received message from server.");
        let msg=JSON.parse(wsm.data);
        console.log(msg);
        if(msg["header"]["msg_type"]=="hello") {
            token=msg["header"]["token"];
            console.log(token);
        }
        if(msg["msg_type"]=="latex_view") {
            ltx=msg["content"]["output"];
            cell_id=msg["header"]["cell_id"];
            ltx=ltx.replace("\\begin{dmath*}", "").replace("\\end{dmath*}", "");
            $("#notebook").append("<div id='"+cell_id+"' class='latex'>$$"+ltx+"$$</div>");
            selec="#"+cell_id;
            console.log(selec);
            MathJax.typesetPromise($(selec));
        }
        if(msg["msg_type"]=="output" && msg["header"]["last_in_sequence"])
            add_cell();
    };
    ws.onclose = function() {
        console.log("Connection to Cadabra server closed.");
    };
    ws.onerror = function() {
        console.log("Error in connection to Cadabra server.");
    };
};

$(document).ready( function() {
    $("#start").click( function() {
        connect_to_kernel();
    });
});
