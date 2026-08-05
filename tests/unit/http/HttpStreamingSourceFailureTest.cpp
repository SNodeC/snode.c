#include "core/pipe/Source.h"
#include "core/socket/SocketAddress.h"
#include "core/socket/stream/SocketConnection.h"
#include "core/socket/stream/SocketContext.h"
#include "express/Response.h"
#include "log/Logger.h"
#include "tests/support/SemanticLogCapture.h"
#include "tests/support/TestResult.h"
#include "web/http/client/Request.h"
#include "web/http/client/SocketContext.h"
#include "web/http/server/Response.h"
#include "web/http/server/SocketContext.h"

#include <algorithm>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
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
            return "http-source-failure-address";
        }
    };

    class TestSocketConnection : public core::socket::stream::SocketConnection {
    public:
        TestSocketConnection()
            : SocketConnection(42, 315, "http-streaming-source-failure-server", nullptr) {
        }

        ~TestSocketConnection() override = default;

        int getFd() const override {
            return 42;
        }

        void sendToPeer(const char* chunk, std::size_t chunkLength) override {
            sent.append(chunk, chunkLength);
        }

        core::socket::stream::QueueResult trySendToPeer(const char* chunk, std::size_t chunkLength) override {
            ++boundedSendCount;

            if (rejectBoundedSendAt > 0 && boundedSendCount >= rejectBoundedSendAt) {
                return core::socket::stream::QueueResult::WouldExceedLimit;
            }

            sendToPeer(chunk, chunkLength);
            return core::socket::stream::QueueResult::Queued;
        }

        bool streamToPeer(core::pipe::Source* source) override {
            ++streamStartCount;
            activeSource = source;
            return activeSource != nullptr;
        }

        void streamEof() override {
            ++streamEofCount;
            activeSource = nullptr;
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
            ++shutdownWriteCount;
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

        bool hasActiveSource() const {
            return activeSource != nullptr;
        }

        const std::string request = "GET /stream-error HTTP/1.1\r\nHost: localhost\r\n\r\n";
        std::string sent;
        std::size_t requestOffset = 0;
        int streamStartCount = 0;
        int streamEofCount = 0;
        int closeCount = 0;
        int shutdownWriteCount = 0;
        int boundedSendCount = 0;
        int rejectBoundedSendAt = 0;

    private:
        TestSocketAddress address;
        core::pipe::Source* activeSource = nullptr;
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

    class HoldingSource : public core::pipe::Source {
    public:
        explicit HoldingSource(std::function<bool()> isWriterDetached, bool sendPayload = false, bool finish = false)
            : isWriterDetached(std::move(isWriterDetached))
            , sendPayload(sendPayload)
            , finish(finish) {
        }

        bool isOpen() override {
            return open;
        }

        void start() override {
            ++startCount;
            if (sendPayload) {
                constexpr std::string_view payload = "payload";
                bytesSent = send(payload.data(), payload.size());
            }
            if (finish) {
                eof();
            }
        }

        void suspend() override {
        }

        void resume() override {
        }

        void stop() override {
            writerDetachedBeforeStop = isWriterDetached();
            open = false;
            ++stopCount;
        }

        bool open = true;
        int startCount = 0;
        int stopCount = 0;
        bool writerDetachedBeforeStop = false;
        std::ptrdiff_t bytesSent = 0;

    private:
        std::function<bool()> isWriterDetached;
        bool sendPayload;
        bool finish;
    };

    std::string value(const nlohmann::json& record, const char* field) {
        const auto entry = record.find(field);
        return entry != record.end() && entry->is_string() ? entry->get<std::string>() : std::string();
    }

} // namespace

int main() {
    tests::support::TestResult result;
    tests::support::SemanticLogCapture capture("snodec-http-streaming-source-failure");
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
                sourcePipeAccepted = response->pipe(&source);
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
        result.expectEqual(1, source.stopCount, "streaming failure source stops when it reports an error");
        result.expectEqual(1, connection.streamStartCount, "response starts one source stream");
        result.expectEqual(1, connection.streamEofCount, "source error clears the socket writer stream exactly once");
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
        if (value(record, "component") == "web.http.server" && value(record, "instance") == "http-streaming-source-failure-server") {
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
        constexpr std::string_view expectedInstance = "http-streaming-source-failure-server";
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

    {
        TestSocketConnection connection;
        HoldingSource source([&connection]() {
            return !connection.hasActiveSource();
        });
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain").set("Content-Length", "7");
                express::Response expressResponse(response);
                sourcePipeAccepted = expressResponse.pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(sourcePipeAccepted, "Express Response forwards descriptor-compatible sources to the HTTP response pipe");
        result.expectEqual(1, source.startCount, "a connected holding source starts exactly once");
        result.expectEqual(1, source.stopCount, "disconnect stops an active response source exactly once");
        result.expectEqual(1, connection.streamEofCount, "disconnect clears the socket writer stream exactly once");
        result.expectTrue(source.writerDetachedBeforeStop, "disconnect clears the socket writer source pointer before stopping the source");
    }

    {
        TestSocketConnection connection;
        connection.rejectBoundedSendAt = 2;
        HoldingSource source(
            [&connection]() {
                return !connection.hasActiveSource();
            },
            true);
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain").set("Transfer-Encoding", "chunked");
                sourcePipeAccepted = response->pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(sourcePipeAccepted, "bounded response accepts an initially connected source");
        result.expectEqual(2, connection.boundedSendCount, "streamed header and payload each use bounded queue admission");
        result.expectEqual(1, source.startCount, "source starts after its response header is admitted");
        result.expectEqual(1, source.stopCount, "payload queue rejection stops the source immediately");
        result.expectEqual(1, connection.streamEofCount, "payload queue rejection clears the writer source association");
        result.expectTrue(source.writerDetachedBeforeStop, "queue rejection clears the writer before stopping its source");
        result.expectTrue(connection.closeCount >= 1, "payload queue rejection closes the HTTP connection");
        result.expectTrue(connection.sent.find("payload") == std::string::npos && connection.sent.find("7\r\n") == std::string::npos,
                          "rejected chunk framing and payload are not partially appended to the outbound queue");
    }

    {
        TestSocketConnection connection;
        connection.rejectBoundedSendAt = 1;
        HoldingSource source([&connection]() {
            return !connection.hasActiveSource();
        });
        bool sourcePipeAccepted = true;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain").set("Transfer-Encoding", "chunked");
                sourcePipeAccepted = response->pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(!sourcePipeAccepted, "response pipe reports synchronous header admission failure");
        result.expectEqual(0, source.startCount, "source does not start when its response header is rejected");
        result.expectEqual(1, source.stopCount, "header admission failure stops the source");
        result.expectEqual(1, connection.streamEofCount, "header admission failure clears the writer source association");
        result.expectTrue(connection.sent.empty(), "header admission failure appends no partial response header");
    }

    {
        TestSocketConnection connection;
        HoldingSource source(
            [&connection]() {
                return !connection.hasActiveSource();
            },
            true,
            true);
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain").set("Transfer-Encoding", "chunked").setTrailer("X-Stream", "complete");
                sourcePipeAccepted = response->pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(sourcePipeAccepted, "synchronously completing response source is accepted");
        result.expectTrue(connection.sent.find("7\r\npayload\r\n0\r\nX-Stream: complete\r\n\r\n") != std::string::npos,
                          "bounded chunk completion places trailers before the terminating blank line");
        result.expectEqual(1, source.stopCount, "synchronously completing source stops exactly once");
        result.expectEqual(1, connection.streamEofCount, "synchronously completing source clears the writer once");
    }

    {
        TestSocketConnection connection;
        HoldingSource source(
            [&connection]() {
                return !connection.hasActiveSource();
            },
            true,
            true);
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain");
                sourcePipeAccepted = response->pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(sourcePipeAccepted, "generic HTTP/1.1 response source is accepted without explicit framing");
        result.expectTrue(connection.sent.find("Transfer-Encoding: chunked\r\n") != std::string::npos,
                          "generic HTTP/1.1 response pipe selects chunked transfer encoding");
        result.expectTrue(connection.sent.find("7\r\npayload\r\n0\r\n\r\n") != std::string::npos,
                          "generic HTTP/1.1 response pipe emits a complete chunked body");
        result.expectEqual(0, connection.closeCount, "successfully framed generic response does not fail the connection");
        result.expectEqual(1, source.stopCount, "successfully framed generic source stops exactly once");
        result.expectEqual(1, connection.streamEofCount, "successfully framed generic source clears the writer once");
    }

    {
        TestSocketConnection connection;
        HoldingSource source(
            [&connection]() {
                return !connection.hasActiveSource();
            },
            true);
        bool sourcePipeAccepted = false;

        auto* socketContext = new web::http::server::SocketContext(
            &connection, [&sourcePipeAccepted, &source](const auto&, const std::shared_ptr<web::http::server::Response>& response) {
                response->status(200).type("text/plain").set("Content-Length", "3");
                sourcePipeAccepted = response->pipe(&source);
            });

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        static_cast<void>(core::socket::stream::detail::ContextLifecycleTestAccess::receive(socketContext));
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(sourcePipeAccepted, "declared-length response accepts its source after admitting the header");
        result.expectEqual(1, connection.boundedSendCount, "declared-length overrun is rejected before a payload queue admission");
        result.expectTrue(connection.sent.find("Content-Length: 3\r\n") != std::string::npos &&
                              connection.sent.find("payload") == std::string::npos,
                          "declared-length overrun leaves the complete source fragment off the wire");
        result.expectEqual(1, source.stopCount, "declared-length overrun stops the source immediately");
        result.expectEqual(1, connection.streamEofCount, "declared-length overrun clears the writer source association");
        result.expectTrue(connection.closeCount >= 1, "declared-length overrun closes the HTTP connection");
    }

    {
        TestSocketConnection connection;
        connection.rejectBoundedSendAt = 2;
        bool requestQueued = false;

        auto* socketContext = new web::http::client::SocketContext(
            &connection,
            [&requestQueued](const std::shared_ptr<web::http::client::MasterRequest>& request) {
                request->method = "POST";
                requestQueued = request->send(
                    "payload",
                    [](const auto&, const auto&) {
                    },
                    [](const auto&, const std::string&) {
                    });
            },
            [](const auto&) {
            },
            "localhost",
            false);

        core::socket::stream::detail::ContextLifecycleTestAccess::attach(socketContext);
        core::socket::stream::detail::ContextLifecycleTestAccess::detachForConnectionClose(socketContext);

        result.expectTrue(requestQueued, "HTTP client queues a request for bounded admission");
        result.expectEqual(2, connection.boundedSendCount, "HTTP client header and body each use bounded queue admission");
        result.expectTrue(connection.sent.find("POST / HTTP/1.1") != std::string::npos,
                          "HTTP client admits the complete request header before the configured rejection");
        result.expectTrue(connection.sent.find("payload") == std::string::npos, "HTTP client does not append a rejected request payload");
        result.expectEqual(1, connection.shutdownWriteCount, "HTTP client propagates queue rejection as failed request delivery");
    }

    return result.processResult();
}
