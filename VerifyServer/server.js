const grpcModule = require("@grpc/grpc-js")
const { v4 : uuidv4 } = require("uuid")
const messageProto = require("./proto")
const configModule = require("./config")
const mailModule = require("./email")
const constModule = require("./const")
const RedisModule = require("./redis")

async function GetVerifyCode(call, callback){
    console.log("mail is ", call.request.email);
    try{
        let getResult = await RedisModule.Get(call.request.email);
        console.log("Get result is ", getResult);
        let uniqueId = getResult;
        if(getResult == null)
        {
            uniqueId = uuidv4();
            if(uniqueId.length > 4)
            {
                uniqueId = uniqueId.substring(0,4);
            }
            console.log("uniqueId: " + uniqueId);

            let ok = await RedisModule.SetValueAndExpire(call.request.email, uniqueId, 60);
            if(!ok)
            {
                callback(null, {
                    email: call.request.email,
                    error: constModule.Errors.RedisErr
                });
                return;
            }
        }
        
        let mailContent = "您的验证码为: " + uniqueId + ", 请三分钟内完成注册";

        let mailOptions = {
            from: configModule.emailUser,
            to: call.request.email,
            subject: "验证码",
            text: mailContent
        };

        let sendRes = await mailModule.SendMail(mailOptions);
        console.log("sendRes: " + sendRes);

        callback(null, {
            email: call.request.email,
            error: constModule.Errors.Success
        });
    }
    catch(error){
        console.log("error: " + error);

        callback(null, {
            email: call.request.email,
            error: constModule.Errors.Exception
        });
    }
}


function main(){
    var server = new grpcModule.Server();
    server.addService(messageProto.VerifyService.service, {GetVerifyCode: GetVerifyCode});
    server.bindAsync("127.0.0.1:4035", grpcModule.ServerCredentials.createInsecure(), () => {
        // server.start();
        console.log("grpc server started");
    });
}

main();