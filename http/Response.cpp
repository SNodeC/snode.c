#include "Response.h"
#include "HTTPContext.h"
#include "HTTPStatusCodes.h"


Response::Response(HTTPContext* httpContext) : httpContext(httpContext) {
}


Response::~Response() {
    this->httpContext->reset();
}


void Response::status(int status) const {
    this->httpContext->responseStatus = status;
}


void Response::set(const std::string& field, const std::string& value) const {
    this->httpContext->responseHeader.insert({field, value});
}

/*
void Response::append(const std::string& field, const std::string& value) const {
    this->httpContext->responseHeader[field] = httpContext->responseHeader[field] + ", " + value;
}
*/


void Response::cookie(const std::string& name, const std::string& value, const std::string& op1, 
                      const std::string& op2, const std::string& op3, const std::string& op4,
                      const std::string& op5, const std::string& op6, const std::string& op7) const {
    
    std::string parameters = value + setOptions(op1,op2,op3,op4,op5,op6,op7);
    this->httpContext->responseCookies.insert({name, parameters});
}

void Response::send(const std::string& text) const {
    this->httpContext->send(text);
}


void Response::sendFile(const std::string& file) const {
    this->httpContext->sendFile(file, 0);
}


void Response::sendFile(const std::string& file, const std::function<void (int err)>& fn) const {
    this->httpContext->sendFile(file, fn);
}


void Response::send(const char* puffer, int n) const {
    this->httpContext->send(puffer, n);
}


void Response::redirect(const std::string& name) const {
    this->redirect(302, name);
}


void Response::redirect(int status, const std::string& name) const {
    this->status(status);
    this->set("Location", name);
    this->end();
}


void Response::end() const {
    this->httpContext->end();
}
std::string Response::setOptions(const std::string& op1,const std::string& op2,const std::string& op3,
                const std::string& op4,const std::string& op5,
                const std::string& op6,const std::string& op7)const {
       
    std::string output = "";
    
    if(op1 != " "){
        output += ";" + op1 + ";" ;
    }
    if(op2 != " "){
        output +=  op2 + ";" ;
    }
    if(op3 != " "){
        output +=  op3 + ";" ;
    }
    if(op4 != " "){
        output +=  op4 + ";" ;
    }
    if(op5 != " "){
        output +=  op5 + ";" ;
    }
    if(op6 != " "){
        output +=  op6 + ";" ;
    }
    if(op7 != " "){
        output +=  op7 + ";" ;
    }
    
    return output;
    
}
                
                
                
                
                
                
                
                
                
                
                
