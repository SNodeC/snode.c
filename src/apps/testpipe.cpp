#include "core/SNodeC.h"
#include "core/pipe/Pipe.h"
#include "core/pipe/PipeSink.h"
#include "core/pipe/PipeSource.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS

#include "log/Logger.h"

#include <cstddef>
#include <functional>
#include <string>

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const core::pipe::Pipe pipe(
        []([[maybe_unused]] core::pipe::PipeSource& pipeSource, [[maybe_unused]] core::pipe::PipeSink& pipeSink) -> void {
            pipeSink.setOnData([&pipeSource](const char* junk, std::size_t junkLen) -> void {
                const std::string string(junk, junkLen);
                VLOG(0) << "Pipe Data: " << string;
                pipeSource.send(junk, junkLen);
                // pipeSink.disable();
                // pipeSource.disable();
            });

            pipeSink.setOnEof([]() -> void {
                VLOG(0) << "Pipe EOF";
            });

            pipeSink.setOnError([]([[maybe_unused]] int errnum) -> void {
                VLOG(0) << "PipeSink";
            });

            pipeSource.setOnError([]([[maybe_unused]] int errnum) -> void {
                VLOG(0) << "PipeSource";
            });

            pipeSource.send("Hello World!");
        },
        []([[maybe_unused]] int errnum) -> void {
            PLOG(ERROR) << "Pipe not created";
        });

    return core::SNodeC::start();
}
