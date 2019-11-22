
var ws=null;

var connect_to_kernel = function() {
    if(ws!=null) return;
    
    let url="ws:"+window.location.origin.substr(window.location.protocol.length)+"/ws";
    console.log(url);
    ws = new WebSocket(url);
    ws.onopen = function() {
        console.log("Connection to Cadabra server open.");
//        ws.send("test");
    };
    ws.onmessage = function(msg) {
        console.log("Received message from server.");
        console.log(msg);
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
