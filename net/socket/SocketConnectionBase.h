#ifndef SOCKETCONNECTION_H
#define SOCKETCONNECTION_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include "AttributeInjector.h"


class SocketConnectionBase {
public:
    SocketConnectionBase(const SocketConnectionBase&) = delete;
    SocketConnectionBase& operator=(const SocketConnectionBase&) = delete;

    virtual ~SocketConnectionBase() = default;

    virtual void enqueue(const char* buffer, int size) = 0;

    virtual void end() = 0;

    template <utils::InjectedAttribute Attribute>
    constexpr void setProtocol(Attribute& attribute) const {
        protocol.setAttribute<Attribute>(attribute);
    }

    template <utils::InjectedAttribute Attribute>
    constexpr void setProtocol(Attribute&& attribute) const {
        protocol.setAttribute<Attribute>(attribute);
    }

    template <utils::InjectedAttribute Attribute>
    constexpr bool getProtocol(std::function<void(Attribute&)> onFound) const {
        return protocol.getAttribute<Attribute>([&onFound](Attribute& attribute) -> void {
            onFound(attribute);
        });
    }

    template <utils::InjectedAttribute Attribute>
    constexpr void getProtocol(std::function<void(Attribute&)> onFound, std::function<void(const std::string&)> onNotFound) const {
        return protocol.getAttribute<Attribute>(
            [&onFound](Attribute& attribute) -> void {
                onFound(attribute);
            },
            [&onNotFound](const std::string& msg) -> void {
                onNotFound(msg);
            });
    }

protected:
    SocketConnectionBase() = default;

private:
    utils::SingleAttributeInjector protocol;
};

#endif // SOCKETCONNECTION_H
