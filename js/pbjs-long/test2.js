#!/usr/bin/env node

// this code compiles the proto file offline

var Long = require("long");
var myproto = require("./mypackage.js");
var MyMessage = myproto.mypackage.MyMessage;

var payload = { value: Long.fromValue('9223372036854775807', true) };

var message = MyMessage.create(payload);
console.log(message);

//var buffer = MyMessage.encodeDelimited(message).finish();
var buffer = MyMessage.encode(message).finish();
console.log(buffer);

//var message = MyMessage.decodeDelimited(buffer);
var message = MyMessage.decode(buffer);
console.log(message);
