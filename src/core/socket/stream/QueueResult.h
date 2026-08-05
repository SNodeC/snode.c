/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#ifndef CORE_SOCKET_STREAM_QUEUERESULT_H
#define CORE_SOCKET_STREAM_QUEUERESULT_H

namespace core::socket::stream {

    enum class QueueResult { Queued, WouldExceedLimit, Closed, ShutdownInProgress };

} // namespace core::socket::stream

#endif // CORE_SOCKET_STREAM_QUEUERESULT_H
