#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"

#include <filesystem>
#include <fstream>
#include <openssl/err.h>
#include <openssl/sslerr.h>
#include <string>

namespace {
    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::init();
            logger::LogManager::init();
            ERR_clear_error();
        }
    };

    std::filesystem::path tempLogPath(const std::string& name) {
        auto path = std::filesystem::temp_directory_path() / name;
        std::error_code error;
        std::filesystem::remove(path, error);
        std::filesystem::remove(path.string() + ".1", error);
        return path;
    }

    std::string readFile(const std::filesystem::path& path) {
        std::ifstream input(path);
        return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    }

    void queueOpenSslError() {
        ERR_clear_error();
        ERR_new();
        ERR_set_error(ERR_LIB_SSL, SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE, nullptr);
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const auto emittedPath = tempLogPath("snodec-openssl-log-emitted.log");
    {
        LoggerStateGuard guard(emittedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        queueOpenSslError();

        core::socket::stream::tls::ssl_log_error("SSL/TLS: OpenSSL helper failed");

        result.expectEqual(static_cast<unsigned long>(0), ERR_peek_error(),
                           "an emitted ssl_log_error drains the OpenSSL error queue");
    }
    const std::string emittedLog = readFile(emittedPath);
    result.expectTrue(emittedLog.find("SSL/TLS: OpenSSL helper failed") != std::string::npos &&
                          emittedLog.find("SSL routines") != std::string::npos,
                      "an emitted OpenSSL diagnostic includes the queued OpenSSL error");

    const auto expectSuppressedHelperKeepsQueue = [&](const std::string& label, const auto& helper) {
        const auto path = tempLogPath("snodec-openssl-log-suppressed-" + label + ".log");
        LoggerStateGuard guard(path.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Critical);
        logger::LogManager::freeze();
        queueOpenSslError();

        helper("SSL/TLS: suppressed OpenSSL helper");

        result.expectTrue(ERR_peek_error() != 0, "suppressed ssl_log_" + label + " does not drain OpenSSL error queue");
        result.expectTrue(readFile(path).empty(), "suppressed ssl_log_" + label + " emits no backend output");
    };

    expectSuppressedHelperKeepsQueue("error", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_error(message);
    });
    expectSuppressedHelperKeepsQueue("warning", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_warning(message);
    });
    expectSuppressedHelperKeepsQueue("info", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_info(message);
    });

    return result.processResult();
}
