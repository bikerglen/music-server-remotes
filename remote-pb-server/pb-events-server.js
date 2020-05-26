"use strict"

process.title = 'pb-events-server';

var inPort = 6682;
var echoPort = 6683;

var http = require('http');
var webSocketServer = require ('websocket').server;

var clients = [];

var server = http.createServer (function (request, response) {

	let body = '';

	request.on ('data', chunk => {
		body += chunk.toString ();
	});

	request.on ('end', () => {
		console.log (body);
		response.end ('OK');
        for (var i=0; i < clients.length; i++) {
			clients[i].sendUTF (body);
        }
	});

}).listen (inPort);


var server2 = http.createServer (function (request, response) {
});

server2.listen (echoPort, function () {
});

var wsServer = new webSocketServer ({
	httpServer: server2,
	keepalive: false
});

wsServer.on ('request', function (request) {

	console.log((new Date()) + ' Connection from origin ' + request.origin + '.');

	var connection = request.accept(null, request.origin);
	var index = clients.push (connection) - 1;

	connection.on ('close', function (connection) {
		console.log((new Date()) + " Peer " + connection.remoteAddress + " disconnected.");
		clients.splice(index, 1);
	});

});
