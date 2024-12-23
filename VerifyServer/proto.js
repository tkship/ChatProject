const path = require("path");
const grpc = require("@grpc/grpc-js");
const protoLoader = require("@grpc/proto-loader");

const protoPath = path.join(__dirname, "message.proto");
const packageDefinition = protoLoader.loadSync(protoPath, 
    {keepCase:true, longs:String, enums:String, defaults:true, oneofs:true});

const protoDescriptor = grpc.loadPackageDefinition(packageDefinition);
const messageProto = protoDescriptor.message;

module.exports = messageProto;