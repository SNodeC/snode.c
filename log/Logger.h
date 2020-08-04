#ifndef SNLOGGER_H
#define SNLOGGER_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

class Logger {
public:
    enum Level { INFO, DEBUG, WARNING, ERROR, FATAL };

    virtual ~Logger() = default;

    static void init(int argc, char* argv[]);

    static void setLevel(int level);

    static void logToFile(bool yes);

    static void logToStdOut(bool yes);

protected:
    Level level{INFO};
};

#endif // SNLOGGER_H
