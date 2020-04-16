#include "Response.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <iostream>

#include "AcceptedSocket.h"

Response::Response(AcceptedSocket* as) : acceptedSocket(as), stat(200), headerSent(false) {
    headerMap["Content-Type"] = "text/html; charset=UTF8";
    
    statusCode[100]	= "Continue";
    statusCode[101] = "Switching Protocols";
    statusCode[102] = "Processing";
    statusCode[103] = "Early Hints";
    statusCode[200] = "OK";
    statusCode[201] = "Created";
    statusCode[202] = "Accepted";
    statusCode[203] = "Non-Authoritative Information";
    statusCode[204] = "No Content";
    statusCode[205] = "Reset Content";
    statusCode[206] = "Partial Content";
    statusCode[207] = "Multi-Status";
    statusCode[208] = "Already Reported";
    statusCode[226] = "IM Used";
    statusCode[300] = "Multiple Choices";
    statusCode[301] = "Moved Permanently";
    statusCode[302] = "Found";
    statusCode[303] = "See Other";
    statusCode[304] = "Not Modified";
    statusCode[305] = "Use Proxy";
    statusCode[307] = "Temporary Redirect";
    statusCode[308] = "Permanent Redirect";
    statusCode[400] = "Bad Request";
    statusCode[401] = "Unauthorized";
    statusCode[402] = "Payment Required";
    statusCode[403] = "Forbidden";
    statusCode[404] = "Not Found";
    statusCode[405] = "Method Not Allowed";
    statusCode[406] = "Not Acceptable";
    statusCode[407] = "Proxy Authentication Required";
    statusCode[408] = "Request Timeout";
    statusCode[409] = "Conflict";
    statusCode[410] = "Gone";
    statusCode[411] = "Length Required";
    statusCode[412] = "Precondition Failed";
    statusCode[413] = "Payload Too Large";
    statusCode[414] = "URI Too Long";
    statusCode[415] = "Unsupported Media Type";
    statusCode[416] = "Range Not Satisfiable";
    statusCode[417] = "Expectation Failed";
    statusCode[421] = "Misdirected Request";
    statusCode[422] = "Unprocessable Entity";
    statusCode[423] = "Locked";
    statusCode[424] = "Failed Dependency";
    statusCode[425] = "Too Early";
    statusCode[426] = "Upgrade Required";
    statusCode[427] = "Unassigned";
    statusCode[428] = "Precondition Required";
    statusCode[429] = "Too Many Requests";
    statusCode[430] = "Unassigned";
    statusCode[431] = "Request Header Fields Too Large";
    statusCode[451] = "Unavailable For Legal Reasons";
    statusCode[500] = "Internal Server Error";
    statusCode[501] = "Not Implemented";
    statusCode[502] = "Bad Gateway";
    statusCode[503] = "Service Unavailable";
    statusCode[504] = "Gateway Timeout";
    statusCode[505] = "HTTP Version Not Supported";
    statusCode[506] = "Variant Also Negotiates";
    statusCode[507] = "Insufficient Storage";
    statusCode[508] = "Loop Detected";
    statusCode[510] = "Not Extended";
    statusCode[511] = "Network Authentication Required";
}

void Response::sendHeader() {
    if (!headerSent) {
        acceptedSocket->writeLn("HTTP/1.1 " + std::to_string(stat) + " " + statusCode[stat]);
    
        for (std::map<std::string, std::string>::iterator it = headerMap.begin(); it != headerMap.end(); ++it) {
            acceptedSocket->writeLn((*it).first + ": " + (*it).second);
        }
        acceptedSocket->writeLn();
        headerSent = true;
    }
}

void Response::send(const std::string& text) {
    headerMap["Content-Length"] = std::to_string(text.length());
    sendHeader();
    acceptedSocket->write(text);
}


void Response::sendFile(const std::string& file) {
    sendHeader();
    acceptedSocket->sendFile(file);
}


void Response::send(const char* buffer, int n) {
    sendHeader();
    acceptedSocket->write(buffer, n);
}


void Response::end() {
    acceptedSocket->close();
    headerSent = false;
    headerMap.clear();
    headerMap["Content-Type"] = "text/html; charset=UTF8";
    stat = 200;
}
