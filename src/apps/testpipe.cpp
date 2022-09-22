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
                VLOG(0) << "Pipe Data: " << string;
                pipeSource.send(junk, junkLen);
                // pipeSink.disable();
                // pipeSource.disable();
            });

            pipeSink.setOnEof([]() -> void {
                VLOG(0) << "Pipe EOF";
            });

            pipeSink.setOnError([]([[maybe_unused]] int errnum) -> void {
                PLOG(ERROR) << "PipeSink";
            });

            pipeSource.setOnError([]([[maybe_unused]] int errnum) -> void {
                PLOG(ERROR) << "PipeSource";
            });

            pipeSource.send("Hello World!");
        },
        []([[maybe_unused]] int errnum) -> void {
            PLOG(ERROR) << "Pipe not created";
        });

    return core::SNodeC::start();
}
