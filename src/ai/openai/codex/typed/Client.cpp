/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "ai/openai/codex/typed/Client.h"

#include "ai/openai/codex/AppServerClient.h"
#include "ai/openai/codex/Protocol.h"
#include "ai/openai/codex/detail/AccountCodec.h"
#include "ai/openai/codex/detail/CodexErrorInfoCodec.h"
#include "ai/openai/codex/detail/ClientOperationCodec.h"
#include "ai/openai/codex/detail/EventDecoder.h"
#include "ai/openai/codex/detail/ProtocolSurfaceRegistry.h"
#include "ai/openai/codex/detail/ServerRequestDecoder.h"
#include "ai/openai/codex/detail/ThreadCodec.h"
#include "ai/openai/codex/detail/TurnCodec.h"
#include "ai/openai/codex/typed/Accounts.h"
#include "ai/openai/codex/typed/Conversation.h"
#include "ai/openai/codex/typed/Events.h"
#include "ai/openai/codex/typed/Results.h"
#include "ai/openai/codex/typed/ServerRequests.h"
#include "ai/openai/codex/typed/Threads.h"
#include "ai/openai/codex/typed/Turns.h"
#include "ai/openai/codex/typed/Types.h"

#include <cerrno>
#include <exception>
#include <functional>
#include <memory>
#include <nlohmann/detail/json_ref.hpp>
#include <nlohmann/json.hpp>
#include <optional>
#include <set>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace ai::openai::codex::typed {

    class Client::Impl {
    public:
        Impl(std::unique_ptr<Accounts> accounts,
             std::unique_ptr<Threads> threads,
             std::unique_ptr<Turns> turns,
             std::unique_ptr<Events> events,
             std::unique_ptr<Requests> requests)
            : accounts(std::move(accounts))
            , threads(std::move(threads))
            , turns(std::move(turns))
            , events(std::move(events))
            , requests(std::move(requests)) {
        }

        std::unique_ptr<Accounts> accounts;
        std::unique_ptr<Threads> threads;
        std::unique_ptr<Turns> turns;
        std::unique_ptr<Events> events;
        std::unique_ptr<Requests> requests;
    };

    Client::Client(std::unique_ptr<Accounts> accounts,
                   std::unique_ptr<Threads> threads,
                   std::unique_ptr<Turns> turns,
                   std::unique_ptr<Events> events,
                   std::unique_ptr<Requests> requests)
        : impl(std::make_unique<Impl>(std::move(accounts), std::move(threads), std::move(turns), std::move(events), std::move(requests))) {
    }

    Client::~Client() = default;

    Accounts& Client::accounts() noexcept {
        return *impl->accounts;
    }

    const Accounts& Client::accounts() const noexcept {
        return *impl->accounts;
    }

    Threads& Client::threads() noexcept {
        return *impl->threads;
    }

    const Threads& Client::threads() const noexcept {
        return *impl->threads;
    }

    Turns& Client::turns() noexcept {
        return *impl->turns;
    }

    const Turns& Client::turns() const noexcept {
        return *impl->turns;
    }

    Events& Client::events() noexcept {
        return *impl->events;
    }

    const Events& Client::events() const noexcept {
        return *impl->events;
    }

    Requests& Client::requests() noexcept {
        return *impl->requests;
    }

    const Requests& Client::requests() const noexcept {
        return *impl->requests;
    }

    namespace {
        AppServerClient::RawProtocol::Submission submissionFailure(std::string message) {
            return {std::nullopt, Error{Error::Category::Protocol, EINVAL, std::move(message)}};
        }

        std::optional<Json> encodeUnitParams(const Unit&, std::string& error) {
            error.clear();
            return std::optional<Json>{Json(nullptr)};
        }

        std::string registeredMethod(detail::ClientRequestTarget target) {
            return std::string(detail::entryFor(target).key.name);
        }

        template <typename T, typename Handler>
        AppServerClient::RawProtocol::ResponseHandler
        adaptResponse(Handler handler,
                      detail::ClientRequestTarget target,
                      std::optional<ThreadId> contextualThreadId) {
            return [handler = std::move(handler),
                    target,
                    contextualThreadId = std::move(contextualThreadId)](
                       const Response& response) {
                OperationResult<T> result;
                result.requestId = response.id;
                result.raw = response.result;

                switch (response.kind) {
                    case Response::Kind::RemoteError:
                        result.kind = OperationResult<T>::Kind::RemoteError;
                        result.remoteError = response.remoteError;
                        if (result.remoteError) {
                            detail::decodeProtocolErrorTurnInfo(
                                *result.remoteError, result.codexErrorInfo, result.codexErrorDiagnostic);
                        }
                        break;
                    case Response::Kind::Cancelled:
                        result.kind = OperationResult<T>::Kind::Cancelled;
                        result.localError = response.localError;
                        break;
                    case Response::Kind::Result: {
                        std::string decodingError;
                        try {
                            result.value = detail::decodeClientOperationResultAs<T>(
                                target,
                                response.result,
                                contextualThreadId,
                                decodingError);
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

        template <typename T, typename Params, typename Handler, typename Encoder>
        AppServerClient::RawProtocol::Submission submitTypedRequest(AppServerClient::RawProtocol* protocol,
                                                                   detail::ClientRequestTarget target,
                                                                   const Params& params,
                                                                   Handler handler,
                                                                   Encoder encoder,
                                                                   std::optional<ThreadId> contextualThreadId = std::nullopt) {
            const std::string method = registeredMethod(target);
            if (!handler) {
                return submissionFailure("typed " + method + " requires a result handler");
            }

            std::string encodingError;
            std::optional<Json> encodedParams = encoder(params, encodingError);
            if (!encodedParams) {
                return submissionFailure(encodingError.empty() ? "typed " + method + " parameters could not be encoded"
                                                               : std::move(encodingError));
            }

            return protocol->request(
                method,
                std::move(*encodedParams),
                adaptResponse<T>(
                    std::move(handler), target, std::move(contextualThreadId)));
        }

        template <typename To, typename From, typename Mapper>
        OperationResult<To> mapOperationResult(const OperationResult<From>& source, Mapper mapper) {
            OperationResult<To> result;
            switch (source.kind) {
                case OperationResult<From>::Kind::Success:
                    result.kind = OperationResult<To>::Kind::Success;
                    if (source.value) {
                        result.value = mapper(*source.value);
                    }
                    break;
                case OperationResult<From>::Kind::RemoteError:
                    result.kind = OperationResult<To>::Kind::RemoteError;
                    break;
                case OperationResult<From>::Kind::Cancelled:
                    result.kind = OperationResult<To>::Kind::Cancelled;
                    break;
                case OperationResult<From>::Kind::LocalError:
                    result.kind = OperationResult<To>::Kind::LocalError;
                    break;
            }
            result.remoteError = source.remoteError;
            result.localError = source.localError;
            result.requestId = source.requestId;
            result.raw = source.raw;
            result.codexErrorInfo = source.codexErrorInfo;
            result.codexErrorDiagnostic = source.codexErrorDiagnostic;
            return result;
        }
    } // namespace

    Accounts::Accounts(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    Accounts::Submission Accounts::cancelLogin(CancelLoginAccountParams params, CancelLoginResultHandler handler) {
        return submitTypedRequest<CancelLoginAccountResponse>(
            protocol, detail::ClientRequestTarget::AccountLoginCancel, params, std::move(handler), detail::encodeCancelLoginAccountParams);
    }

    Accounts::Submission Accounts::startLogin(LoginAccountParams params, StartLoginResultHandler handler) {
        return submitTypedRequest<LoginAccountResponse>(
            protocol, detail::ClientRequestTarget::AccountLoginStart, params, std::move(handler), detail::encodeLoginAccountParams);
    }

    Accounts::Submission Accounts::logout(Unit params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(protocol, detail::ClientRequestTarget::AccountLogout, params, std::move(handler), encodeUnitParams);
    }

    Accounts::Submission Accounts::consumeRateLimitResetCredit(ConsumeAccountRateLimitResetCreditParams params,
                                                               ConsumeRateLimitResetCreditResultHandler handler) {
        return submitTypedRequest<ConsumeAccountRateLimitResetCreditResponse>(
            protocol,
            detail::ClientRequestTarget::AccountRateLimitResetCreditConsume,
            params,
            std::move(handler),
            detail::encodeConsumeAccountRateLimitResetCreditParams);
    }

    Accounts::Submission Accounts::readRateLimits(Unit params, ReadRateLimitsResultHandler handler) {
        return submitTypedRequest<GetAccountRateLimitsResponse>(
            protocol, detail::ClientRequestTarget::AccountRateLimitsRead, params, std::move(handler), encodeUnitParams);
    }

    Accounts::Submission Accounts::read(GetAccountParams params, ReadResultHandler handler) {
        return submitTypedRequest<GetAccountResponse>(
            protocol, detail::ClientRequestTarget::AccountRead, params, std::move(handler), detail::encodeGetAccountParams);
    }

    Accounts::Submission Accounts::sendAddCreditsNudgeEmail(SendAddCreditsNudgeEmailParams params,
                                                            SendAddCreditsNudgeEmailResultHandler handler) {
        return submitTypedRequest<SendAddCreditsNudgeEmailResponse>(protocol,
                                                                    detail::ClientRequestTarget::AccountSendAddCreditsNudgeEmail,
                                                                    params,
                                                                    std::move(handler),
                                                                    detail::encodeSendAddCreditsNudgeEmailParams);
    }

    Accounts::Submission Accounts::readUsage(Unit params, ReadUsageResultHandler handler) {
        return submitTypedRequest<GetAccountTokenUsageResponse>(
            protocol, detail::ClientRequestTarget::AccountUsageRead, params, std::move(handler), encodeUnitParams);
    }

    Accounts::Submission Accounts::readWorkspaceMessages(Unit params, ReadWorkspaceMessagesResultHandler handler) {
        return submitTypedRequest<GetWorkspaceMessagesResponse>(
            protocol, detail::ClientRequestTarget::AccountWorkspaceMessagesRead, params, std::move(handler), encodeUnitParams);
    }

    Threads::Threads(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    ThreadStartParams toThreadStartParams(ThreadStartOptions options) {
        ThreadStartParams params;
        params.cwd = std::move(options.cwd);
        params.model = std::move(options.model);
        params.modelProvider = std::move(options.modelProvider);
        if (options.approvalPolicy) {
            params.approvalPolicy = AskForApproval{std::move(*options.approvalPolicy)};
        }
        params.sandbox = std::move(options.sandboxMode);
        params.ephemeral = std::move(options.ephemeral);
        return params;
    }

    ThreadResumeParams toThreadResumeParams(ThreadId threadId, ThreadResumeOptions options) {
        ThreadResumeParams params;
        params.threadId = std::move(threadId);
        params.cwd = std::move(options.cwd);
        params.model = std::move(options.model);
        params.modelProvider = std::move(options.modelProvider);
        if (options.approvalPolicy) {
            params.approvalPolicy = AskForApproval{std::move(*options.approvalPolicy)};
        }
        params.sandbox = std::move(options.sandboxMode);
        return params;
    }

    ThreadListParams toThreadListParams(ThreadListOptions options) {
        ThreadListParams params;
        params.cursor = std::move(options.cursor);
        params.limit = std::move(options.limit);
        params.archived = std::move(options.archived);
        params.searchTerm = std::move(options.searchTerm);
        return params;
    }

    ThreadReadParams toThreadReadParams(ThreadId threadId, ThreadReadOptions options) {
        return {std::move(threadId), options.includeTurns};
    }

    Threads::Submission Threads::archive(ThreadArchiveParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadArchive,
            params,
            std::move(handler),
            detail::encodeThreadArchiveParams);
    }

    Threads::Submission Threads::startCompaction(ThreadCompactStartParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadCompactStart,
            params,
            std::move(handler),
            detail::encodeThreadCompactStartParams);
    }

    Threads::Submission Threads::remove(ThreadDeleteParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadDelete,
            params,
            std::move(handler),
            detail::encodeThreadDeleteParams);
    }

    Threads::Submission Threads::fork(ThreadForkParams params, ThreadForkResultHandler handler) {
        return submitTypedRequest<ThreadForkResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadFork,
            params,
            std::move(handler),
            detail::encodeThreadForkParams);
    }

    Threads::Submission Threads::clearGoal(ThreadGoalClearParams params, ThreadGoalClearResultHandler handler) {
        return submitTypedRequest<ThreadGoalClearResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadGoalClear,
            params,
            std::move(handler),
            detail::encodeThreadGoalClearParams);
    }

    Threads::Submission Threads::getGoal(ThreadGoalGetParams params, ThreadGoalGetResultHandler handler) {
        return submitTypedRequest<ThreadGoalGetResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadGoalGet,
            params,
            std::move(handler),
            detail::encodeThreadGoalGetParams);
    }

    Threads::Submission Threads::setGoal(ThreadGoalSetParams params, ThreadGoalSetResultHandler handler) {
        return submitTypedRequest<ThreadGoalSetResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadGoalSet,
            params,
            std::move(handler),
            detail::encodeThreadGoalSetParams);
    }

    Threads::Submission Threads::injectItems(ThreadInjectItemsParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadInjectItems,
            params,
            std::move(handler),
            detail::encodeThreadInjectItemsParams);
    }

    Threads::Submission Threads::submitList(ThreadListParams params, ThreadListResultHandler handler) {
        return submitTypedRequest<ThreadListResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadList,
            params,
            std::move(handler),
            [](const ThreadListParams& value, std::string& error) {
                return detail::encodeThreadListParams(value, error);
            });
    }

    Threads::Submission Threads::listLoaded(ThreadLoadedListParams params, ThreadLoadedListResultHandler handler) {
        return submitTypedRequest<ThreadLoadedListResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadLoadedList,
            params,
            std::move(handler),
            detail::encodeThreadLoadedListParams);
    }

    Threads::Submission Threads::updateMetadata(ThreadMetadataUpdateParams params, ThreadMetadataUpdateResultHandler handler) {
        return submitTypedRequest<ThreadMetadataUpdateResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadMetadataUpdate,
            params,
            std::move(handler),
            detail::encodeThreadMetadataUpdateParams);
    }

    Threads::Submission Threads::setName(ThreadSetNameParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadSetName,
            params,
            std::move(handler),
            detail::encodeThreadSetNameParams);
    }

    Threads::Submission Threads::submitRead(ThreadReadParams params, ThreadReadResultHandler handler) {
        return submitTypedRequest<ThreadReadResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadRead,
            params,
            std::move(handler),
            [](const ThreadReadParams& value, std::string& error) {
                return detail::encodeThreadReadParams(value, error);
            });
    }

    Threads::Submission Threads::resume(ThreadResumeParams params, ThreadResumeResultHandler handler) {
        return submitTypedRequest<ThreadResumeResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadResume,
            params,
            std::move(handler),
            [](const ThreadResumeParams& value, std::string& error) {
                return detail::encodeThreadResumeParams(value, error);
            });
    }

    Threads::Submission Threads::rollback(ThreadRollbackParams params, ThreadRollbackResultHandler handler) {
        return submitTypedRequest<ThreadRollbackResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadRollback,
            params,
            std::move(handler),
            detail::encodeThreadRollbackParams);
    }

    Threads::Submission Threads::shellCommand(ThreadShellCommandParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::ThreadShellCommand,
            params,
            std::move(handler),
            detail::encodeThreadShellCommandParams);
    }

    Threads::Submission Threads::submitStart(ThreadStartParams params, ThreadStartResultHandler handler) {
        return submitTypedRequest<ThreadStartResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadStart,
            params,
            std::move(handler),
            [](const ThreadStartParams& value, std::string& error) {
                return detail::encodeThreadStartParams(value, error);
            });
    }

    Threads::Submission Threads::unarchive(ThreadUnarchiveParams params, ThreadUnarchiveResultHandler handler) {
        return submitTypedRequest<ThreadUnarchiveResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadUnarchive,
            params,
            std::move(handler),
            detail::encodeThreadUnarchiveParams);
    }

    Threads::Submission Threads::unsubscribe(ThreadUnsubscribeParams params, ThreadUnsubscribeResultHandler handler) {
        return submitTypedRequest<ThreadUnsubscribeResponse>(
            protocol,
            detail::ClientRequestTarget::ThreadUnsubscribe,
            params,
            std::move(handler),
            detail::encodeThreadUnsubscribeParams);
    }

    Threads::Submission Threads::start(ThreadStartOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/start requires a result handler");
        }

        return submitStart(toThreadStartParams(std::move(options)),
                           [handler = std::move(handler)](const OperationResult<ThreadStartResponse>& result) {
            handler(mapOperationResult<Thread>(result, [](const ThreadStartResponse& response) {
                return response.thread;
            }));
        });
    }

    Threads::Submission Threads::resume(ThreadId threadId, ThreadResumeOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/resume requires a result handler");
        }

        return resume(toThreadResumeParams(std::move(threadId), std::move(options)),
                      [handler = std::move(handler)](const OperationResult<ThreadResumeResponse>& result) {
            handler(mapOperationResult<Thread>(result, [](const ThreadResumeResponse& response) {
                return response.thread;
            }));
        });
    }

    Threads::Submission Threads::list(ThreadListOptions options, ThreadListResultHandler handler) {
        return submitList(toThreadListParams(std::move(options)), std::move(handler));
    }

    Threads::Submission Threads::read(ThreadId threadId, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/read requires a result handler");
        }
        return submitRead(toThreadReadParams(std::move(threadId)),
                          [handler = std::move(handler)](const OperationResult<ThreadReadResponse>& result) {
            handler(mapOperationResult<Thread>(result, [](const ThreadReadResponse& response) {
                return response.thread;
            }));
        });
    }

    Threads::Submission Threads::read(ThreadId threadId, ThreadReadOptions options, ThreadResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed thread/read requires a result handler");
        }
        return submitRead(toThreadReadParams(std::move(threadId), std::move(options)),
                          [handler = std::move(handler)](const OperationResult<ThreadReadResponse>& result) {
            handler(mapOperationResult<Thread>(result, [](const ThreadReadResponse& response) {
                return response.thread;
            }));
        });
    }

    Turns::Turns(AppServerClient::RawProtocol& protocol) noexcept
        : protocol(&protocol) {
    }

    TurnStartParams toTurnStartParams(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options) {
        TurnStartParams params;
        params.threadId = std::move(threadId);
        params.input = std::move(input);
        params.cwd = std::move(options.cwd);
        params.model = std::move(options.model);
        params.effort = std::move(options.reasoningEffort);
        if (options.approvalPolicy) {
            params.approvalPolicy = AskForApproval{std::move(*options.approvalPolicy)};
        }
        params.sandboxPolicy = std::move(options.sandboxPolicy);
        return params;
    }

    TurnInterruptParams toTurnInterruptParams(ThreadId threadId, TurnId turnId) {
        return {std::move(threadId), std::move(turnId)};
    }

    Turns::Submission Turns::interrupt(TurnInterruptParams params, UnitResultHandler handler) {
        return submitTypedRequest<Unit>(
            protocol,
            detail::ClientRequestTarget::TurnInterrupt,
            params,
            std::move(handler),
            [](const TurnInterruptParams& value, std::string& error) {
                return detail::encodeTurnInterruptParams(value, error);
            });
    }

    Turns::Submission Turns::start(TurnStartParams params, TurnStartResultHandler handler) {
        const ThreadId threadId = params.threadId;
        return submitTypedRequest<TurnStartResponse>(
            protocol,
            detail::ClientRequestTarget::TurnStart,
            params,
            std::move(handler),
            [](const TurnStartParams& value, std::string& error) {
                return detail::encodeTurnStartParams(value, error);
            },
            threadId);
    }

    Turns::Submission Turns::steer(TurnSteerParams params, TurnSteerResultHandler handler) {
        return submitTypedRequest<TurnSteerResponse>(
            protocol,
            detail::ClientRequestTarget::TurnSteer,
            params,
            std::move(handler),
            detail::encodeTurnSteerParams);
    }

    Turns::Submission Turns::start(ThreadId threadId, std::vector<TurnInput> input, TurnStartOptions options, TurnResultHandler handler) {
        if (!handler) {
            return submissionFailure("typed turn/start requires a result handler");
        }

        return start(toTurnStartParams(std::move(threadId), std::move(input), std::move(options)),
                     [handler = std::move(handler)](const OperationResult<TurnStartResponse>& result) {
            handler(mapOperationResult<Turn>(result, [](const TurnStartResponse& response) {
                return response.turn;
            }));
        });
    }

    Turns::Submission Turns::interrupt(ThreadId threadId, TurnId turnId, InterruptResultHandler handler) {
        return interrupt(toTurnInterruptParams(std::move(threadId), std::move(turnId)), std::move(handler));
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

    Requests::SendResult Requests::respondRefresh(const ChatgptAuthTokensRefreshRequest& request,
                                                  ChatgptAuthTokensRefreshResponse response) {
        std::string error;
        std::optional<Json> result = detail::encodeChatgptAuthTokensRefreshResponse(std::move(response), error);
        if (!result) {
            return validationFailure(std::move(error));
        }
        return protocol->respondOwned(request.requestId, request.requestToken, std::move(*result));
    }

    Requests::SendResult Requests::respond(const AuthenticationRequest& request, AuthenticationResponse response) {
        ChatgptAuthTokensRefreshResponse canonical{
            std::move(response.accessToken),
            AccountId{std::move(response.chatgptAccountId)},
            response.chatgptPlanType ? OptionalNullable<PlanType>::withValue(PlanType{std::move(*response.chatgptPlanType)})
                                     : OptionalNullable<PlanType>::explicitNull()};
        return respondRefresh(request, std::move(canonical));
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
