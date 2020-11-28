const express = require("express");


var app = express();


app.get('/:file', (req, res) => {   // http://localhost:8081/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/docs/html/" + req.params.file);
});


app.get('/search/:file', (req, res) => {     http://localhost:8081/search/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/docs/html/search/" + req.params.file);
});


app.listen(8081, () => {
    console.log("node.js listening on port 8081");
})
