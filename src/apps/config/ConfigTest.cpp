#include "utils/CLI11.hpp"

#include <iostream>
#include <libgen.h>
#include <stdexcept>
#include <string>
#include <vector>

class ServerConfig {
public:
    void printConfig() {
        std::cout << "Name = " << name << std::endl;
        std::cout << "===============================";
        std::cout << "Backlog = " << backlog << std::endl;
        std::cout << "BindInterface = " << bindInterface << std::endl;
        std::cout << "BindPort = " << bindPort << std::endl;
        std::cout << "ReadTimeout = " << readTimeout << std::endl;
        std::cout << "WriteTimeout = " << writeTimeout << std::endl;
    }

    std::string name;
    int backlog;
    std::string bindInterface;
    uint32_t bindPort;
    int readTimeout;
    int writeTimeout;
};

ServerConfig createServerConfig(CLI::App& app, const std::string& name) {
    ServerConfig serverConfig;

    serverConfig.name = name + "_server";

    CLI::App* serverSc = app.add_subcommand(name + "_server", "Configuration of the server");
    serverSc->required();
    serverSc->configurable();

    CLI::Option* serverBacklogOpt = serverSc->add_option("-b,--backlog,backlog", serverConfig.backlog, "Server listen backlog");
    serverBacklogOpt->type_name("[backlog]");
    serverBacklogOpt->default_val(5);
    serverBacklogOpt->configurable();

    CLI::App* serverBindSc = serverSc->add_subcommand("bind");
    serverBindSc->description("Server socket bind options");
    serverBindSc->required();
    serverBindSc->configurable();

    CLI::Option* bindServerHostOpt = serverBindSc->add_option("-a,--host,host", serverConfig.bindInterface, "Bind host name or IP address");
    bindServerHostOpt->type_name("[hostname|ip]");
    bindServerHostOpt->default_val("0.0.0.0");
    //    bindServerHostOpt->required(REQUIRED);
    bindServerHostOpt->configurable();

    CLI::Option* bindServerPortOpt = serverBindSc->add_option("-p,--port,port", serverConfig.bindPort, "Bind port number");
    bindServerPortOpt->type_name("[port number]");
    bindServerPortOpt->default_val(0);
    bindServerPortOpt->required();
    bindServerPortOpt->configurable();

    CLI::App* serverConnectionSc = serverSc->add_subcommand("connection");
    serverConnectionSc->description("Options for established client connections");
    serverConnectionSc->configurable();

    CLI::Option* serverConnectionReadTimeoutOpt =
        serverConnectionSc->add_option("-r,--readtimeout,readtimeout", serverConfig.readTimeout, "Read timeout");
    serverConnectionReadTimeoutOpt->type_name("[sec]");
    serverConnectionReadTimeoutOpt->default_val(60);
    serverConnectionReadTimeoutOpt->configurable();

    CLI::Option* serverConnectionWriteTimeoutOpt =
        serverConnectionSc->add_option("-w,--writetimeout,writetimeout", serverConfig.writeTimeout, "Write timeout");
    serverConnectionWriteTimeoutOpt->type_name("[sec]");
    serverConnectionWriteTimeoutOpt->default_val(60);
    serverConnectionWriteTimeoutOpt->configurable();

    return serverConfig;
}

class ClientConfig {
public:
    void printConfig() {
        std::cout << "Name = " << name << std::endl;
        std::cout << "===============================";
        std::cout << "ServerAddress = " << serverAddress << std::endl;
        std::cout << "ServerPort = " << serverPort << std::endl;
        std::cout << "BindInterface = " << bindInterface << std::endl;
        std::cout << "BindPort = " << bindPort << std::endl;
        std::cout << "ReadTimeout = " << readTimeout << std::endl;
        std::cout << "WriteTimeout = " << writeTimeout << std::endl;
    }

    std::string name;
    std::string serverAddress;
    uint32_t serverPort;
    std::string bindInterface;
    uint32_t bindPort;
    int readTimeout;
    int writeTimeout;
};

ClientConfig createClientConfig(CLI::App& app, const std::string& name) {
    ClientConfig clientConfig;

    clientConfig.name = name + "_client";

    CLI::App* clientSc = app.add_subcommand(name + "_client", "Configuration of the client");
    clientSc->required();
    clientSc->configurable();

    CLI::App* clientServerSc = clientSc->add_subcommand("server");
    clientServerSc->description("Server address and port");
    clientServerSc->required();
    clientServerSc->configurable();

    CLI::Option* clientServerHostOpt =
        clientServerSc->add_option("-a,--host,host", clientConfig.serverAddress, "Server host name or IP address");
    clientServerHostOpt->type_name("[hostname|ip]");
    clientServerHostOpt->required();
    clientServerHostOpt->configurable();

    CLI::Option* clientServerPortOpt = clientServerSc->add_option("-p,--port,port", clientConfig.serverPort, "Server port number");
    clientServerPortOpt->type_name("[port number]");
    clientServerPortOpt->required();
    clientServerPortOpt->configurable();

    CLI::App* clientConnectionSc = clientSc->add_subcommand("connection");
    clientConnectionSc->description("Options for established client connections");
    clientConnectionSc->configurable();

    CLI::App* clientBindSc = clientSc->add_subcommand("bind");
    clientBindSc->description("Client socket bind options [optional]");
    clientBindSc->configurable();

    CLI::Option* clientBindHostOpt =
        clientBindSc->add_option("-a,--host,host", clientConfig.bindInterface, "Client bind host name or IP address");
    clientBindHostOpt->type_name("[hostname|ip]");
    clientBindHostOpt->default_val("0.0.0.0");
    clientBindHostOpt->configurable();

    CLI::Option* clientBindPortOpt = clientBindSc->add_option("-p,--port,port", clientConfig.bindPort, "Client bind port number");
    clientBindPortOpt->type_name("[port number]");
    clientBindPortOpt->default_val(0);
    clientBindPortOpt->configurable();

    CLI::Option* clientConnectionReadTimeoutOpt =
        clientConnectionSc->add_option("-r,--readtimeout,readtimeout", clientConfig.readTimeout, "Read timeout");
    clientConnectionReadTimeoutOpt->type_name("[sec]");
    clientConnectionReadTimeoutOpt->default_val(60);
    clientConnectionReadTimeoutOpt->configurable();

    CLI::Option* clientConnectionWriteTimeoutOpt =
        clientConnectionSc->add_option("-w,--writetimeout,writetimeout", clientConfig.writeTimeout, "Write timeout");
    clientConnectionWriteTimeoutOpt->type_name("[sec]");
    clientConnectionWriteTimeoutOpt->default_val(60);
    clientConnectionWriteTimeoutOpt->configurable();

    return clientConfig;
}

int app4(int argc, char* argv[]) {
    CLI::App app("Configuration file for application " + std::string(basename(argv[0])));

    app.set_config("--config", std::string(CONFFILEPATH "/") + std::string(basename(argv[0])) + ".conf", "Read an config file", false);

    bool dumpConfig = false;
    CLI::Option* dumpConfigFlg = app.add_flag("-d,--dump-config", dumpConfig, "Dump config file template");
    dumpConfigFlg->configurable(false);

    try {
        app.parse(argc, argv);
    } catch (const CLI::ParseError& e) {
    }

    ServerConfig legacyServerConfig = createServerConfig(app, "legacy");
    ServerConfig tlsServerConfig = createServerConfig(app, "tls");

    ClientConfig legacyClientConfig = createClientConfig(app, "legacy");
    ClientConfig tlsClisntConfig = createClientConfig(app, "tls");

    CLI11_PARSE(app, argc, argv);

    std::cout << app.config_to_str(true, true);

    if (dumpConfig) {
        std::cout << "Dumping config file template: " << std::string(CONFFILEPATH "/") + std::string(basename(argv[0])) + ".conf.template"
                  << std::endl;
        std::ofstream confFile(std::string(CONFFILEPATH "/") + std::string(basename(argv[0])) + ".conf.tmpl");
        confFile << app.config_to_str(true, true);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    app4(argc, argv);
    //    app2(argc, argv);
    return 0;
}
