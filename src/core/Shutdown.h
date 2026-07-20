/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef CORE_SHUTDOWN_H
#define CORE_SHUTDOWN_H

namespace core {

    enum class ShutdownReason {
        Requested,
        Signal,
        NoObserver,
    };

    struct ShutdownContext {
        ShutdownReason reason = ShutdownReason::Requested;
        int signal = 0;
    };

} // namespace core

#endif // CORE_SHUTDOWN_H
