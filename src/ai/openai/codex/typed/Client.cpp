/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Client.h"

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"

#include <cerrno>
#include <exception>
#include <functional>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ai::openai::codex::typed {

    namespace {
        AppServerClient::RawProtocol::Submission submissionFailure(std::string message) {
            return {std::nullopt, Error{Error::Category::Protocol, EINVAL, std::move(message)}};
        }

        template <typename T, typename Handler, typename Decoder>
        AppServerClient::RawProtocol::ResponseHandler adaptResponse(Handler handler, Decoder decoder) {
            return [handler = std::move(handler), decoder = std::move(decoder)](const Response& response) {
                OperationResult<T> result;
                result.requestId = response.id;
                result.raw = response.result;

                switch (response.kind) {
                    case Response::Kind::RemoteError:
                        result.kind = OperationResult<T>::Kind::RemoteError;
                        result.remoteError = response.remoteError;
                        break;
                    case Response::Kind::Cancelled:
                        result.kind = OperationResult<T>::Kind::Cancelled;
                        result.localError = response.localError;
                        break;
                    case Response::Kind::Result: {
                        std::string decodingError;
                        try {
                            result.value = decoder(response.result, decodingError);
                        } catch (const std::exception& exception) {
                            decodingError = std::string("typed App Server result decoder threw: ") + exception.what();
                        } catch (...) {
                            decodingError = "typed App Server result decoder threw an unknown exception";
                        }

                        if (result.value) {
                            result.kind = OperationResult<T>::Kind::Success;
                        } else {
                            result.kind = OperationResult<T>::Kind::LocalError;
                            if (decodingError.empty()) {
                                decodingError = "typed App Server result could not be decoded";
                            }
                            result.localError = Error{Error::Category::Protocol, EINVAL, std::move(decodingError)};
                        }
                        break;
                    }
                }

                handler(result);
            };
        }
    } // namespace

    Threads::Threads(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    Threads::Submission Threads::start(ThreadStartOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/start requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeThreadStartParams(options, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed thread/start parameters could not be encoded"
                                                           : std::move(encodingError));
        }

        return protocol->request(
            "thread/start", std::move(*params), adaptResponse<Thread>(std::move(handler), [](const Json& value, std::string& error) {
                return detail::decodeThreadOperationResult(value, error);
            }));
    }

    Threads::Submission Threads::resume(ThreadId threadId, ThreadResumeOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/resume requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeThreadResumeParams(threadId, options, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed thread/resume parameters could not be encoded"
                                                           : std::move(encodingError));
        }

        return protocol->request(
            "thread/resume", std::move(*params), adaptResponse<Thread>(std::move(handler), [](const Json& value, std::string& error) {
                return detail::decodeThreadOperationResult(value, error);
            }));
    }

    Threads::Submission Threads::list(ThreadListOptions options, ThreadListResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/list requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeThreadListParams(options, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed thread/list parameters could not be encoded"
                                                           : std::move(encodingError));
        }

        return protocol->request(
            "thread/list", std::move(*params), adaptResponse<ThreadPage>(std::move(handler), [](const Json& value, std::string& error) {
                return detail::decodeThreadListResult(value, error);
            }));
    }

    Threads::Submission Threads::read(ThreadId threadId, ThreadResultHandler handler) {
        return read(std::move(threadId), {}, std::move(handler));
    }

    Threads::Submission Threads::read(ThreadId threadId, ThreadReadOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/read requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeThreadReadParams(threadId, options, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed thread/read parameters could not be encoded"
                                                           : std::move(encodingError));
        }

        return protocol->request(
            "thread/read", std::move(*params), adaptResponse<Thread>(std::move(handler), [](const Json& value, std::string& error) {
                return detail::decodeThreadReadResult(value, error);
            }));
    }

    Turns::Turns(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    Turns::Submission Turns::start(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options, TurnResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed turn/start requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeTurnStartParams(threadId, input, options, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed turn/start input could not be encoded" : std::move(encodingError));
        }

        return protocol->request(
            "turn/start",
            std::move(*params),
            adaptResponse<Turn>(std::move(handler), [threadId = std::move(threadId)](const Json& value, std::string& error) {
                return detail::decodeTurnStartResult(value, threadId, error);
            }));
    }

    Turns::Submission Turns::interrupt(ThreadId threadId, TurnId turnId, InterruptResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed turn/interrupt requires a result handler");
        }

        std::string encodingError;
        std::optional<Json> params = detail::encodeTurnInterruptParams(threadId, turnId, encodingError);
        if (!params) {
            return submissionFailure(encodingError.empty() ? "typed turn/interrupt parameters could not be encoded"
                                                           : std::move(encodingError));
        }

        return protocol->request("turn/interrupt",
                                 std::move(*params),
                                 adaptResponse<TurnInterruptResult>(std::move(handler), [](const Json& value, std::string& error) {
                                     return detail::decodeTurnInterruptResult(value, error);
                                 }));
    }

    Events::Events(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    void Events::setOnEvent(EventHandler handler) {
        if (!handler) {
            protocol->setTypedNotificationDispatcher({});
            return;
        }

        protocol->setTypedNotificationDispatcher([handler = std::move(handler)](const Notification& notification) {
            const Event event = detail::decodeEvent(notification);
            handler(event);
        });
    }

    Requests::Requests(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    void Requests::setOnRequest(RequestHandler handler) {
        if (!handler) {
            protocol->setTypedServerRequestDispatcher({});
            return;
        }

        protocol->setTypedServerRequestDispatcher([handler = std::move(handler)](const ServerRequest& request) {
            const TypedServerRequest typedRequest = detail::decodeServerRequest(request);
            handler(typedRequest);
        });
    }

    Requests::SendResult Requests::respond(const CommandApprovalRequest& request, ApprovalDecision decision) {
        if (decision.value.empty()) {
            return validationFailure("approval decision must not be empty");
        }
        return protocol->respondOwned(request.requestId, request.requestToken, Json{{"decision", std::move(decision.value)}});
    }

    Requests::SendResult Requests::respond(const FileChangeApprovalRequest& request, ApprovalDecision decision) {
        if (decision.value.empty()) {
            return validationFailure("approval decision must not be empty");
        }
        return protocol->respondOwned(request.requestId, request.requestToken, Json{{"decision", std::move(decision.value)}});
    }

    Requests::SendResult Requests::respond(const UserInputRequest& request, std::vector<UserInputAnswer> answers) {
        std::set<std::string> questionIds;
        for (const UserInputQuestion& question : request.questions) {
            questionIds.insert(question.id);
        }

        std::set<std::string> answeredIds;
        Json encodedAnswers = Json::object();
        for (UserInputAnswer& answer : answers) {
            if (!questionIds.contains(answer.questionId)) {
                return validationFailure("user-input answer refers to an unknown question ID: " + answer.questionId);
            }
            if (!answeredIds.insert(answer.questionId).second) {
                return validationFailure("duplicate user-input answer for question ID: " + answer.questionId);
            }
            encodedAnswers[answer.questionId] = Json{{"answers", std::move(answer.answers)}};
        }

        return protocol->respondOwned(request.requestId, request.requestToken, Json{{"answers", std::move(encodedAnswers)}});
    }

    Requests::SendResult Requests::respond(const AuthenticationRequest& request, AuthenticationResponse response) {
        Json result{{"accessToken", std::move(response.accessToken)},
                    {"chatgptAccountId", std::move(response.chatgptAccountId)},
                    {"chatgptPlanType", response.chatgptPlanType ? Json(std::move(*response.chatgptPlanType)) : Json(nullptr)}};
        return protocol->respondOwned(request.requestId, request.requestToken, std::move(result));
    }

    Requests::SendResult Requests::respondRaw(const UnknownServerRequest& request, Json result) {
        return protocol->respondOwned(request.requestId, request.requestToken, std::move(result));
    }

    Requests::SendResult Requests::reject(const UnknownServerRequest& request, ProtocolError error) {
        return protocol->rejectOwned(request.requestId, request.requestToken, std::move(error));
    }

    Requests::SendResult Requests::validationFailure(std::string message) {
        return {false, Error{Error::Category::Protocol, EINVAL, std::move(message)}};
    }

} // namespace ai::openai::codex::typed
