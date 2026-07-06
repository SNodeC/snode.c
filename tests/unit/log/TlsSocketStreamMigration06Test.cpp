#include "core/socket/stream/tls/SocketReader.h"
#include "core/socket/stream/tls/SocketWriter.h"
#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"
#include "tests/support/TestResult.h"
#include "utils/Timeval.h"

#include <cerrno>
#include <filesystem>
#include <fstream>
#include <openssl/err.h>
#include <openssl/sslerr.h>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace {
    struct ExpensiveValue {
        int* evaluations;
    };

    std::ostream& operator<<(std::ostream& out, const ExpensiveValue& value) {
        ++*value.evaluations;
        return out << "expensive";
    }

    class TestTlsReader : public core::socket::stream::tls::SocketReader {
    public:
        explicit TestTlsReader(const std::string& instanceName = "migration06-reader")
            : SocketReader(
                  instanceName,
                  [](int) {
                  },
                  utils::Timeval{},
                  1024,
                  utils::Timeval{}) {
        }

    private:
        void onReceivedFromPeer(std::size_t) override {
        }

        void onReadShutdown() override {
        }

        bool
        doSSLHandshake(const std::function<void()>& onSuccess, const std::function<void()>&, const std::function<void(int)>&) override {
            onSuccess();
            return true;
        }

        void unobservedEvent() override {
        }
    };

    class TestTlsWriter : public core::socket::stream::tls::SocketWriter {
    public:
        explicit TestTlsWriter(const std::string& instanceName = "migration06-writer")
            : SocketWriter(
                  instanceName,
                  [](int) {
                  },
                  utils::Timeval{},
                  1024,
                  utils::Timeval{}) {
        }

    private:
        bool onSignal(int) override {
            return false;
        }

        void doWriteShutdown(const std::function<void()>& onShutdown) override {
            onShutdown();
        }

        bool
        doSSLHandshake(const std::function<void()>& onSuccess, const std::function<void()>&, const std::function<void(int)>&) override {
            onSuccess();
            return true;
        }

        void unobservedEvent() override {
        }
    };

    class LoggerStateGuard {
    public:
        explicit LoggerStateGuard(const std::string& logFile) {
            logger::Logger::init();
            logger::LogManager::init();
            logger::Logger::setLogLevel(6);
            logger::Logger::setVerboseLevel(0);
            logger::Logger::setQuiet(true);
            logger::Logger::setDisableColor(true);
            logger::Logger::setTickResolver([]() {
                return std::string("MIGRATION06TICK");
            });
            logger::Logger::logToFile(logFile);
        }

        ~LoggerStateGuard() {
            logger::Logger::disableLogToFile();
            logger::Logger::setTickResolver({});
            logger::Logger::init();
            logger::LogManager::init();
            errno = 0;
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
        std::ifstream in(path);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
} // namespace

int main() {
    tests::support::TestResult result;

    const auto enabledPath = tempLogPath("snodec-migration06-enabled.log");
    {
        LoggerStateGuard guard(enabledPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        TestTlsReader reader;
        TestTlsWriter writer;
        reader.log().debug("tls reader semantic owner emitted");
        writer.log().debug("tls writer semantic owner emitted");
    }
    const auto enabledLog = readFile(enabledPath);
    result.expectTrue(enabledLog.find("tls reader semantic owner emitted") != std::string::npos,
                      "TLS reader semantic owner emits when enabled");
    result.expectTrue(enabledLog.find("tls writer semantic owner emitted") != std::string::npos,
                      "TLS writer semantic owner emits when enabled");
    result.expectTrue(enabledLog.find(" framework connection core.socket.stream.tls ") != std::string::npos,
                      "emitted records carry framework core.socket.stream.tls component scope");

    const auto managerFilterPath = tempLogPath("snodec-migration06-manager-filter.log");
    {
        LoggerStateGuard guard(managerFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();
        TestTlsReader reader;
        reader.log().debug("tls reader suppressed by manager");
    }
    result.expectTrue(readFile(managerFilterPath).find("suppressed by manager") == std::string::npos,
                      "LogManager filtering suppresses migrated TLS semantic calls");

    const auto backendFilterPath = tempLogPath("snodec-migration06-backend-filter.log");
    {
        LoggerStateGuard guard(backendFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        logger::Logger::setLogLevel(3);
        TestTlsWriter writer;
        writer.log().debug("tls writer suppressed by backend");
    }
    result.expectTrue(readFile(backendFilterPath).find("suppressed by backend") == std::string::npos,
                      "Logger::setLogLevel backend filtering suppresses output");

    const auto jsonPath = tempLogPath("snodec-migration06-json.log");
    {
        LoggerStateGuard guard(jsonPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
        logger::LogManager::setFormat(logger::LogManager::Format::Json);
        logger::LogManager::freeze();
        TestTlsReader reader;
        reader.log().debug("tls reader json format respected");
    }
    const auto jsonLog = readFile(jsonPath);
    result.expectTrue(jsonLog.find("{\"v\":1") != std::string::npos &&
                          jsonLog.find("\"message\":\"tls reader json format respected\"") != std::string::npos &&
                          jsonLog.find("\"origin\":\"framework\"") != std::string::npos &&
                          jsonLog.find("\"component\":\"core.socket.stream.tls\"") != std::string::npos,
                      "JSON format selection is respected");

    std::vector<logger::LogRecord> sysErrorRecords;
    TestTlsWriter sysErrorOwner;
    sysErrorOwner
        .log([&](logger::LogRecord record) {
            sysErrorRecords.push_back(std::move(record));
        })
        .sysError(logger::LogLevel::Warn, EPIPE, "{} SSL/TLS: Syscal error (SIGPIPE detected) on write.", "migration06-writer");
    result.expectTrue(sysErrorRecords.size() == 1 &&
                          sysErrorRecords[0].message == "migration06-writer SSL/TLS: Syscal error (SIGPIPE detected) on write." &&
                          sysErrorRecords[0].error && sysErrorRecords[0].error->code == EPIPE &&
                          sysErrorRecords[0].error->text.find("Broken pipe") != std::string::npos,
                      "sysError errno code/text is preserved for migrated TLS writer PLOG sites");

    std::vector<logger::LogRecord> sinkRecords;
    TestTlsReader sinkOwner;
    sinkOwner
        .log([&](logger::LogRecord record) {
            sinkRecords.push_back(std::move(record));
        })
        .info("sink overload still works");
    result.expectTrue(sinkRecords.size() == 1 && sinkRecords[0].message == "sink overload still works" &&
                          sinkRecords[0].component == "core.socket.stream.tls" && sinkRecords[0].origin == logger::LogOrigin::Framework,
                      "sink-taking overload remains compatible");

    const auto suppressedPath = tempLogPath("snodec-migration06-suppressed.log");
    {
        LoggerStateGuard guard(suppressedPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Error);
        logger::LogManager::freeze();

        int evaluations = 0;
        TestTlsReader reader;
        reader.log().debug("{}", ExpensiveValue{&evaluations});
        result.expectEqual(0, evaluations, "suppressed production formatting does not evaluate ExpensiveValue after PR #151");
    }
    result.expectTrue(readFile(suppressedPath).empty(), "suppressed production formatting emits no backend output");

    std::vector<logger::LogRecord> opensslIoRecords;
    TestTlsReader opensslIoOwner;
    opensslIoOwner
        .log([&](logger::LogRecord record) {
            opensslIoRecords.push_back(std::move(record));
        })
        .warn("{} SSL/TLS: Renegotiation on read timed out", "migration06-reader");
    result.expectTrue(opensslIoRecords.size() == 1 &&
                          opensslIoRecords[0].message.find("SSL/TLS: Renegotiation on read timed out") != std::string::npos,
                      "OpenSSL I/O retry payload text is preserved without requiring live TLS sockets/certs");

    std::vector<logger::LogRecord> handshakeRecords;
    TestTlsReader handshakeOwner("migration06-handshake");
    handshakeOwner
        .log([&](logger::LogRecord record) {
            handshakeRecords.push_back(std::move(record));
        })
        .debug("{} SSL/TLS: Handshake success", "migration06-handshake");
    result.expectTrue(handshakeRecords.size() == 1 && handshakeRecords[0].message == "migration06-handshake SSL/TLS: Handshake success" &&
                          handshakeRecords[0].component == "core.socket.stream.tls",
                      "handshake semantic payload preservation is covered through the TLS owner sink path");

    std::vector<logger::LogRecord> shutdownRecords;
    TestTlsWriter shutdownOwner("migration06-shutdown");
    shutdownOwner
        .log([&](logger::LogRecord record) {
            shutdownRecords.push_back(std::move(record));
        })
        .debug("{} SSL/TLS: Passive close_notify received and sent", "migration06-shutdown");
    result.expectTrue(shutdownRecords.size() == 1 &&
                          shutdownRecords[0].message == "migration06-shutdown SSL/TLS: Passive close_notify received and sent",
                      "shutdown close_notify semantic payload preservation is covered through the TLS owner sink path");

    const auto opensslHelperPath = tempLogPath("snodec-migration06-openssl-helper.log");
    {
        LoggerStateGuard guard(opensslHelperPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Trace);
        logger::LogManager::freeze();
        ERR_new();
        ERR_set_error(ERR_LIB_SSL, SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE, nullptr);
        core::socket::stream::tls::ssl_log_error("migration06 SSL/TLS: OpenSSL helper failed");
    }
    const auto opensslHelperLog = readFile(opensslHelperPath);
    result.expectTrue(opensslHelperLog.find("migration06 SSL/TLS: OpenSSL helper failed") != std::string::npos &&
                          opensslHelperLog.find("SSL routines") != std::string::npos,
                      "OpenSSL helper severity mapping and queue-drain payload are preserved");

    const auto opensslHelperFilterPath = tempLogPath("snodec-migration06-openssl-helper-filter.log");
    {
        LoggerStateGuard guard(opensslHelperFilterPath.string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Critical);
        logger::LogManager::freeze();
        ERR_new();
        ERR_set_error(ERR_LIB_SSL, SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE, nullptr);
        core::socket::stream::tls::ssl_log_warning("migration06 SSL/TLS: OpenSSL helper suppressed");
    }
    result.expectTrue(readFile(opensslHelperFilterPath).find("OpenSSL helper suppressed") == std::string::npos,
                      "LogManager filtering suppresses 6b OpenSSL helper semantic calls");

    auto expectSuppressedOpenSslHelperKeepsQueue = [&](const std::string& label, const auto& helper) {
        LoggerStateGuard guard(tempLogPath("snodec-migration06-openssl-queue-" + label + ".log").string());
        logger::LogManager::setGlobalLevel(logger::LogLevel::Critical);
        logger::LogManager::freeze();
        ERR_clear_error();
        ERR_new();
        ERR_set_error(ERR_LIB_SSL, SSL_R_SSLV3_ALERT_HANDSHAKE_FAILURE, nullptr);

        helper("migration06 SSL/TLS: suppressed OpenSSL helper queue check");

        const unsigned long stillQueued = ERR_get_error();
        result.expectTrue(stillQueued != 0, "suppressed ssl_log_" + label + " does not drain OpenSSL error queue");
        while (ERR_get_error() != 0) {
        }
    };
    expectSuppressedOpenSslHelperKeepsQueue("error", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_error(message);
    });
    expectSuppressedOpenSslHelperKeepsQueue("warning", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_warning(message);
    });
    expectSuppressedOpenSslHelperKeepsQueue("info", [](const std::string& message) {
        core::socket::stream::tls::ssl_log_info(message);
    });

    std::vector<logger::LogRecord> sniRecords;
    TestTlsReader sniOwner("migration06-sni");
    auto sniLog = sniOwner.log([&](logger::LogRecord record) {
        sniRecords.push_back(std::move(record));
    });
    sniLog.debug("{} SSL/TLS: Setting sni certificate for '{}'", "migration06-connection", "example.test");
    sniLog.error("{} SSL/TLS: No sni certificate found for '{}' but forceSni set - terminating", "migration06-connection", "missing.test");
    sniLog.warn("{} SSL/TLS: No sni certificate found for '{}'. Still using master certificate", "migration06-connection", "fallback.test");
    sniLog.debug("{} SSL/TLS: No sni certificate requested from client. Still using master certificate", "migration06-connection");
    result.expectTrue(
        sniRecords.size() == 4 && sniRecords[0].message == "migration06-connection SSL/TLS: Setting sni certificate for 'example.test'" &&
            sniRecords[1].message ==
                "migration06-connection SSL/TLS: No sni certificate found for 'missing.test' but forceSni set - terminating" &&
            sniRecords[2].message ==
                "migration06-connection SSL/TLS: No sni certificate found for 'fallback.test'. Still using master certificate" &&
            sniRecords[3].message ==
                "migration06-connection SSL/TLS: No sni certificate requested from client. Still using master certificate",
        "SNI/client-hello payload preservation is covered through the TLS owner sink path");

    return result.processResult();
}
