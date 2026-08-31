#include "Log.h"

#include "log/LogScopeOwner.h"
#include "log/Logger.h"
#include "log/detail/Native.h"

#include <utility>

namespace snode::log {
    namespace {
        logger::LogLevel nativeLevel(const Level level) noexcept {
            switch (level) {
                case Level::Trace:
                    return logger::LogLevel::Trace;
                case Level::Debug:
                    return logger::LogLevel::Debug;
                case Level::Info:
                    return logger::LogLevel::Info;
                case Level::Warning:
                    return logger::LogLevel::Warn;
                case Level::Error:
                    return logger::LogLevel::Error;
                case Level::Critical:
                    return logger::LogLevel::Critical;
                case Level::Off:
                    return logger::LogLevel::Off;
            }
            return logger::LogLevel::Off;
        }

        logger::LogOrigin nativeOrigin(const Origin origin) noexcept {
            return origin == Origin::Framework ? logger::LogOrigin::Framework : logger::LogOrigin::Application;
        }

        logger::LogBoundary nativeBoundary(const Boundary boundary) noexcept {
            switch (boundary) {
                case Boundary::Application:
                    return logger::LogBoundary::Application;
                case Boundary::Configuration:
                    return logger::LogBoundary::Configuration;
                case Boundary::Instance:
                    return logger::LogBoundary::Instance;
                case Boundary::Connection:
                    return logger::LogBoundary::Connection;
                case Boundary::Context:
                    return logger::LogBoundary::Context;
                case Boundary::System:
                    return logger::LogBoundary::System;
            }
            return logger::LogBoundary::System;
        }

        logger::LogRole nativeRole(const std::optional<Role>& role) noexcept {
            if (!role) {
                return logger::LogRole::Unknown;
            }
            return *role == Role::Server ? logger::LogRole::Server : logger::LogRole::Client;
        }
    } // namespace

    logger::LogScope detail::nativeScope(const Scope& scope) noexcept {
        return {nativeOrigin(scope.origin),
                nativeBoundary(scope.boundary),
                scope.component,
                scope.identity.instance ? std::string_view(*scope.identity.instance) : std::string_view(),
                nativeRole(scope.identity.role),
                scope.identity.connection ? std::string_view(*scope.identity.connection) : std::string_view()};
    }

    class Logger::Impl {
    public:
        explicit Impl(logger::BoundaryLogger logger)
            : logger(std::move(logger)) {
        }

        logger::BoundaryLogger logger;
    };

    Stream::Stream(std::unique_ptr<State> state)
        : state(std::move(state)) {
    }

    Stream::Stream(Stream&& other) noexcept = default;

    Stream& Stream::operator=(Stream&& other) noexcept {
        if (this != &other) {
            flush();
            state = std::move(other.state);
        }
        return *this;
    }

    Stream::~Stream() {
        flush();
    }

    void Stream::flush() {
        if (state && !state->emitted) {
            state->emitted = true;
            if (state->enabled) {
                state->emit(state->buffer.str());
            }
        }
    }

    Logger::Logger(std::shared_ptr<const Impl> impl)
        : impl(std::move(impl)) {
    }

    bool Logger::enabled(const Level level) const noexcept {
        return impl && impl->logger.enabled(nativeLevel(level));
    }

    void Logger::emit(const Level level, Message message) const {
        if (!enabled(level)) {
            return;
        }
        impl->logger.emit(nativeLevel(level),
                          logger::PresentedMessage{.plain = std::move(message.plain),
                                                   .terminal = message.terminal ? std::move(*message.terminal) : std::string()});
    }

    Stream Logger::stream(const Level level) const {
        const bool isEnabled = enabled(level);
        auto state = std::make_unique<Stream::State>();
        state->enabled = isEnabled;
        if (isEnabled) {
            const auto implementation = impl;
            state->emit = [implementation, level](std::string message) {
                implementation->logger.emit(nativeLevel(level), std::move(message));
            };
        }
        return Stream(std::move(state));
    }

    Stream Logger::trace() const {
        return stream(Level::Trace);
    }
    Stream Logger::debug() const {
        return stream(Level::Debug);
    }
    Stream Logger::info() const {
        return stream(Level::Info);
    }
    Stream Logger::warn() const {
        return stream(Level::Warning);
    }
    Stream Logger::error() const {
        return stream(Level::Error);
    }
    Stream Logger::critical() const {
        return stream(Level::Critical);
    }

    Stream Logger::systemError(const Level level, std::error_code error) const {
        const bool isEnabled = enabled(level);
        auto state = std::make_unique<Stream::State>();
        state->enabled = isEnabled;
        if (isEnabled) {
            const auto implementation = impl;
            state->emit = [implementation, level, error = std::move(error)](std::string message) {
                implementation->logger.sysError(nativeLevel(level), error, "{}", message);
            };
        }
        return Stream(std::move(state));
    }

    Stream Logger::systemError(const Level level, const int errorNumber) const {
        return systemError(level, std::error_code(errorNumber, std::generic_category()));
    }

    void Logger::write(const Level level, std::string message) const {
        if (enabled(level)) {
            impl->logger.emit(nativeLevel(level), std::move(message));
        }
    }

    void Logger::writeEvent(const Level level, std::string eventName, std::string message) const {
        if (!enabled(level)) {
            return;
        }
        logger::LogRecordOptions options;
        options.event = std::move(eventName);
        impl->logger.emit(nativeLevel(level), std::move(message), std::move(options));
    }

    void Logger::writeSystemError(const Level level, std::error_code error, std::string message) const {
        if (enabled(level)) {
            impl->logger.sysError(nativeLevel(level), std::move(error), "{}", message);
        }
    }

    Logger makeLogger(Scope scope) {
        auto owner = logger::LogScopeOwner::fromScope(detail::nativeScope(scope));
        return Logger(std::make_shared<const Logger::Impl>(owner.logger(logger::Logger::semanticSink())));
    }

    Logger application(std::string component, Identity identity) {
        return makeLogger({Origin::Application, Boundary::Application, std::move(component), std::move(identity)});
    }

    Logger framework(std::string component, const Boundary boundary, Identity identity) {
        return makeLogger({Origin::Framework, boundary, std::move(component), std::move(identity)});
    }

    void configure(const Settings& settings) {
        logger::Logger::init();
        logger::LogManager::init();
        logger::LogManager::setGlobalLevel(nativeLevel(settings.level));
        logger::LogManager::setFormat(settings.format == Format::Json ? logger::LogManager::Format::Json : logger::LogManager::Format::Text);
        for (const auto& rule : settings.originLevels) {
            logger::LogManager::setOriginLevel(nativeOrigin(rule.origin), nativeLevel(rule.level));
        }
        for (const auto& rule : settings.boundaryLevels) {
            logger::LogManager::setBoundaryLevel(nativeBoundary(rule.boundary), nativeLevel(rule.level));
        }
        for (const auto& rule : settings.componentLevels) {
            logger::LogManager::setComponentLevel(rule.name, nativeLevel(rule.level));
        }
        for (const auto& rule : settings.instanceLevels) {
            logger::LogManager::setInstanceLevel(rule.name, nativeLevel(rule.level));
        }
        logger::Logger::setQuiet(settings.quiet);
        if (settings.color == ColorMode::Always) {
            logger::Logger::setDisableColor(false);
        } else if (settings.color == ColorMode::Never) {
            logger::Logger::setDisableColor(true);
        }
        if (settings.file && !settings.file->empty()) {
            logger::Logger::logToFile(*settings.file);
        } else {
            logger::Logger::disableLogToFile();
        }
        logger::LogManager::freeze();
    }

} // namespace snode::log
