#ifndef SNLOGGER_H
#define SNLOGGER_H

#include <memory>
#include <string_view>

class LoggingFacility {
public:
    virtual ~LoggingFacility() = default;
    virtual void writeInfoEntry(std::string_view entry) = 0;
    virtual void writeWarnEntry(std::string_view entry) = 0;
    virtual void writeErrorEntry(std::string_view entry) = 0;
};

using Logger = std::shared_ptr<LoggingFacility>;

extern Logger logger;

#endif // SNLOGGER_H
