const configModule = require("./config.js");
const RedisModule = require("ioredis");

const redisClient = new RedisModule({
    host: configModule.redisHost,
    port: configModule.redisPort,
    password: configModule.redisPwd
});

redisClient.on("error", function(){
    console.log("Redis Connect Error");
    redisClient.quit();
});


async function Get(key)
{
    try
    {
        const result = await redisClient.get(key);
        if(result == null)
        {
            console.log("result:<", result, "> key:<", key, "> not found");
            return null;
        }
        console.log("result:<", result, "> get key:<", key, "> success");
        return result;
    }
    catch(error)
    {
        console.log("command Get error, error is:", error);
        return null;
    }
}

async function Exists(key)
{
    try
    {
        const result = await redisClient.exists(key);
        if(result == null)
        {
            console.log("result:<", result, "> key:<", key, "> not found");
            return false;
        }

        console.log("result:<", result, "> key:<", key, "> exists");
        return true;
    }
    catch(error)
    {
        console.log("command Exists error, error is:", error);
        return false;
    }
}

async function SetValueAndExpire(key, value, expireTime)
{
    try
    {
        // 二者应该要同一事件
        await redisClient.set(key, value);
        await redisClient.expire(key, expireTime);
        return true
    }
    catch(error)
    {
        console.log("command SetValueAndExpire error, error is:", error);
        return false;
    }
}

function Quit()
{
    redisClient.quit();
}

module.exports = {Get, Exists, SetValueAndExpire, Quit};