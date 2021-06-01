const AWS = require("aws-sdk");
module.exports.dynamoDocClient = new AWS.DynamoDB.DocumentClient({ region: "us-east-1" });
module.exports.s3 = new AWS.S3();
