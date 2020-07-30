#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "Logger.h"
#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <cstring>
#include <iostream>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c.key.encrypted.pem"
#define KEYFPASS "snode.c"

int testPost() {
    legacy::WebApp legacyApp("/home/voc/projects/ServerVoc/build/html");

    legacyApp.get("/", []([[maybe_unused]] Request& req, Response& res) -> void {
        res.send("<html>"
                 "<head>"
                 "<style>"
                 "main {"
                 "min-height: 30em;"
                 "padding: 3em;"
                 "background-image: repeating-radial-gradient( circle at 0 0, #fff, #ddd 50px);"
                 "}"
                 "input[type=\"file\"] {"
                 "display: block;"
                 "margin: 2em;"
                 "padding: 2em;"
                 "border: 1px dotted;"
                 "}"
                 "</style>"
                 "</head>"
                 "<body>"
                 "<h1>Datei-Upload mit input type=\"file\"</h1>"
                 "<main>"
                 "<h2>Schicken Sie uns was Schickes!</h2>"
                 "<form method=\"post\" enctype=\"multipart/form-data\">"
                 "<label> Wählen Sie eine Textdatei (*.txt, *.html usw.) von Ihrem Rechner aus."
                 "<input name=\"datei\" type=\"file\" size=\"50\" accept=\"text/*\">"
                 "</label>"
                 "<button>… und ab geht die Post!</button>"
                 "</form>"
                 "</main>"
                 "</body>"
                 "</html>");
    });

    legacyApp.post("/", [](Request& req, Response& res) -> void {
        std::cout << "Content-Type: " << req.header("Content-Type") << std::endl;
        std::cout << "Content-Length: " << req.header("Content-Length") << std::endl;
        char* body = new char[std::stoul(req.header("Content-Length")) + 1];
        memcpy(body, req.body, std::stoul(req.header("Content-Length")));
        body[std::stoi(req.header("Content-Length"))] = 0;

        std::cout << "Body: " << std::endl;
        std::cout << body << std::endl;
        res.send("<html>"
                 "<body>"
                 "<h1>Thank you</h1>"
                 "</body>"
                 "</html>");
        delete[] body;
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    tls::WebApp tlsWebApp("/home/voc/projects/ServerVoc/build/html", CERTF, KEYF, KEYFPASS);
    WebApp::clone(tlsWebApp, legacyApp);

    tlsWebApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    WebApp::start();

    return 0;
}

int main(int argc, char** argv) {
    WebApp::init(argc, argv);

    return testPost();
}
