#include "NewRouter.h"

#include <iostream>

using namespace NEW;


Router route() {
    Router r1;
    
    r1.use("/", [] (const Request& req, const Response& res, const std::function<void(void)>& next) -> void {
        std::cout << "r1.use(\"/\")" << std::endl;
        next();
    });
    
    Router r2;
    r2.get("/hallo", [] (const Request& req, const Response& res) -> void {
        std::cout << "r2.get(\"/hallo\")" << std::endl;
    });
    
    r1.use("/test", r2);
    
    return r1;
}



int main(int argc, char* argv[]) {
    Router r(route());
    Router r1;
    r1 = r;
    
    r1.dispatch("get", Request(argv[1]), Response());
    
    
    return 0;
}

