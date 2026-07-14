#include "core/socket/stream/tls/detail/TLSLifecycleTestAccess.h"
#include "tests/support/TestResult.h"

#include <cerrno>
#include <openssl/err.h>
#include <openssl/ssl.h>

using core::socket::stream::tls::detail::TLSLifecycleTestAccess;
using tests::support::TestResult;

namespace {

    class ReaderProbe : public core::socket::stream::tls::SocketReader {
    public:
        int fatalCalls = 0;
        int lastFatalErrno = 0;
        int readShutdownCalls = 0;
        int handshakes = 0;
        SSL_CTX* ctx = nullptr;

        ReaderProbe()
            : SocketReader("reader-probe", [](int) {}, utils::Timeval(0), 64, utils::Timeval(0)) {
            ctx = SSL_CTX_new(TLS_method());
            ssl = SSL_new(ctx);
        }
        ~ReaderProbe() override {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
        }

    private:
        void unobservedEvent() override {}
        void onReceivedFromPeer(std::size_t) override {}
        void onReadShutdown() override { readShutdownCalls++; }
        void onTlsFatalError(int errnum) override {
            fatalCalls++;
            lastFatalErrno = errnum;
            errno = errnum;
        }
        bool doSSLHandshake(const std::function<void()>&, const std::function<void()>&, const std::function<void(int)>&) override {
            handshakes++;
            return true;
        }
    };

    class WriterProbe : public core::socket::stream::tls::SocketWriter {
    public:
        int fatalCalls = 0;
        int lastFatalErrno = 0;
        int handshakes = 0;
        SSL_CTX* ctx = nullptr;

        WriterProbe()
            : SocketWriter("writer-probe", [](int) {}, utils::Timeval(0), 64, utils::Timeval(0)) {
            ctx = SSL_CTX_new(TLS_method());
            ssl = SSL_new(ctx);
        }
        ~WriterProbe() override {
            SSL_free(ssl);
            SSL_CTX_free(ctx);
        }

    private:
        void unobservedEvent() override {}
        bool onSignal(int) override { return false; }
        void doWriteShutdown(const std::function<void()>& onShutdown) override { onShutdown(); }
        void onTlsFatalError(int errnum) override {
            fatalCalls++;
            lastFatalErrno = errnum;
            errno = errnum;
        }
        bool doSSLHandshake(const std::function<void()>&, const std::function<void()>&, const std::function<void(int)>&) override {
            handshakes++;
            return true;
        }
    };

    void expectReaderFatal(TestResult& result, const char* name, int ret, int sslError, int systemError, unsigned long openSslError, int expectedErrno) {
        TLSLifecycleTestAccess::resetReader();
        ReaderProbe reader;
        char buffer[8] = {};
        TLSLifecycleTestAccess::enqueueReaderResult(ret, sslError, systemError, openSslError);
        errno = 0;
        const ssize_t readRet = TLSLifecycleTestAccess::tlsRead(reader, buffer, sizeof(buffer));
        result.expectEqual(-1, static_cast<int>(readRet), std::string(name) + " read return");
        result.expectEqual(expectedErrno, errno, std::string(name) + " errno");
        result.expectEqual(1, reader.fatalCalls, std::string(name) + " fatal hook");
        result.expectEqual(expectedErrno, reader.lastFatalErrno, std::string(name) + " fatal errno");
        result.expectEqual(0, reader.readShutdownCalls, std::string(name) + " no clean shutdown");
    }

    void expectWriterFatal(TestResult& result, const char* name, int ret, int sslError, int systemError, unsigned long openSslError, int expectedErrno) {
        TLSLifecycleTestAccess::resetWriter();
        WriterProbe writer;
        TLSLifecycleTestAccess::enqueueWriterResult(ret, sslError, systemError, openSslError);
        errno = 0;
        const ssize_t writeRet = TLSLifecycleTestAccess::tlsWrite(writer, "x", 1);
        result.expectEqual(-1, static_cast<int>(writeRet), std::string(name) + " write return");
        result.expectEqual(expectedErrno, errno, std::string(name) + " errno");
        result.expectEqual(1, writer.fatalCalls, std::string(name) + " fatal hook");
        result.expectEqual(expectedErrno, writer.lastFatalErrno, std::string(name) + " fatal errno");
    }

} // namespace

int main() {
    TestResult result;

    {
        TLSLifecycleTestAccess::resetReader();
        ReaderProbe reader;
        char buffer[8] = {};
        TLSLifecycleTestAccess::enqueueReaderResult(3, SSL_ERROR_NONE);
        result.expectEqual(3, static_cast<int>(TLSLifecycleTestAccess::tlsRead(reader, buffer, sizeof(buffer))), "reader success");
    }
    {
        TLSLifecycleTestAccess::resetReader();
        ReaderProbe reader;
        char buffer[8] = {};
        TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_READ);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsRead(reader, buffer, sizeof(buffer))), "reader want read return");
        result.expectEqual(EAGAIN, errno, "reader want read errno");
    }
    {
        TLSLifecycleTestAccess::resetReader();
        ReaderProbe reader;
        char buffer[8] = {};
        TLSLifecycleTestAccess::enqueueReaderResult(-1, SSL_ERROR_WANT_WRITE);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsRead(reader, buffer, sizeof(buffer))), "reader want write return");
        result.expectEqual(1, reader.handshakes, "reader want write one handshake");
        result.expectEqual(EAGAIN, errno, "reader want write errno");
    }
    {
        TLSLifecycleTestAccess::resetReader();
        ReaderProbe reader;
        char buffer[8] = {};
        TLSLifecycleTestAccess::enqueueReaderResult(0, SSL_ERROR_ZERO_RETURN);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsRead(reader, buffer, sizeof(buffer))), "reader clean shutdown return");
        result.expectEqual(1, reader.readShutdownCalls, "reader clean shutdown hook");
        result.expectEqual(0, reader.fatalCalls, "reader clean shutdown not fatal");
        result.expectEqual(EAGAIN, errno, "reader clean shutdown errno");
    }

    expectReaderFatal(result, "reader 111 eof", 0, SSL_ERROR_SYSCALL, 0, 0, EPROTO);
#if defined(SSL_R_UNEXPECTED_EOF_WHILE_READING)
    const unsigned long unexpectedEof = ERR_PACK(ERR_LIB_SSL, 0, SSL_R_UNEXPECTED_EOF_WHILE_READING);
    expectReaderFatal(result, "reader 3 eof", -1, SSL_ERROR_SSL, 0, unexpectedEof, EPROTO);
#endif
    expectReaderFatal(result, "reader reset", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET);
    expectReaderFatal(result, "reader protocol", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO);
    expectReaderFatal(result, "reader unknown", -1, 12345, 0, 0, EIO);

    {
        TLSLifecycleTestAccess::resetWriter();
        WriterProbe writer;
        TLSLifecycleTestAccess::enqueueWriterResult(2, SSL_ERROR_NONE);
        result.expectEqual(2, static_cast<int>(TLSLifecycleTestAccess::tlsWrite(writer, "xy", 2)), "writer success");
    }
    {
        TLSLifecycleTestAccess::resetWriter();
        WriterProbe writer;
        TLSLifecycleTestAccess::enqueueWriterResult(-1, SSL_ERROR_WANT_WRITE);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsWrite(writer, "x", 1)), "writer want write return");
        result.expectEqual(EAGAIN, errno, "writer want write errno");
    }
    {
        TLSLifecycleTestAccess::resetWriter();
        WriterProbe writer;
        TLSLifecycleTestAccess::enqueueWriterResult(-1, SSL_ERROR_WANT_READ);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsWrite(writer, "x", 1)), "writer want read return");
        result.expectEqual(1, writer.handshakes, "writer want read one handshake");
        result.expectEqual(EAGAIN, errno, "writer want read errno");
    }
    {
        TLSLifecycleTestAccess::resetWriter();
        WriterProbe writer;
        TLSLifecycleTestAccess::enqueueWriterResult(0, SSL_ERROR_ZERO_RETURN);
        result.expectEqual(-1, static_cast<int>(TLSLifecycleTestAccess::tlsWrite(writer, "x", 1)), "writer zero return");
        result.expectEqual(EAGAIN, errno, "writer zero return errno");
        result.expectEqual(0, writer.fatalCalls, "writer zero return not fatal");
    }

    expectWriterFatal(result, "writer epipe", -1, SSL_ERROR_SYSCALL, EPIPE, 0, EPIPE);
    expectWriterFatal(result, "writer reset", -1, SSL_ERROR_SYSCALL, ECONNRESET, 0, ECONNRESET);
    expectWriterFatal(result, "writer eof", 0, SSL_ERROR_SYSCALL, 0, 0, EPROTO);
    expectWriterFatal(result, "writer protocol", -1, SSL_ERROR_SSL, 0, ERR_PACK(ERR_LIB_SSL, 0, SSL_R_BAD_RECORD_TYPE), EPROTO);
    expectWriterFatal(result, "writer unknown", -1, 12345, 0, 0, EIO);

    return result.processResult();
}
