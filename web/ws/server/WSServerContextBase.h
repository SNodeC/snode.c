#ifndef WSSERVERCONTEXTBASE_H
#define WSSERVERCONTEXTBASE_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <cstddef>
#include <cstdint>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace web::ws::server {

    class WSServerContextBase {
    public:
        /* WSServerContextBase */
        virtual void sendMessageStart(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0) = 0;
        virtual void sendMessageFrame(const char* message, std::size_t messageLength, uint32_t messageKey = 0) = 0;
        virtual void sendMessageEnd(const char* message, std::size_t messageLength, uint32_t messageKey = 0) = 0;
        virtual void sendMessage(uint8_t opCode, const char* message, std::size_t messageLength, uint32_t messageKey = 0) = 0;
        virtual void sendPing(const char* reason = nullptr, std::size_t reasonLength = 0) = 0;
        virtual void close(uint16_t statusCode = 1000, const char* reason = nullptr, std::size_t reasonLength = 0) = 0;

        virtual std::string getLocalAddressAsString() const = 0;
        virtual std::string getRemoteAddressAsString() const = 0;
    };

} // namespace web::ws::server

#endif // WSSERVERCONTEXTBASE_H
