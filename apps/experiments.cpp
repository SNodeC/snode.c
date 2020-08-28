/*
 * snode.c - a slim toolkit for network communication
 * Copyright (C) 2020  Volker Christian <me@vchrist.at>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "HTTPRequestParser.h"
#include "HTTPResponseParser.h"
#include "Logger.h"
#include "legacy/WebApp.h"
#include "tls/WebApp.h"

#include <easylogging++.h>
#include <iostream>
#include <unistd.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define CERTF "/home/voc/projects/ServerVoc/certs/calisto.home.vchrist.at_-_snode.c_-_server.pem"
#define KEYF "/home/voc/projects/ServerVoc/certs/Volker_Christian_-_Web_-_snode.c_-_server.key.encrypted.pem"
#define KEYFPASS "snode.c"

using namespace express;

Router route() {
    Router router;
    router.use("/", [] MIDDLEWARE(req, res, next) {
        VLOG(3) << Logger::INFO << "Middleware 1 " + req.originalUrl;
        VLOG(3) << "Cookie 1: " + req.cookie("searchcookie");

        next();

        VLOG(0) << "Queries: " << req.query("query");

        req.setAttribute<std::string>("Hallo");
        req.setAttribute<std::string, "Key1">("World");
        req.setAttribute<int>(3);

        req.setAttribute<std::string, "Hall">("juhu");
        VLOG(3) << "####################### " + req.getAttribute<std::string, "Hall">([](const std::string& attr) -> void {
            VLOG(3) << "String: --------- " + attr;
        });

        if (!req.getAttribute<std::string>([](std::string& hello) -> void {
                VLOG(3) << "String: --------- " + hello;
            })) {
            VLOG(3) << "++++++++++ Attribute String not found";
        }

        req.getAttribute<std::string, "Key1">(
            [](std::string& world) -> void {
                VLOG(3) << "String + Key1: --------- " + world;
            },
            [](const std::string& type) {
                VLOG(3) << "++++++++++ Attribute " + type + " not found";
            });

        if (!req.getAttribute<int>([](int& i) -> void {
                VLOG(3) << "Int: --------- " + std::to_string(i);
            })) {
            VLOG(3) << "++++++++++ Attribute int not found";
        }

        req.getAttribute<float>(
            [](float& f) -> void {
                VLOG(3) << "Float: --------- " + std::to_string(f);
            },
            [](const std::string& type) {
                VLOG(3) << "++++++++++ Attribute " + type + " not found";
            });
    });

    Router r;
    r.use([] MIDDLEWARE(req, res, next) {
        VLOG(3) << "Middleware 2 " + req.originalUrl;
        VLOG(3) << "Cookie 2: " + req.cookie("searchcookie");

        next();
    });

    router.use("/", r);

    router.get("/search", [] APPLICATION(req, res) {
        VLOG(0) << "URL: " + req.originalUrl;
        VLOG(2) << "SearchCookie: " + req.cookie("searchcookie");
        res.sendFile("/home/voc/projects/ServerVoc/doc/html" + req.url, [&req](int ret) -> void {
            if (ret != 0) {
                PLOG(ERROR) << req.originalUrl;
            }
        });
    });

    return router;
}

tls::WebApp sslMain() {
    tls::WebApp sslApp(CERTF, KEYF, KEYFPASS);
    sslApp
        .use("/",
             [] MIDDLEWARE(req, res, next) {
                 res.set("Connection", "Keep-Alive");
                 next();
             })
        .get("/", route())
        .get("/", [] APPLICATION(req, res) {
            std::string uri = req.originalUrl;
            VLOG(0) << "URL: " + uri;
            VLOG(2) << "RootCookie: " + req.cookie("rootcookie");

            res.cookie("searchcookie", "searchcookievalue", {{"Max-Age", "3600"}, {"Path", "/search"}});
            if (req.cookie("rootcookie") == "rootcookievalue") {
                VLOG(2) << "Clear rootcookie";
                res.clearCookie("rootcookie");
            } else {
                VLOG(2) << "Set rootcookie";
                res.cookie("rootcookie", "rootcookievalue", {{"Max-Age", "3600"}, {"Path", "/"}});
            }

            if (uri == "/") {
                res.redirect("/index.html");
            } else if (uri == "/end") {
                res.send("Bye, bye!\n");
                WebApp::stop();
            } else {
                res.sendFile("/home/voc/projects/ServerVoc/doc/html" + uri, [uri](int ret) -> void {
                    if (ret != 0) {
                        PLOG(ERROR) << uri;
                    }
                });
            }
        });

    return sslApp;
}

legacy::WebApp legacyMain() {
    legacy::WebApp legacyApp;
    legacyApp.use("/", [] MIDDLEWARE(req, res, next) {
        if (req.originalUrl == "/end") {
            res.send("Bye, bye!\n");
            WebApp::stop();
        } else {
            VLOG(1) << "Redirect: https://calisto.home.vchrist.at:8088" + req.originalUrl;
            res.redirect("https://calisto.home.vchrist.at:8088" + req.originalUrl);
        }
    });

    return legacyApp;
}

int simpleWebserver() {
    legacy::WebApp legacyApp(legacyMain());

    tls::WebApp sslApp(sslMain());

    sslApp.listen(8088, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8088";
        } else {
            VLOG(0) << "snode.c listening on port 8088 for SSL/TLS connections";
        }
    });

    legacyApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080";
        } else {
            VLOG(0) << "snode.c listening on port 8080 for legacy connections";
        }
    });

    //    daemon(0, 0);

    WebApp::start();

    return 0;
}

int main([[maybe_unused]] int argc, [[maybe_unused]] char** argv) {
    std::string http = "GET /admin/new/index.html?hihihi=3343&query=2324#fragment HTTP/1.1\r\n"
                       "Field1: Value1\r\n"
                       "Field2: Field2\r\n"
                       "Field2: Field3\r\n" // is allowed and must be combined with a comma as separator
                       "Content-Length: 8\r\n"
                       "Cookie: MyCookie1=MyValue1; MyCookie2=MyValue2\r\n"
                       "Cookie: MyCookie3=MyValue3\r\n"
                       "Cookie: MyCookie4=MyValue4\r\n"
                       "Cookie: MyCookie5=MyValue5; MyCookie6=MyValue6\r\n"
                       "\r\n"
                       "juhuhuhu";

    http::HTTPRequestParser requestParser(
        [](const std::string& method, const std::string& originalUrl, const std::string& fragment, const std::string& httpVersion,
           [[maybe_unused]] const std::map<std::string, std::string>& queries) -> void {
            std::cout << "++ Request: " << method << " " << originalUrl << " "
                      << " " << httpVersion << std::endl;
            for (std::pair<std::string, std::string> query : queries) {
                std::cout << "++    Query: " << query.first << " = " << query.second << std::endl;
            }
            std::cout << "++    Fragment: " << fragment << std::endl;
        },
        [](const std::map<std::string, std::string>& header, const std::map<std::string, std::string>& cookies) -> void {
            for (std::pair<std::string, std::string> headerField : header) {
                std::cout << "++ Header: " << headerField.first << " = " << headerField.second << std::endl;
            }
            for (std::pair<std::string, std::string> cookie : cookies) {
                std::cout << "++ Cookie: " << cookie.first << " = " << cookie.second << std::endl;
            }
        },
        [](char* content, size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++ Content: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        [](void) -> void {
            std::cout << "++ Parsed ++" << std::endl;
        },
        [](int status, const std::string& reason) -> void {
            std::cout << "++ Error: " << status << " : " << reason << std::endl;
        });

    std::cout << "++++++++++++++++++++++++++++++++++" << std::endl;
    std::cout << http << std::endl;
    std::cout << "----------------------------------" << std::endl;

    requestParser.parse(http.c_str(), http.size());
    requestParser.reset();

    std::cout << "++++++++++++++++++++++++++++++++++" << std::endl;
    std::cout << http << std::endl;
    std::cout << "----------------------------------" << std::endl;

    requestParser.parse(http.c_str(), http.size());
    requestParser.reset();

    http::HTTPResponseParser responseParser(
        [](const std::string& httpVersion, const std::string& statusCode, const std::string& reason) -> void {
            VLOG(0) << "HTTP Version: " + httpVersion;
            VLOG(0) << "Status Code: " + statusCode;
            VLOG(0) << "Reason: " + reason;
        },
        [](const std::map<std::string, std::string>& headers, const std::map<std::string, http::ResponseCookie>& cookies) -> void {
            VLOG(0) << "++ Headers";
            for (auto [field, value] : headers) {
                VLOG(0) << "    ++ " << field + " = " + value;
            }

            VLOG(0) << "++ Cookies";
            for (auto [name, cookie] : cookies) {
                VLOG(0) << "    +++ " + name + " = " + cookie.getValue();
                for (auto [option, value] : cookie.getOptions()) {
                    VLOG(0) << "        ++ " + option + " = " + value;
                }
            }
        },
        [](char* content, size_t contentLength) -> void {
            char* strContent = new char[contentLength + 1];
            memcpy(strContent, content, contentLength);
            strContent[contentLength] = 0;
            VLOG(0) << "++ Content: " << contentLength << " : " << strContent;
            delete[] strContent;
        },
        []() -> void {
            VLOG(0) << "++ OnParsed";
        },
        [](int status, const std::string& reason) -> void {
            VLOG(0) << " ++ OnError: " + std::to_string(status) + " - " + reason;
        });

    std::string httpResponse = "HTTP/1.1 200 OK\r\n"
                               "Field: Value\r\n"
                               "Set-Cookie: CookieName = CookieValue; OptionName = OptionValue\r\n"
                               "Set-Cookie: CookieName1 = CookieValue1; OptionName1 = OptionValue1; OptionName2 = OptionValue2;\r\n"
                               "Content-Length: 8\r\n"
                               "\r\n"
                               "juhuhuhu";

    responseParser.parse(httpResponse.c_str(), httpResponse.size());
    responseParser.reset();

    responseParser.parse(httpResponse.c_str(), httpResponse.size());
    responseParser.reset();

    return 0;
    //    WebApp::init(argc, argv);

    //    return simpleWebserver();
}
