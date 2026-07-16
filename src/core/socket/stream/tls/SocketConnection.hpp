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

#include "core/socket/stream/SocketConnection.hpp"
#include "core/socket/stream/tls/SocketConnection.h"
#include "core/socket/stream/tls/TLSHandshake.h"
#include "core/socket/stream/tls/TLSShutdown.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "core/socket/stream/tls/ssl_utils.h"
#include "log/Logger.h"

#include <openssl/ssl.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <cerrno>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#endif // DOXYGEN_SHOULD_SKIP_THIS

namespace core::socket::stream::tls {

    template <typename PhysicalSocket, typename Config>
    SocketConnection<PhysicalSocket, Config>::SocketConnection(PhysicalSocket&& physicalSocket,
                                                               const std::function<void(SocketConnection*)>& onDisconnect,
                                                               std::uint64_t connectionId,
                                                               const std::shared_ptr<Config>& config)
        : Super(
              std::move(physicalSocket),
              [onDisconnect, this]() {
                  onDisconnect(this);
              },
              connectionId,
              config)
        , sslInitTimeout(config->getInitTimeout())
        , sslShutdownTimeout(config->getShutdownTimeout())
        , closeNotifyIsEOF(!config->getNoCloseNotifyIsEOF())
        , tlsLifecycle(std::make_shared<TlsLifecycleControl>()) {
        tlsLifecycle->owner = this;
    }

    template <typename PhysicalSocket, typename Config>
    SocketConnection<PhysicalSocket, Config>::~SocketConnection() {
        if (tlsLifecycle != nullptr) {
            tlsLifecycle->owner = nullptr;
            tlsLifecycle->releaseRequested = true;
        }
        detachSSL();
    }

    template <typename PhysicalSocket, typename Config>
    SocketConnection<PhysicalSocket, Config>::TlsLifecycleControl::~TlsLifecycleControl() {
        if (ssl != nullptr) {
            SSL_free(ssl);
            ssl = nullptr;
        }
    }

    template <typename PhysicalSocket, typename Config>
    bool SocketConnection<PhysicalSocket, Config>::isLegalTlsTransition(TlsTransportState from, TlsTransportState to) const {
        if (from == to) {
            return true;
        }
        switch (from) {
            case TlsTransportState::Plaintext:
                return to == TlsTransportState::TlsPrepared || to == TlsTransportState::Closing || to == TlsTransportState::Closed;
            case TlsTransportState::TlsPrepared:
                return to == TlsTransportState::Plaintext || to == TlsTransportState::Handshaking || to == TlsTransportState::ShutdownInProgress ||
                       to == TlsTransportState::Closing || to == TlsTransportState::Fatal;
            case TlsTransportState::Handshaking:
                return to == TlsTransportState::TlsActive || to == TlsTransportState::Fatal || to == TlsTransportState::Closing;
            case TlsTransportState::TlsActive:
                return to == TlsTransportState::Handshaking || to == TlsTransportState::ShutdownInProgress || to == TlsTransportState::Fatal ||
                       to == TlsTransportState::Plaintext || to == TlsTransportState::Closing;
            case TlsTransportState::ShutdownInProgress:
                return to == TlsTransportState::ShutdownCompleteAwaitingRelease || to == TlsTransportState::Fatal || to == TlsTransportState::Closing;
            case TlsTransportState::ShutdownCompleteAwaitingRelease:
                return to == TlsTransportState::Plaintext || to == TlsTransportState::Closing || to == TlsTransportState::Fatal;
            case TlsTransportState::Closing:
                return to == TlsTransportState::Closed || to == TlsTransportState::Fatal;
            case TlsTransportState::Fatal:
                return to == TlsTransportState::Closing || to == TlsTransportState::Closed;
            case TlsTransportState::Closed:
                return false;
        }
        return false;
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::transitionTo(TlsTransportState next) {
        assert(isLegalTlsTransition(tlsTransportState, next));
        tlsTransportState = next;
        switch (next) {
            case TlsTransportState::Plaintext:
            case TlsTransportState::TlsActive:
                SocketWriter::unblockWriteActivation();
                break;
            case TlsTransportState::TlsPrepared:
            case TlsTransportState::Handshaking:
            case TlsTransportState::ShutdownInProgress:
            case TlsTransportState::ShutdownCompleteAwaitingRelease:
            case TlsTransportState::Closing:
            case TlsTransportState::Fatal:
            case TlsTransportState::Closed:
                SocketWriter::blockWriteActivation();
                break;
        }
    }


    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::onTlsFatalError(int errnum) {
        tlsFatalError = true;
        transitionTo(TlsTransportState::Fatal);
        errno = errnum;
        SocketConnection::close();
    }


    template <typename PhysicalSocket, typename Config>
    SSL* SocketConnection<PhysicalSocket, Config>::getSSL() const {
        return ssl;
    }

    template <typename PhysicalSocket, typename Config>
    SSL* SocketConnection<PhysicalSocket, Config>::startSSL(int fd, SSL_CTX* ctx) {
        if (ctx != nullptr) {
            if (tlsLifecycle == nullptr) {
                tlsLifecycle = std::make_shared<TlsLifecycleControl>();
                tlsLifecycle->owner = this;
            }
            releaseSSLNow();
            ssl = SSL_new(ctx);
            tlsLifecycle->ssl = ssl;
            tlsLifecycle->releaseRequested = false;

            if (ssl != nullptr) {
                SSL_set_ex_data(ssl, 0, const_cast<std::string*>(&Super::getConnectionName()));

                // Match the context default explicitly: the transport may continue in plaintext after complete TLS shutdown.
                SSL_set_read_ahead(ssl, 0);
                if (SSL_set_fd(ssl, fd) == 1 && SSL_get_read_ahead(ssl) == 0) {
                    tlsFatalError = false;
                    transitionTo(TlsTransportState::TlsPrepared);
                    SocketReader::ssl = ssl;
                    SocketWriter::ssl = ssl;
                } else {
                    tlsLifecycle->ssl = nullptr;
                    SSL_free(ssl);
                    ssl = nullptr;
                    transitionTo(TlsTransportState::Plaintext);
                }
            }
        }

        return ssl;
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::detachSSL() {
        ssl = nullptr;
        SocketReader::ssl = nullptr;
        SocketWriter::ssl = nullptr;
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::releaseSSLNow() {
        detachSSL();
        if (tlsLifecycle != nullptr) {
            if (tlsLifecycle->ssl != nullptr) {
                SSL_free(tlsLifecycle->ssl);
                tlsLifecycle->ssl = nullptr;
            }
            tlsLifecycle->releaseRequested = false;
        }
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::stopSSL() {
        if (tlsLifecycle != nullptr) {
            tlsLifecycle->releaseRequested = true;
        }
        if (sslHandshakeInProgress != nullptr && *sslHandshakeInProgress) {
            transitionTo(TlsTransportState::Closing);
            detachSSL();
            return;
        }
        if (sslShutdownInProgress != nullptr && *sslShutdownInProgress) {
            tlsShutdownIntent = TlsShutdownIntent::CloseTransport;
            tlsCloseEofPending = false;
            detachSSL();
            return;
        }
        releaseSSLNow();
        if (tlsTransportState != TlsTransportState::Closed && tlsTransportState != TlsTransportState::Fatal && tlsTransportState != TlsTransportState::Closing) {
            transitionTo(TlsTransportState::Plaintext);
        }
    }

    template <typename PhysicalSocket, typename Config>
    bool SocketConnection<PhysicalSocket, Config>::doSSLHandshake(const std::function<void()>& onSuccess,
                                                                  const std::function<void()>& onTimeout,
                                                                  const std::function<void(int)>& onStatus) {
        bool started = false;
        if (ssl != nullptr && (sslHandshakeInProgress == nullptr || !*sslHandshakeInProgress) && tlsTransportState != TlsTransportState::ShutdownInProgress && tlsTransportState != TlsTransportState::ShutdownCompleteAwaitingRelease && tlsTransportState != TlsTransportState::Fatal && tlsTransportState != TlsTransportState::Closing && tlsTransportState != TlsTransportState::Closed) {
            started = true;
            transitionTo(TlsTransportState::Handshaking);
            sslHandshakeInProgress = std::make_shared<bool>(true);
            const std::weak_ptr<bool> handshakeInProgress = sslHandshakeInProgress;
            const std::shared_ptr<TlsLifecycleControl> lifecycle = tlsLifecycle;
            if (!SocketReader::isSuspended()) {
                SocketReader::suspend();
            }
            if (!SocketWriter::isSuspended()) {
                SocketWriter::suspend();
            }

            TLSHandshake::doHandshakeWithRelease(
                Super::getConnectionName(),
                lifecycle->ssl,
                [lifecycle, onSuccess]() { // onSuccess
                    auto* owner = lifecycle->owner;
                    if (owner == nullptr) {
                        return;
                    }
                    if (lifecycle->releaseRequested || owner->tlsTransportState == TlsTransportState::Closing) {
                        return;
                    }
                    owner->transitionTo(TlsTransportState::TlsActive);
                    owner->SocketReader::span();
                    onSuccess();
                },
                [lifecycle, onTimeout]() { // onTimeout
                    auto* owner = lifecycle->owner;
                    if (owner == nullptr) {
                        return;
                    }
                    owner->transitionTo(TlsTransportState::Fatal);
                    owner->SocketConnection::close();
                    onTimeout();
                },
                [lifecycle, onStatus](int sslErr) { // onStatus
                    auto* owner = lifecycle->owner;
                    if (owner == nullptr) {
                        return;
                    }
                    owner->transitionTo(TlsTransportState::Fatal);
                    owner->SocketConnection::close();
                    onStatus(sslErr);
                },
                sslInitTimeout,
                [handshakeInProgress, lifecycle]() {
                    if (const std::shared_ptr<bool> inProgress = handshakeInProgress.lock()) {
                        *inProgress = false;
                    }
                    auto* owner = lifecycle->owner;
                    if (owner == nullptr) {
                        return;
                    }
                    if (lifecycle->releaseRequested) {
                        owner->releaseSSLNow();
                        return;
                    }
                    if (owner->pendingShutdownAfterHandshake && owner->tlsTransportState == TlsTransportState::TlsActive) {
                        owner->pendingShutdownAfterHandshake = false;
                        owner->startPendingTlsShutdown();
                    }
                });
        }

        return started;
    }

    template <typename PhysicalSocket, typename Config>
    bool SocketConnection<PhysicalSocket, Config>::preserveTlsHandoffBytes() {
        if (ssl == nullptr) {
            return true;
        }

        // Safe TLS-to-plaintext handoff relies on read-ahead being disabled on both the context and this SSL object:
        // the same transport may continue in plaintext after complete TLS shutdown, so only decrypted application data
        // that OpenSSL exposes through SSL_read() may be transferred into the raw reader handoff buffer.
        std::array<char, 4096> buffer{};
        std::vector<char> candidateHandoff;
        while (SSL_pending(ssl) > 0) {
            const int ret = SSL_read(ssl, buffer.data(), static_cast<int>(buffer.size()));
            if (ret <= 0) {
                return false;
            }
            candidateHandoff.insert(candidateHandoff.end(), buffer.data(), buffer.data() + ret);
        }

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
        if (SSL_has_pending(ssl) != 0) {
            return false;
        }
#endif
        if (!candidateHandoff.empty()) {
            SocketReader::appendHandoffBytes(candidateHandoff.data(), candidateHandoff.size());
        }
        return true;
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::drainTlsShutdownCallbacks() {
        auto callbacks = std::move(tlsShutdownCompletionCallbacks);
        tlsShutdownCompletionCallbacks.clear();
        for (const auto& callback : callbacks) {
            if (callback) {
                callback();
            }
        }
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::completeTlsShutdownAfterRelease() {
        if (tlsShutdownFailurePending) {
            finalizeTlsCloseTransport(tlsCloseEofPending);
            return;
        }
        if (tlsShutdownIntent == TlsShutdownIntent::ContinuePlaintext && tlsShutdownFullComplete) {
            if (!preserveTlsHandoffBytes()) {
                markTlsShutdownFailure(EPROTO);
                finalizeTlsCloseTransport(false);
                return;
            }
            releaseSSLNow();
            transitionTo(TlsTransportState::Plaintext);
            SocketReader::resume();
            drainTlsShutdownCallbacks();
            return;
        }

        finalizeTlsCloseTransport(tlsCloseEofPending);
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::finalizeTlsCloseTransport(bool reportCleanEof) {
        if (tlsCloseFinalizationInProgress) {
            return;
        }
        tlsCloseFinalizationInProgress = true;
        SocketWriter::shutdownInProgress = true;
        SocketWriter::markShutdown = false;
        transitionTo(TlsTransportState::Closing);
        releaseSSLNow();
        const std::shared_ptr<TlsLifecycleControl> lifecycle = tlsLifecycle;
        Super::doWriteShutdown([lifecycle, reportCleanEof]() {
            auto* owner = lifecycle->owner;
            if (owner == nullptr) {
                return;
            }
            owner->SocketWriter::shutdownInProgress = true;
            owner->transitionTo(TlsTransportState::Closed);
            owner->SocketConnection::close();
            auto callbacks = std::exchange(owner->tlsShutdownCompletionCallbacks, {});
            const bool emitEof = reportCleanEof && owner->getSocketContext() != nullptr;
            owner->tlsCloseEofPending = false;

            for (const auto& callback : callbacks) {
                if (callback) {
                    callback();
                }
            }

            if (emitEof) {
                auto* currentOwner = lifecycle->owner;
                if (currentOwner != nullptr) {
                    currentOwner->onReadError(0);
                }
            }
        });
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::markTlsShutdownFailure(int errnum) {
        tlsShutdownFailurePending = true;
        tlsShutdownFailureErrno = errnum;
        tlsFatalError = true;
        errno = errnum;
        transitionTo(TlsTransportState::Fatal);
        tlsShutdownIntent = TlsShutdownIntent::CloseTransport;
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::requestTlsShutdown(TlsShutdownIntent intent, const std::function<void()>& onComplete) {
        if (tlsTransportState == TlsTransportState::Fatal || tlsTransportState == TlsTransportState::Closing || tlsTransportState == TlsTransportState::Closed) {
            return;
        }
        if (onComplete) {
            tlsShutdownCompletionCallbacks.push_back(onComplete);
        }
        if (intent == TlsShutdownIntent::CloseTransport) {
            tlsShutdownIntent = TlsShutdownIntent::CloseTransport;
        } else if (tlsShutdownIntent != TlsShutdownIntent::CloseTransport) {
            tlsShutdownIntent = TlsShutdownIntent::ContinuePlaintext;
        }
        tlsShutdownPending = true;
        startPendingTlsShutdown();
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::startPendingTlsShutdown() {
        if (!tlsShutdownPending || ssl == nullptr || tlsTransportState == TlsTransportState::Fatal || tlsTransportState == TlsTransportState::Closing || tlsTransportState == TlsTransportState::Closed) {
            return;
        }
        if (sslHandshakeInProgress != nullptr && *sslHandshakeInProgress) {
            pendingShutdownAfterHandshake = true;
            return;
        }
        if (sslShutdownInProgress != nullptr && *sslShutdownInProgress) {
            return;
        }

        tlsShutdownPending = false;
        transitionTo(TlsTransportState::ShutdownInProgress);
        tlsShutdownSemanticComplete = false;
        tlsShutdownFullComplete = false;
        sslShutdownInProgress = std::make_shared<bool>(true);
        const std::weak_ptr<bool> shutdownInProgress = sslShutdownInProgress;
        const std::shared_ptr<TlsLifecycleControl> lifecycle = tlsLifecycle;

        if (!SocketReader::isSuspended()) {
            SocketReader::suspend();
        }

        TLSShutdown::doShutdownTypedWithRelease(
            Super::getConnectionName(),
            lifecycle->ssl,
            [lifecycle](TLSShutdown::TypedSuccess success) {
                auto* owner = lifecycle->owner;
                if (owner == nullptr) {
                    return;
                }
                owner->tlsShutdownSemanticComplete = true;
                owner->tlsShutdownFullComplete = success == TLSShutdown::TypedSuccess::FullShutdownComplete;
                owner->transitionTo(TlsTransportState::ShutdownCompleteAwaitingRelease);
            },
            [lifecycle]() {
                auto* owner = lifecycle->owner;
                if (owner == nullptr) {
                    return;
                }
                owner->markTlsShutdownFailure(ETIMEDOUT);
            },
            [lifecycle](int sslErr) {
                auto* owner = lifecycle->owner;
                if (owner == nullptr) {
                    return;
                }
                ssl_log(owner->Super::getConnectionName() + " SSL/TLS: Shutdown handshake failed", sslErr);
                owner->markTlsShutdownFailure(errno != 0 ? errno : EPROTO);
            },
            sslShutdownTimeout,
            [shutdownInProgress, lifecycle]() {
                if (const std::shared_ptr<bool> inProgress = shutdownInProgress.lock()) {
                    *inProgress = false;
                }
                auto* owner = lifecycle->owner;
                if (owner == nullptr) {
                    return;
                }
                if (owner->tlsShutdownFailurePending) {
                    owner->completeTlsShutdownAfterRelease();
                    return;
                }
                if (lifecycle->releaseRequested && owner->tlsTransportState != TlsTransportState::ShutdownCompleteAwaitingRelease) {
                    owner->releaseSSLNow();
                    return;
                }
                if (owner->tlsTransportState == TlsTransportState::ShutdownCompleteAwaitingRelease) {
                    owner->completeTlsShutdownAfterRelease();
                }
            },
            TLSShutdown::CompletionRequirement::RequireFullShutdown);
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::doSSLShutdown() {
        requestTlsShutdown(TlsShutdownIntent::CloseTransport);
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::onReadShutdown() {
        core::socket::stream::SocketConnection::log().debug("SSL/TLS: Passive close_notify received, requesting connection-owned TLS shutdown");
        const TlsShutdownIntent intent = closeNotifyIsEOF ? TlsShutdownIntent::CloseTransport : TlsShutdownIntent::ContinuePlaintext;
        if (intent == TlsShutdownIntent::CloseTransport) {
            tlsCloseEofPending = true;
            requestTlsShutdown(intent);
        } else {
            requestTlsShutdown(intent);
        }
    }

    template <typename PhysicalSocket, typename Config>
    void SocketConnection<PhysicalSocket, Config>::doWriteShutdown(const std::function<void()>& onShutdown) {
        if (tlsFatalError || ssl == nullptr) {
            Super::doWriteShutdown(onShutdown);
        } else {
            core::socket::stream::SocketConnection::log().debug("SSL/TLS: Active send close_notify");
            requestTlsShutdown(TlsShutdownIntent::CloseTransport, onShutdown);
        }
    }

} // namespace core::socket::stream::tls
