#ifndef SNODEC_LOG_H
#define SNODEC_LOG_H

#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace snode::log {

    enum class Level { Trace, Debug, Info, Warning, Error, Critical, Off };
    enum class Origin { Framework, Application };
    enum class Boundary { Application, Configuration, Instance, Connection, Context, System };
    enum class Role { Server, Client };
    enum class Format { Text, Json };
    enum class ColorMode { Automatic, Always, Never };

    struct Identity {
        std::optional<std::string> instance = std::nullopt;
        std::optional<Role> role = std::nullopt;
        std::optional<std::string> connection = std::nullopt;
    };

    struct Scope {
        Origin origin = Origin::Application;
        Boundary boundary = Boundary::Application;
        std::string component = "app";
        Identity identity;

    };

    struct LevelOverride {
        std::string name;
        Level level = Level::Info;
    };

    struct OriginLevelOverride {
        Origin origin = Origin::Application;
        Level level = Level::Info;
    };

    struct BoundaryLevelOverride {
        Boundary boundary = Boundary::Application;
        Level level = Level::Info;
    };

    struct Message {
        std::string plain;
        std::optional<std::string> terminal;
    };

    struct Settings {
        Level level = Level::Info;
        Format format = Format::Text;
        ColorMode color = ColorMode::Automatic;
        bool quiet = false;
        std::optional<std::string> file;
        std::vector<OriginLevelOverride> originLevels;
        std::vector<BoundaryLevelOverride> boundaryLevels;
        std::vector<LevelOverride> componentLevels;
        std::vector<LevelOverride> instanceLevels;
    };

    void configure(const Settings& settings);

    namespace detail {
        template <class T>
        std::string stringify(T&& value) {
            std::ostringstream out;
            out << std::forward<T>(value);
            return out.str();
        }

        template <class... Args>
        std::string format(std::string_view pattern, Args&&... args) {
            std::vector<std::string> values;
            values.reserve(sizeof...(Args));
            (values.emplace_back(stringify(std::forward<Args>(args))), ...);

            std::string result;
            result.reserve(pattern.size());
            std::size_t argument = 0;
            for (std::size_t index = 0; index < pattern.size(); ++index) {
                if (pattern[index] == '{' && index + 1 < pattern.size()) {
                    if (pattern[index + 1] == '{') {
                        result.push_back('{');
                        ++index;
                        continue;
                    }
                    if (pattern[index + 1] == '}') {
                        if (argument >= values.size()) {
                            throw std::invalid_argument("log format has too few arguments");
                        }
                        result += values[argument++];
                        ++index;
                        continue;
                    }
                }
                if (pattern[index] == '}') {
                    if (index + 1 < pattern.size() && pattern[index + 1] == '}') {
                        result.push_back('}');
                        ++index;
                        continue;
                    }
                    throw std::invalid_argument("log format contains an unmatched '}'");
                }
                if (pattern[index] == '{') {
                    throw std::invalid_argument("log format contains an unmatched '{'");
                }
                result.push_back(pattern[index]);
            }
            if (argument != values.size()) {
                throw std::invalid_argument("log format has too many arguments");
            }
            return result;
        }
    } // namespace detail

    class Logger;

    class Stream {
    public:
        Stream(Stream&&) noexcept;
        Stream& operator=(Stream&&) noexcept;
        Stream(const Stream&) = delete;
        Stream& operator=(const Stream&) = delete;
        ~Stream();

        template <class T>
        Stream& operator<<(T&& value) {
            if (state && state->enabled) {
                state->buffer << std::forward<T>(value);
            }
            return *this;
        }

    private:
        struct State {
            bool enabled;
            bool emitted = false;
            std::ostringstream buffer;
            std::function<void(std::string)> emit;
        };

        explicit Stream(std::unique_ptr<State> state);
        void flush();

        std::unique_ptr<State> state;

        friend class Logger;
    };

    class Logger {
    public:
        Logger(const Logger&) noexcept = default;
        Logger& operator=(const Logger&) noexcept = default;
        Logger(Logger&&) noexcept = default;
        Logger& operator=(Logger&&) noexcept = default;
        ~Logger() = default;

        bool enabled(Level level) const noexcept;
        void emit(Level level, Message message) const;

        Stream trace() const;
        Stream debug() const;
        Stream info() const;
        Stream warn() const;
        Stream error() const;
        Stream critical() const;
        Stream systemError(Level level, std::error_code error) const;
        Stream systemError(Level level, int errorNumber) const;

        template <class... Args>
        void trace(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Trace)) {
                write(Level::Trace, detail::format(pattern, std::forward<Args>(args)...));
            }
        }
        template <class... Args>
        void debug(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Debug)) {
                write(Level::Debug, detail::format(pattern, std::forward<Args>(args)...));
            }
        }
        template <class... Args>
        void info(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Info)) {
                write(Level::Info, detail::format(pattern, std::forward<Args>(args)...));
            }
        }
        template <class... Args>
        void warn(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Warning)) {
                write(Level::Warning, detail::format(pattern, std::forward<Args>(args)...));
            }
        }
        template <class... Args>
        void error(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Error)) {
                write(Level::Error, detail::format(pattern, std::forward<Args>(args)...));
            }
        }
        template <class... Args>
        void critical(std::string_view pattern, Args&&... args) const {
            if (enabled(Level::Critical)) {
                write(Level::Critical, detail::format(pattern, std::forward<Args>(args)...));
            }
        }

        template <class... Args>
        void event(Level level, std::string eventName, std::string_view pattern, Args&&... args) const {
            if (enabled(level)) {
                writeEvent(level, std::move(eventName), detail::format(pattern, std::forward<Args>(args)...));
            }
        }

        template <class... Args>
        void systemError(Level level, std::error_code error, std::string_view pattern, Args&&... args) const {
            if (enabled(level)) {
                writeSystemError(level, std::move(error), detail::format(pattern, std::forward<Args>(args)...));
            }
        }

        template <class... Args>
        void systemError(Level level, int errorNumber, std::string_view pattern, Args&&... args) const {
            systemError(level,
                        std::error_code(errorNumber, std::generic_category()),
                        pattern,
                        std::forward<Args>(args)...);
        }

    private:
        class Impl;

        explicit Logger(std::shared_ptr<const Impl> impl);
        Stream stream(Level level) const;
        void write(Level level, std::string message) const;
        void writeEvent(Level level, std::string eventName, std::string message) const;
        void writeSystemError(Level level, std::error_code error, std::string message) const;

        std::shared_ptr<const Impl> impl;

        friend Logger makeLogger(Scope scope);
    };

    Logger makeLogger(Scope scope);
    Logger application(std::string component = "app", Identity identity = {});
    Logger framework(std::string component = "framework", Boundary boundary = Boundary::System, Identity identity = {});

    template <class Connection>
    Logger forConnection(const Connection& connection,
                         std::string component = "app",
                         Origin origin = Origin::Application,
                         Boundary boundary = Boundary::Connection,
                         std::optional<Role> role = std::nullopt) {
        Identity identity;
        if (!connection.getInstanceName().empty()) {
            identity.instance = connection.getInstanceName();
        }
        identity.role = role;
        identity.connection = std::to_string(connection.getConnectionId());
        return makeLogger({origin, boundary, std::move(component), std::move(identity)});
    }

} // namespace snode::log

#endif // SNODEC_LOG_H
