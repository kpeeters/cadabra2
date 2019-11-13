
var ws=null;

$(document).ready( function() {
    ws = new WebSocket("ws://localhost:35831");
    ws.onopen = function() {
        console.log("Connection to Cadabra server open.");
    };
    ws.onmessage = function() {
        console.log("Received message from server.");
    };
    ws.onclose = function() {
        console.log("Connection to Cadabra server closed.");
    };
    ws.onerror = function() {
        console.log("Error in connection to Cadabra server.");
    };
});
