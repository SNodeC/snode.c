#ifndef WSTRANSMITTER_H
#define WSTRANSMITTER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#define WSPAYLOADLENGTH 4

namespace http::websocket {

    class WSTransmitter {
    public:
        WSTransmitter();

        void messageStart(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void message(const char* message, std::size_t messageLength, uint32_t messageKey = 0);
        void messageEnd(const char* message, std::size_t messageLength, uint32_t messageKey = 0);

        void message(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0);

    protected:
        union MaskingKeyAsArray {
            uint32_t value;
            char array[4];
        };

        void send(bool end, uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey);
        void sendFrame(bool fin, uint8_t opCode, uint32_t maskingKey, const char* payload, uint64_t payloadLength);
        void dumpFrame(char* frame, uint64_t frameLength);

        virtual void onFrameReady(char* frame, uint64_t frameLength) = 0;
    };

} // namespace http::websocket

#endif // WSTRANSMITTER_H
