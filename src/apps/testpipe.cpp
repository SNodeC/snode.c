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
        []([[maybe_unused]] core::pipe::PipeSource& pipeSource, [[maybe_unused]] core::pipe::PipeSink& pipeSink) {
            pipeSink.setOnData([&pipeSource](const char* chunk, std::size_t chunkLen) {
                const std::string string(chunk, chunkLen);
                VLOG(1) << "Pipe Data: " << string;
                pipeSource.send(chunk, chunkLen);
                // pipeSink.disable();
                // pipeSource.disable();
            });

            pipeSink.setOnEof([]() {
                VLOG(1) << "Pipe EOF";
            });

            pipeSink.setOnError([]([[maybe_unused]] int errnum) {
                VLOG(1) << "PipeSink";
            });

            pipeSource.setOnError([]([[maybe_unused]] int errnum) {
                VLOG(1) << "PipeSource";
            });

            pipeSource.send("Hello World!");
        },
        []([[maybe_unused]] int errnum) {
            PLOG(ERROR) << "Pipe not created";
        });

    return core::SNodeC::start();
}
