#include "core/pipe/Source.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "log/Logger.h"
#include "tests/support/Phase3SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "web/http/server/Response.h"
#include "web/http/server/SocketContext.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace core::socket::stream::detail {

    struct ContextLifecycleTestAccess {
        static void attach(SocketContext* context) {
            context->attach();
        }

        static std::size_t receive(SocketContext* context) {
            return context->readFromPeer();
        }

        static void detachForConnectionClose(SocketContext* context) {
            context->detach(SocketContext::DetachReason::ConnectionClose);
        }
    };

} // namespace core::socket::stream::detail

namespace {

    class TestSocketAddress : public core::socket::SocketAddress {
    public:
        std::string toString(bool = true) const override {
            return "phase3-http-source-failure-address";
        }
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        TestSocketConnection()
            : SocketConnection(42, 315, "phase3-http-source-failure-server", nullptr) {
        }

        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 42;
        }

        void sendToPeer(const char* chunk, std::size_t chunkLength) override {
            sent.append(chunk, chunkLength);
        }

        bool streamToPeer(core::pipe::Source* source) override {
            ++streamStartCount;
            return source != nullptr;
        }

        void streamEof() override {
            ++streamEofCount;
        }

        std::size_t readFromPeer(char* chunk, std::size_t chunkLength) override {
            const std::size_t remaining = request.size() - requestOffset;
            const std::size_t copied = std::min(chunkLength, remaining);
            if (copied > 0) {
                std::memcpy(chunk, request.data() + requestOffset, copied);
                requestOffset += copied;
            }
            return copied;
        }

        void shutdownRead() override {
        }

        void shutdownWrite() override {
        }

        const core::socket::SocketAddress& getBindAddress() const override {
            return address;
        }

        const core::socket::SocketAddress& getLocalAddress() const override {
            return address;
        }

        const core::socket::SocketAddress& getRemoteAddress() const override {
            return address;
        }

        void close() override {
            ++closeCount;
        }

        void setTimeout(const utils::Timeval&) override {
        }

        void setReadTimeout(const utils::Timeval&) override {
        }

        void setWriteTimeout(const utils::Timeval&) override {
        }

        std::size_t getTotalSent() const override {
            return sent.size();
        }

        std::size_t getTotalQueued() const override {
            return sent.size();
        }

        std::size_t getTotalRead() const override {
            return requestOffset;
        }

        std::size_t getTotalProcessed() const override {
            return requestOffset;
        }

        const std::string request = "GET /stream-error HTTP/1.1\r\nHost: localhost\r\n\r\n";
        std::string sent;
        std::size_t requestOffset = 0;
        int streamStartCount = 0;
        int streamEofCount = 0;
        int closeCount = 0;

    private:
        TestSocketAddress address;
    };

    class FailingSource : public core::pipe::Source {
    public:
        bool isOpen() override {
            return open;
        }

        void start() override {
            ++startCount;
            constexpr std::string_view payload = "partial";
            bytesSent = send(payload.data(), payload.size());
            ++errorCount;
            error(EIO);
        }

        void suspend() override {
        }

        void resume() override {
        }

        void stop() override {
            open = false;
            ++stopCount;
        }

        void finishAfterFailure() {
            ++lateEofCount;
            eof();
        }

        bool open = true;
        int startCount = 0;
        int errorCount = 0;
        int lateEofCount = 0;
        int stopCount = 0;
        std::ptrdiff_t bytesSent = 0;
    };

    std::string value(const nlohmann::json& record, const char* field) {
        const auto entry = record.find(field);
        return entry != record.end() && entry->is_string() ? entry->get<std::string>() : std::string();
    }

} // namespace

int main() {
    tests::support::TestResult result;
    tests::support::Phase3SemanticLogCapture capture("snodec-phase3-http-streaming-source-failure");
    logger::Logger::setLogLevel(6);
    logger::LogManager::setGlobalLevel(logger::LogLevel::Debug);
    logger::LogManager::setFormat(logger::LogManager::Format::Json);
    logger::LogManager::freeze();

    {
        TestSocketConnection connection;
        FailingSource source;
        int requestCount = 0;
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection,
            [&requestCount, &sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                ++requestCount;
                response->status(200).type("text/plain").set("Content-Length", "7");
                sourcePipeAccepted = source.pipe(response.get());
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        const std::size_t consumed = core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext);

        source.finishAfterFailure();
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectEqual(connection.request.size(), consumed, "streaming failure fixture consumes the complete HTTP request");
        result.expectEqual(1, requestCount, "streaming failure fixture delivers one HTTP request");
        result.expectTrue(sourcePipeAccepted, "streaming failure source attaches through the real response sink");
        result.expectEqual(1, source.startCount, "streaming failure source starts once");
        result.expectEqual(1, source.errorCount, "streaming failure source reports one error");
        result.expectEqual(7, source.bytesSent, "streaming failure source sends the complete declared body length before failing");
        result.expectEqual(1, source.lateEofCount, "streaming failure fixture exercises later EOF cleanup");
        result.expectEqual(1, source.stopCount, "streaming failure source stops during context detach");
        result.expectEqual(1, connection.streamStartCount, "response starts one source stream");
        result.expectTrue(connection.streamEofCount >= 2, "source error and later EOF both reach stream cleanup");
        result.expectTrue(connection.closeCount >= 1, "source error closes the HTTP connection");
        result.expectTrue(connection.sent.find("HTTP/1.1 200 OK") != std::string::npos &&
                              connection.sent.find("partial") != std::string::npos,
                          "response starts and forwards the exact-length source payload before failure");
    }

    const std::vector<nlohmann::json> records = capture.finish();
    std::vector<nlohmann::json> started;
    std::vector<nlohmann::json> failed;
    int completedCount = 0;
    int abortedCount = 0;
    int responseErrorDiagnosticCount = 0;

    for (const nlohmann::json& record : records) {
        if (value(record, "component") == "web.http.server" && value(record, "instance") == "phase3-http-source-failure-server") {
            const std::string message = value(record, "message");
            if (message.starts_with("request started:")) {
                started.push_back(record);
            } else if (message.starts_with("request failed:")) {
                failed.push_back(record);
            } else if (message.starts_with("request completed:")) {
                ++completedCount;
            } else if (message.starts_with("request aborted:")) {
                ++abortedCount;
            }
        }
        if (value(record, "message").starts_with("HTTP: Response completed with error:")) {
            ++responseErrorDiagnosticCount;
        }
    }

    result.expectEqual(1, started.size(), "streaming source failure emits one request start");
    result.expectEqual(1, failed.size(), "streaming source failure emits one request failure");
    result.expectEqual(0, completedCount, "streaming source failure never emits request completed");
    result.expectEqual(0, abortedCount, "context detach does not abort an already failed request");
    result.expectTrue(responseErrorDiagnosticCount >= 1, "existing response-error diagnostic remains available");

    if (started.size() == 1 && failed.size() == 1) {
        constexpr std::string_view expectedInstance = "phase3-http-source-failure-server";
        constexpr std::string_view expectedConnection = "315";
        for (const nlohmann::json* record : {&started.front(), &failed.front()}) {
            result.expectTrue(value(*record, "level") == "debug" && value(*record, "origin") == "framework" &&
                                  value(*record, "boundary") == "connection" && value(*record, "component") == "web.http.server" &&
                                  value(*record, "role") == "server" && value(*record, "instance") == expectedInstance &&
                                  value(*record, "connection") == expectedConnection,
                              "streaming request lifecycle uses bound server identity and Debug level");
        }
        result.expectTrue(value(started.front(), "connection") == value(failed.front(), "connection"),
                          "streaming request start and failure carry matching connection identity");
        result.expectTrue(value(started.front(), "message") == "request started: id=1" &&
                              value(failed.front(), "message") == "request failed: id=1",
                          "streaming request start and failure retain the same request identifier");
    }

    return result.processResult();
}
