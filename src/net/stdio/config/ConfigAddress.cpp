// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
#include "net/stdio/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"
#include "net/config/ConfigAddressLocal.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

namespace net::stdio::config {

    ConfigAddress::ConfigAddress(net::config::ConfigInstance* instance)
        : ConfigSection(instance, this)
        , Super(this) {
        inputFdOpt = addOption("--input-fd", "Protocol input descriptor", "fd", 0, CLI::TypeValidator<int>() & CLI::NonNegativeNumber);
        outputFdOpt = addOption("--output-fd", "Protocol output descriptor", "fd", 1, CLI::TypeValidator<int>() & CLI::NonNegativeNumber);
        ownershipOpt = addOption("--ownership", "Descriptor ownership", "take|borrow", "take", CLI::IsMember({"take", "borrow"}));
    }

    SocketAddress* ConfigAddress::init() {
        auto* address = new SocketAddress(getInputFd(), getOutputFd());
        return address;
    }

    ConfigAddress* ConfigAddress::setSocketAddress(const SocketAddress& address) {
        setInputFd(address.getInputFd());
        setOutputFd(address.getOutputFd());
        return this;
    }

    ConfigAddress* ConfigAddress::setInputFd(int inputFd) {
        setDefaultValue(inputFdOpt, inputFd);
        return this;
    }

    int ConfigAddress::getInputFd() const {
        return inputFdOpt->as<int>();
    }

    ConfigAddress* ConfigAddress::setOutputFd(int outputFd) {
        setDefaultValue(outputFdOpt, outputFd);
        return this;
    }

    int ConfigAddress::getOutputFd() const {
        return outputFdOpt->as<int>();
    }

    ConfigAddress* ConfigAddress::setDescriptorOwnership(DescriptorOwnership ownership) {
        setDefaultValue(ownershipOpt, ownership == DescriptorOwnership::TAKE ? "take" : "borrow");
        return this;
    }

    DescriptorOwnership ConfigAddress::getDescriptorOwnership() const {
        return ownershipOpt->as<std::string>() == "take" ? DescriptorOwnership::TAKE : DescriptorOwnership::BORROW;
    }

    void ConfigAddress::configurable(bool configurable) {
        setConfigurable(inputFdOpt, configurable);
        setConfigurable(outputFdOpt, configurable);
        setConfigurable(ownershipOpt, configurable);
    }

    ConfigAddressRemote::ConfigAddressRemote(net::config::ConfigInstance* instance)
        : ConfigAddressBase(instance) {
    }

    ConfigAddressRemote::~ConfigAddressRemote() {
    }

} // namespace net::stdio::config

template class net::config::ConfigAddressBase<net::stdio::SocketAddress>;
template class net::config::ConfigAddress<net::stdio::SocketAddress>;
template class net::config::ConfigAddressLocal<net::stdio::SocketAddress>;
