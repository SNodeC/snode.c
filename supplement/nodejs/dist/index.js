const express = require("express");
var app = express();
app.get('/:file', (req, res) => {
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/doc/html/" + req.params.file);
});
app.get('/search/:file', (req, res) => {
    http: //localhost:8088/search/index.html
     console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/doc/html/search/" + req.params.file);
});
app.listen(8088, (...args) => {
    console.log("node.js listening on port 8088");
});
//# sourceMappingURL=index.js.map