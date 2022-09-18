import express from "express";

var app = express();

app.get('/:file', (req: { originalUrl: any; params: { file: string; }; }, res: { sendFile: (arg0: string) => void; }) => {   // http://localhost:8088/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/doc/html/" + req.params.file);
});


app.get('/search/:file', (req: { originalUrl: any; params: { file: string; }; }, res: { sendFile: (arg0: string) => void; }) => {     http://localhost:8088/search/index.html
    console.log(req.originalUrl);
    res.sendFile("/home/voc/projects/ServerVoc/doc/html/search/" + req.params.file);
});


app.listen(8088, (...args: any[]) => {
    console.log("node.js listening on port 8088");
})
