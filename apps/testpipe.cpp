#include "EventLoop.h"
#include "Logger.h"
#include "stream/Pipe.h"
#include "stream/PipeSink.h"
#include "stream/PipeSource.h"

#include <functional> // for function
#include <iosfwd>     // for size_t
#include <string>     // for allocator, string

int main(int argc, char* argv[]) {
    net::EventLoop::init(argc, argv);

    net::stream::Pipe pipe(
        [](net::stream::PipeSource& pipeSource, net::stream::PipeSink& pipeSink) -> void {
            pipeSink.setOnData([&pipeSource, &pipeSink](const char* junk, std::size_t junkLen) -> void {
                std::string string(junk, junkLen);
                VLOG(0) << "Pipe Data: " << string;
                pipeSource.send(junk, junkLen);
                pipeSink.disable();
                pipeSource.disable();
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

    return net::EventLoop::start();
}
