#!/usr/bin/env node

// this code loads the proto file and compiles it at runtime.

var protobuf = require("protobufjs");
var Long = require("long");
var MyMessage = null;

// note protobuf.load() returns a promise while loadSync "does it NOW"
var mypackage = protobuf.loadSync("mypackage.proto");
MyMessage = mypackage.lookupType("mypackage.MyMessage");

var payload = { value: Long.fromValue('9223372036854775807', true) };
var errMsg = MyMessage.verify(payload);
if (errMsg)
    throw Error(errMsg);

var message = MyMessage.create(payload); // or use .fromObject if conversion is necessary
console.log(message);

// var buffer = MyMessage.encodeDelimited(message).finish();
var buffer = MyMessage.encode(message).finish();
console.log(buffer);

var message = MyMessage.decode(buffer);
// var message = MyMessage.decodeDelimited(buffer);
console.log(message);
