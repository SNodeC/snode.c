#include <SemanticLog.h>
#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"
#include "log/Logger.h"

#include <cstddef>
#include <functional>
#include <string>

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    core::pipe::Pipe pipe(
        []([[maybe_unused]] core::pipe::PipeSource& pipeSource, [[maybe_unused]] core::pipe::PipeSink& pipeSink) -> void {
            pipeSink.setOnData([&pipeSource](const char* junk, std::size_t junkLen) -> void {
                std::string string(junk, junkLen);
                snode::semantic::appLog().trace() << "Pipe Data: " << string;
                pipeSource.send(junk, junkLen);
                // pipeSink.disable();
                // pipeSource.disable();
            });

            pipeSink.setOnEof([]() -> void {
                snode::semantic::appLog().trace() << "Pipe EOF";
            });

            pipeSink.setOnError([]([[maybe_unused]] int errnum) -> void {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "PipeSink";
            });

            pipeSource.setOnError([]([[maybe_unused]] int errnum) -> void {
                snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "PipeSource";
            });

            pipeSource.send("Hello World!");
        },
        []([[maybe_unused]] int errnum) -> void {
            snode::semantic::sysError(snode::semantic::appLog(), logger::LogLevel::Error, errno) << "Pipe not created";
        });

    return core::SNodeC::start();
}
