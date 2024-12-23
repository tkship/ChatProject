const fs = require("fs");

let config = JSON.parse(fs.readFileSync("config.json", "utf-8"));

let emailUser = config.email.user;
let emailPass = config.email.pass;
let redisHost = config.redis.host;
let redisPort = config.redis.port;
let redisPwd = config.redis.password;

module.exports = {emailUser, emailPass, redisHost, redisPort, redisPwd};