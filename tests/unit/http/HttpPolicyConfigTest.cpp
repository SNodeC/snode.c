/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 */

#include "support/TestResult.h"
#include "web/http/ConfigHttpParser.h"
#include "web/http/ConfigWebSocket.h"
#include "web/http/client/ConfigHTTP.h"
#include "web/http/server/ConfigHttpServer.h"

#include <cstddef>
#include <string>

int main() {
    tests::support::TestResult testResult;

    {
        web::http::client::ConfigHTTP clientConfig(nullptr);
        web::http::ConfigHttpParser* parserConfig = clientConfig.getParserConfig();

        testResult.expectTrue(parserConfig != nullptr, "HTTP client owns the shared parser configuration section");
        testResult.expectEqual(std::size_t{0}, parserConfig->getMaximumStartLineBytes(), "start-line default is unlimited");
        testResult.expectEqual(std::size_t{8192}, parserConfig->getMaximumHeaderLineBytes(), "header-line default remains 8192");
        testResult.expectEqual(std::size_t{0}, parserConfig->getMaximumHeaderBytes(), "header-section default is unlimited");
        testResult.expectEqual(std::size_t{0}, parserConfig->getMaximumHeaderFields(), "header-field default is unlimited");
        testResult.expectEqual(std::size_t{0}, parserConfig->getMaximumBodyBytes(), "body default is unlimited");

        parserConfig->setMaximumStartLineBytes(101)
            ->setMaximumHeaderLineBytes(102)
            ->setMaximumHeaderBytes(103)
            ->setMaximumHeaderFields(104)
            ->setMaximumBodyBytes(105);
        const web::http::ParserLimits limits = clientConfig.getParserLimits();
        testResult.expectEqual(std::size_t{101}, limits.maximumStartLineBytes, "client exposes configured start-line limit");
        testResult.expectEqual(std::size_t{102}, limits.maximumHeaderLineBytes, "client exposes configured header-line limit");
        testResult.expectEqual(std::size_t{103}, limits.maximumHeaderBytes, "client exposes configured header-section limit");
        testResult.expectEqual(std::size_t{104}, limits.maximumHeaderFields, "client exposes configured header-field limit");
        testResult.expectEqual(std::size_t{105}, limits.maximumBodyBytes, "client exposes configured body limit");

        for (const std::string optionName : {"--maximum-start-line-bytes",
                                             "--maximum-header-line-bytes",
                                             "--maximum-header-bytes",
                                             "--maximum-header-fields",
                                             "--maximum-body-bytes"}) {
            testResult.expectTrue(parserConfig->getOption(optionName) != nullptr, optionName + " is registered as a CLI option");
            testResult.expectTrue(parserConfig->getOption(optionName)->get_configurable(), optionName + " is configuration-file enabled");
        }

        const std::string serialized = clientConfig.configToStr();
        testResult.expectTrue(serialized.find("parser.maximum-start-line-bytes") != std::string::npos,
                              "generated configuration contains the nested start-line key");
        testResult.expectTrue(serialized.find("parser.maximum-body-bytes") != std::string::npos,
                              "generated configuration contains the nested body key");
    }

    {
        web::http::server::ConfigHttpServer serverConfig(nullptr);
        testResult.expectTrue(serverConfig.getParserConfig() != nullptr, "HTTP server owns the shared parser configuration section");
        testResult.expectEqual(std::size_t{0}, serverConfig.getMaximumPendingRequests(), "pending request default is unlimited");
        testResult.expectTrue(serverConfig.getAllowChunkedTransfer(), "chunked requests are allowed by default");
        testResult.expectTrue(serverConfig.getAllowPipelining(), "request pipelining is allowed by default");

        serverConfig.setMaximumPendingRequests(7)->setAllowChunkedTransfer(false)->setAllowPipelining(false);
        const web::http::server::HttpServerPolicy policy = serverConfig.getServerPolicy();
        testResult.expectEqual(std::size_t{7}, policy.maximumPendingRequests, "server exposes configured pending-request limit");
        testResult.expectTrue(!policy.allowChunkedTransfer, "server exposes configured chunked-transfer policy");
        testResult.expectTrue(!policy.allowPipelining, "server exposes configured pipelining policy");

        for (const std::string optionName : {"--maximum-pending-requests", "--allow-chunked-transfer", "--allow-pipelining"}) {
            testResult.expectTrue(serverConfig.getOption(optionName) != nullptr, optionName + " is registered as a CLI option");
            testResult.expectTrue(serverConfig.getOption(optionName)->get_configurable(), optionName + " is configuration-file enabled");
        }
    }

    {
        web::http::ConfigWebSocket webSocketConfig(nullptr);
        testResult.expectEqual(std::size_t{0}, webSocketConfig.getMaximumFrameBytes(), "WebSocket frame default is unlimited");
        testResult.expectEqual(std::size_t{0}, webSocketConfig.getMaximumMessageBytes(), "WebSocket message default is unlimited");
        testResult.expectEqual(std::size_t{0}, webSocketConfig.getMaximumFragments(), "WebSocket fragment default is unlimited");

        webSocketConfig.setMaximumFrameBytes(201)->setMaximumMessageBytes(202)->setMaximumFragments(203);
        testResult.expectEqual(std::size_t{201}, webSocketConfig.getMaximumFrameBytes(), "WebSocket frame accessor reflects configuration");
        testResult.expectEqual(
            std::size_t{202}, webSocketConfig.getMaximumMessageBytes(), "WebSocket message accessor reflects configuration");
        testResult.expectEqual(
            std::size_t{203}, webSocketConfig.getMaximumFragments(), "WebSocket fragment accessor reflects configuration");

        for (const std::string optionName : {"--maximum-frame-bytes", "--maximum-message-bytes", "--maximum-fragments"}) {
            testResult.expectTrue(webSocketConfig.getOption(optionName) != nullptr, optionName + " is registered as a CLI option");
            testResult.expectTrue(webSocketConfig.getOption(optionName)->get_configurable(), optionName + " is configuration-file enabled");
        }

        const std::string serialized = webSocketConfig.configToStr();
        testResult.expectTrue(serialized.find("maximum-frame-bytes") != std::string::npos,
                              "generated configuration contains the WebSocket frame key");
        testResult.expectTrue(serialized.find("maximum-message-bytes") != std::string::npos,
                              "generated configuration contains the WebSocket message key");
        testResult.expectTrue(serialized.find("maximum-fragments") != std::string::npos,
                              "generated configuration contains the WebSocket fragment key");
    }

    return testResult.processResult();
}
