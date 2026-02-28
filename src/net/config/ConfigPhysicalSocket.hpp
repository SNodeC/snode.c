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

#include "ConfigPhysicalSocket.h"
#include "net/config/ConfigSection.hpp"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include <functional>

#endif // DOXYGEN_SHOULD_SKIP_THIS

#define XSTR(s) STR(s)
#define STR(s) #s

namespace net::config {

    template <typename ConcretConfigPhysicalSocket>
    ConfigPhysicalSocket::ConfigPhysicalSocket(ConfigInstance* instance, ConcretConfigPhysicalSocket* section)
        : ConfigSection(instance, section) {
        retryOpt = addFlag( //
            "--retry{true}",
            "Automatically retry listen|connect",
            "bool",
            retry,
            CLI::IsMember({"true", "false"}));

        retryOnFatalOpt = addFlagFunction( //
            "--retry-on-fatal{true}",
            [this]([[maybe_unused]] std::uint64_t) {
                if (retryOnFatalOpt->as<bool>() && !retryOpt->as<bool>()) {
                    throw CLI::RequiresError(retryOnFatalOpt->get_name(), retryOpt->get_name().append("=true"));
                }
            },
            "Retry also on fatal errors",
            "bool",
            retryOnFatal,
            CLI::IsMember({"true", "false"}));
        retryOnFatalOpt->needs(retryOpt);

        retryTimeoutOpt = addOption( //
            "--retry-timeout",
            "Timeout of the retry timer",
            "sec",
            retryTimeout,
            CLI::NonNegativeNumber);
        retryTimeoutOpt->needs(retryOpt);

        retryTriesOpt = addOption( //
            "--retry-tries",
            "Number of retry attempts before giving up (0 = unlimited)",
            "tries",
            retryTries,
            CLI::TypeValidator<unsigned int>());
        retryTriesOpt->needs(retryOpt);

        retryBaseOpt = addOption( //
            "--retry-base",
            "Base of exponential backoff",
            "base",
            retryBase,
            CLI::PositiveNumber);
        retryBaseOpt->needs(retryOpt);

        retryJitterOpt = addOption( //
            "--retry-jitter",
            "Maximum jitter in percent to apply randomly to calculated retry timeout (0 to disable)",
            "jitter",
            retryJitter,
            CLI::Range(0., 100.));
        retryJitterOpt->needs(retryOpt);

        retryLimitOpt = addOption( //
            "--retry-limit",
            "Upper limit in seconds of retry timeout (0 for infinite)",
            "sec",
            retryLimit,
            CLI::NonNegativeNumber);
        retryLimitOpt->needs(retryOpt);
    }

} // namespace net::config
