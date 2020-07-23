import express from 'express';


let app = express();


app.get('/:file', (req, res) => {   // http://localhost:8088/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/build/html/" + req.params.file);
});


app.get('/search/:file', (req, res) => {     http://localhost:8088/search/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/build/html/search/" + req.params.file);
});


app.listen(8088, (...args: any[]) => {
    console.log("node.js listening on port 8088");
})
