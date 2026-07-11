#include "support/TestResult.h"
#include "web/websocket/Receiver.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace {
    class BufferReceiver : public web::websocket::Receiver {
    public:
        explicit BufferReceiver(bool maskingExpected)
            : Receiver(maskingExpected) {
        }

        void feed(const std::vector<uint8_t>& bytes) {
            input.insert(input.end(), bytes.begin(), bytes.end());
            while (offset < input.size() && errors == 0) {
                const std::size_t consumed = receive();
                if (consumed == 0) {
                    break;
                }
            }
        }

        int starts = 0;
        int ends = 0;
        int errors = 0;
        uint16_t lastError = 0;
        std::vector<int> opcodes;
        std::string data;

    private:
        std::size_t readFrameChunk(char* chunk, std::size_t chunkLen) const override {
            const std::size_t available = input.size() - offset;
            const std::size_t toRead = std::min(chunkLen, available);
            if (toRead > 0) {
                std::memcpy(chunk, input.data() + offset, toRead);
                offset += toRead;
            }
            return toRead;
        }

        void onMessageStart(int opCode) override {
            starts++;
            opcodes.push_back(opCode);
        }

        void onMessageData(const char* chunk, uint64_t chunkLen) override {
            data.append(chunk, static_cast<std::size_t>(chunkLen));
        }

        void onMessageEnd() override {
            ends++;
        }

        void onMessageError(uint16_t errnum) override {
            errors++;
            lastError = errnum;
        }

        mutable std::size_t offset = 0;
        mutable std::vector<uint8_t> input;
    };

    std::vector<uint8_t> mask(std::initializer_list<uint8_t> header, const std::string& payload = {}) {
        constexpr uint8_t maskingKey[] = {0x01, 0x02, 0x03, 0x04};
        std::vector<uint8_t> frame(header);
        frame.insert(frame.end(), std::begin(maskingKey), std::end(maskingKey));
        for (std::size_t i = 0; i < payload.size(); ++i) {
            frame.push_back(static_cast<uint8_t>(payload[i]) ^ maskingKey[i % 4]);
        }
        return frame;
    }

    std::vector<uint8_t> unmasked(std::initializer_list<uint8_t> header, const std::string& payload = {}) {
        std::vector<uint8_t> frame(header);
        frame.insert(frame.end(), payload.begin(), payload.end());
        return frame;
    }

    void expectAccepted(tests::support::TestResult& result, const std::string& name, BufferReceiver& receiver, int ends = 1) {
        result.expectEqual(0, receiver.errors, name + " has no protocol error");
        result.expectEqual(ends, receiver.ends, name + " completes expected messages");
    }

    void expectProtocolError(tests::support::TestResult& result, const std::string& name, BufferReceiver& receiver) {
        result.expectEqual(1, receiver.errors, name + " is rejected");
        result.expectEqual(1002, receiver.lastError, name + " uses protocol error status");
    }
}

int main() {
    tests::support::TestResult result;

    for (const auto& frame : {mask({0xC1, 0x82}, "hi"), mask({0xA1, 0x82}, "hi"), mask({0x91, 0x82}, "hi")}) {
        BufferReceiver r(true);
        r.feed(frame);
        expectProtocolError(result, "RSV bit", r);
    }
    { BufferReceiver r(true); r.feed(mask({0x81, 0x82}, "hi")); expectAccepted(result, "normal masked text", r); }

    for (uint8_t opcode : {0x03, 0x07, 0x0B, 0x0F}) {
        BufferReceiver r(true);
        r.feed(mask({static_cast<uint8_t>(0x80 | opcode), 0x80}));
        expectProtocolError(result, "reserved opcode", r);
    }
    for (uint8_t opcode : {0x01, 0x02, 0x09, 0x0A, 0x08}) {
        BufferReceiver r(true);
        r.feed(mask({static_cast<uint8_t>(0x80 | opcode), 0x80}));
        expectAccepted(result, "valid opcode baseline", r);
    }

    for (uint8_t opcode : {0x08, 0x09, 0x0A}) {
        BufferReceiver r(true);
        r.feed(mask({opcode, 0x80}));
        expectProtocolError(result, "fragmented control", r);
    }
    { BufferReceiver r(true); r.feed(mask({0x89, 0xFD}, std::string(125, 'x'))); expectAccepted(result, "ping length 125", r); }
    for (uint8_t opcode : {0x08, 0x09, 0x0A}) {
        BufferReceiver r(true);
        std::vector<uint8_t> frame = mask({static_cast<uint8_t>(0x80 | opcode), 0xFE, 0x00, 0x7E}, std::string(126, 'x'));
        r.feed(frame);
        expectProtocolError(result, "control extended length", r);
    }

    { BufferReceiver r(true); r.feed(mask({0x80, 0x80})); expectProtocolError(result, "continuation without start", r); }
    { BufferReceiver r(true); auto a = mask({0x01, 0x81}, "a"); auto b = mask({0x81, 0x81}, "b"); a.insert(a.end(), b.begin(), b.end()); r.feed(a); expectProtocolError(result, "new text while fragmented", r); }
    { BufferReceiver r(true); auto a = mask({0x01, 0x81}, "a"); auto b = mask({0x82, 0x81}, "b"); a.insert(a.end(), b.begin(), b.end()); r.feed(a); expectProtocolError(result, "new binary while fragmented", r); }
    { BufferReceiver r(true); auto a = mask({0x01, 0x82}, "he"); auto b = mask({0x80, 0x83}, "llo"); a.insert(a.end(), b.begin(), b.end()); r.feed(a); expectAccepted(result, "fragmented text", r); result.expectTrue(r.data == "hello", "fragmented text is delivered once in order"); }
    { BufferReceiver r(true); auto a = mask({0x02, 0x81}, "a"); auto b = mask({0x00, 0x81}, "b"); auto c = mask({0x80, 0x81}, "c"); a.insert(a.end(), b.begin(), b.end()); a.insert(a.end(), c.begin(), c.end()); r.feed(a); expectAccepted(result, "multi-continuation binary", r); result.expectTrue(r.data == "abc", "binary fragments are delivered once in order"); }
    { BufferReceiver r(true); auto a = mask({0x01, 0x81}, "a"); auto p = mask({0x89, 0x80}); auto b = mask({0x80, 0x81}, "b"); a.insert(a.end(), p.begin(), p.end()); a.insert(a.end(), b.begin(), b.end()); r.feed(a); expectAccepted(result, "ping during fragmented message", r, 2); result.expectTrue(r.data == "ab", "fragmented message continues after ping"); }

    { BufferReceiver r(true); r.feed(mask({0x81, 0x82}, "hi")); expectAccepted(result, "server accepts masked client text", r); }
    { BufferReceiver r(true); r.feed(unmasked({0x81, 0x02}, "hi")); expectProtocolError(result, "server rejects unmasked client text", r); }
    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x02}, "hi")); expectAccepted(result, "client accepts unmasked server text", r); }
    { BufferReceiver r(false); r.feed(mask({0x81, 0x82}, "hi")); expectProtocolError(result, "client rejects masked server text", r); }

    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x7D}, std::string(125, 'x'))); expectAccepted(result, "text length 125 direct", r); }
    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x7E, 0x00, 0x7D}, std::string(125, 'x'))); expectProtocolError(result, "text length 125 encoded as 126", r); }
    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x7E, 0x00, 0x7E}, std::string(126, 'x'))); expectAccepted(result, "text length 126 encoded as 126", r); }
    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7E}, std::string(126, 'x'))); expectProtocolError(result, "text length 126 encoded as 127", r); }
    { BufferReceiver r(false); r.feed(unmasked({0x81, 0x7F, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})); expectProtocolError(result, "64-bit length MSB set", r); }

    { BufferReceiver r(true); r.feed(mask({0x88, 0x80})); expectAccepted(result, "close length 0", r); }
    { BufferReceiver r(true); r.feed(mask({0x88, 0x81}, "x")); expectProtocolError(result, "close length 1", r); }
    for (uint16_t code : {uint16_t(1000), uint16_t(3000), uint16_t(4000)}) {
        BufferReceiver r(true);
        std::string payload{static_cast<char>(code >> 8), static_cast<char>(code & 0xff)};
        r.feed(mask({0x88, 0x82}, payload));
        expectAccepted(result, "valid close code", r);
    }
    for (uint16_t code : {uint16_t(999), uint16_t(1005), uint16_t(1006), uint16_t(1015), uint16_t(5000)}) {
        BufferReceiver r(true);
        std::string payload{static_cast<char>(code >> 8), static_cast<char>(code & 0xff)};
        r.feed(mask({0x88, 0x82}, payload));
        expectProtocolError(result, "invalid close code", r);
    }

    return result.processResult();
}
