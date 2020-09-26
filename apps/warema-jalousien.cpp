#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "legacy/WebApp.h"

#include <cstdlib>
#include <easylogging++.h>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

using namespace express;

std::map<std::string, std::string> jalousien = {{"kueche", "kueche"},
                                                {"strasse", "strasse"},
                                                {"esstisch", "esstisch"},
                                                {"balkon", "balkon"},
                                                {"schlafzimmer", "schlafzimmer"},
                                                {"arbeitszimmer", "arbeitszimmer"},
                                                {"comfort", "komfort"},
                                                {"all", "alle"}};

std::map<std::string, std::string> actions = {{"open", "up"}, {"close", "down"}, {"stop", "stop"}};

int main(int argc, char* argv[]) {
    WebApp::init(argc, argv);

    legacy::WebApp webApp;

    webApp.get("/jalousien/:id", [] APPLICATION(req, res) {
        VLOG(0) << "Param: " << req.param("id");
        VLOG(0) << "Qurey: " << req.query("action");

        std::string arguments = "aircontrol -t " + jalousien[req.param("id")] + "_" + actions[req.query("action")];

        int ret = system(arguments.c_str());
        ret = (ret >> 8) & 0xFF;
        /* ret:
               Bits 15-8 = Exit code.
               Bit     7 = 1 if a core dump was produced.
               Bits  6-0 = Signal number that killed the process.
        */

        switch (ret) {
            case 0:
                res.status(200).send("OK: " + arguments);
                break;
            case 127:
                res.status(404).send("ERROR not found: " + arguments);
                break;
        };
    });

    webApp.use([] APPLICATION(req, res) {
        res.status(404).send("No Jalousie specified");
    });

    webApp.listen(8080, [](int err) -> void {
        if (err != 0) {
            PLOG(FATAL) << "listen on port 8080 " << std::to_string(err);
        } else {
            VLOG(0) << "warema-jalousien listening on port 8080 for legacy connections";
        }
    });

    return WebApp::start();
}
