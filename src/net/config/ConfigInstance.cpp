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

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "net/config/ConfigInstance.h"

#include "utils/Config.h"

#include <functional>
#include <memory>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

namespace net::config {

    const std::string ConfigInstance::nameAnonymous = "<anonymous>";

    ConfigInstance::ConfigInstance(const std::string& instanceName, Role role)
        : utils::SubCommand(!instanceName.empty() ? &utils::Config::configRoot : nullptr,
                            std::make_shared<utils::AppWithPtr>(std::string("Configuration for ")
                                                                    .append(role == Role::SERVER ? "server" : "client")
                                                                    .append(" instance '")
                                                                    .append(instanceName)
                                                                    .append("'"),
                                                                instanceName,
                                                                this),
                            "Instances",
                            false)
        , instanceName(instanceName)
        , role(role)
        , disableOpt(setConfigurable(addFlagFunction(
                                         "--disabled{true}",
                                         [this]() {
                                             if (getParent() != nullptr) {
                                                 getParent()->disabled(this, disableOpt->as<bool>());
                                             }
                                         },
                                         "Disable this instance",
                                         "bool",
                                         "false",
                                         CLI::IsMember({"true", "false"})),
                                     true))
        , onDestroy([](ConfigInstance*) {
        }) {
    }

    ConfigInstance::~ConfigInstance() {
        removeSubCommand();

        onDestroy(this);
    }

    const std::string& ConfigInstance::getInstanceName() const {
        return instanceName.empty() ? nameAnonymous : instanceName;
    }

    ConfigInstance& ConfigInstance::setInstanceName(const std::string& instanceName) {
        this->instanceName = instanceName;

        return *this;
    }

    bool ConfigInstance::getDisabled() const {
        return disableOpt //
            ->as<bool>();
    }

    ConfigInstance& ConfigInstance::setDisabled(bool disabled) {
        setDefaultValue(disableOpt, disabled ? "true" : "false");

        if (getParent() != nullptr) {
            getParent()->disabled(this, disabled);
        }

        return *this;
    }

    ConfigInstance& ConfigInstance::configurable(bool configurable) {
        setConfigurable(disableOpt, configurable);

        return *this;
    }

    ConfigInstance& ConfigInstance::setOnDestroy(const std::function<void(ConfigInstance*)>& onDestroy) {
        const std::function<void(ConfigInstance*)> oldOnDestroy = this->onDestroy;
        this->onDestroy = [oldOnDestroy, onDestroy](ConfigInstance* configInstance) {
            oldOnDestroy(configInstance);
            onDestroy(configInstance);
        };

        return *this;
    }

    CLI::App* ConfigInstance::getHelpTriggerApp() {
        return utils::ConfigRoot::getHelpTriggerApp();
    }

    CLI::App* ConfigInstance::getShowConfigTriggerApp() {
        return utils::ConfigRoot::getShowConfigTriggerApp();
    }
    CLI::App* ConfigInstance::getCommandlineTriggerApp() {
        return utils::ConfigRoot::getCommandlineTriggerApp();
    }

} // namespace net::config
