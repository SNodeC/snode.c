/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "net/stdio/config/ConfigAddress.h"

#include "net/config/ConfigAddressBase.hpp"
#include "net/config/ConfigAddressLocal.hpp" // IWYU pragma: keep
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

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
