/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/JsonLineFramer.h"
#include "support/TestResult.h"

#include <string>
#include <string_view>
#include <utility>
#include <vector>

int main() {
    using apps::codex_backend_client::JsonLineFramer;

    tests::support::TestResult result;
    std::vector<std::string> frames;
    const auto collect = [&frames](std::string frame) {
        frames.push_back(std::move(frame));
    };

    JsonLineFramer framer(128);
    result.expectTrue(framer.push("{\"kind\":\"wel", collect) == JsonLineFramer::Result::Accepted,
                      "the client framer accepts a fragmented server frame prefix");
    result.expectTrue(frames.empty(), "the client framer does not emit an incomplete server frame");

    result.expectTrue(framer.push("come\"}\n{\"kind\":\"snapshot\"}\n{\"kind\":\"sync.complete\"", collect) ==
                          JsonLineFramer::Result::Accepted,
                      "the client framer accepts a completed fragment, a coalesced frame, and a trailing fragment");
    result.expectEqual(2, static_cast<int>(frames.size()), "two complete server frames are emitted from one coalesced read");
    result.expectTrue(frames.size() >= 2 && frames[0] == "{\"kind\":\"welcome\"}", "the fragmented welcome frame is reconstructed exactly");
    result.expectTrue(frames.size() >= 2 && frames[1] == "{\"kind\":\"snapshot\"}", "the coalesced snapshot frame is emitted exactly");
    result.expectTrue(framer.bufferedSize() == std::string("{\"kind\":\"sync.complete\"").size(),
                      "only the unterminated sync frame remains buffered");

    result.expectTrue(framer.push("}\r\n", collect) == JsonLineFramer::Result::Accepted,
                      "the client framer accepts a CRLF-terminated continuation");
    result.expectEqual(3, static_cast<int>(frames.size()), "the completed sync frame is emitted exactly once");
    result.expectTrue(frames.size() >= 3 && frames[2] == "{\"kind\":\"sync.complete\"}",
                      "the client framer strips only the framing carriage return");

    JsonLineFramer bounded(8);
    result.expectTrue(bounded.push("12345678", collect) == JsonLineFramer::Result::Accepted,
                      "a client frame exactly at the configured bound remains accepted");
    result.expectTrue(bounded.bufferedSize() == 8, "the client receive buffer reaches but does not exceed its bound");
    result.expectTrue(bounded.push("9", collect) == JsonLineFramer::Result::FrameTooLarge,
                      "the first byte beyond the configured client frame bound is rejected");
    result.expectTrue(bounded.bufferedSize() == 0, "an oversized client frame releases its accumulated storage");

    constexpr std::size_t ChunkSize = 16 * 1024;
    const std::string snapshotFrame =
        "{\"kind\":\"snapshot\",\"sequence\":42,\"state\":{\"padding\":\"" + std::string(4 * ChunkSize + 257, 'x') + "\"}}";
    const std::string syncPreamble = "{\"kind\":\"";
    const std::string syncKind = "sync.complete";
    const std::string syncSuffix = "\",\"sequence\":42}";
    const std::string syncFrame = syncPreamble + syncKind + syncSuffix;
    std::vector<std::string> largeFrames;
    const auto collectLarge = [&largeFrames](std::string frame) {
        largeFrames.push_back(std::move(frame));
    };

    JsonLineFramer largeFramer(snapshotFrame.size());
    result.expectTrue(snapshotFrame.size() > 64 * 1024, "the snapshot-like frame exceeds four client receive chunks");
    std::size_t offset = 0;
    while (snapshotFrame.size() - offset >= ChunkSize) {
        result.expectTrue(largeFramer.push(std::string_view(snapshotFrame).substr(offset, ChunkSize), collectLarge) ==
                              JsonLineFramer::Result::Accepted,
                          "the client framer accepts an exact 16 KiB snapshot fragment");
        offset += ChunkSize;
    }
    result.expectTrue(largeFramer.push(std::string_view(snapshotFrame).substr(offset), collectLarge) == JsonLineFramer::Result::Accepted,
                      "the client framer accepts the final unterminated snapshot fragment");
    result.expectTrue(largeFrames.empty() && largeFramer.bufferedSize() == snapshotFrame.size(),
                      "the complete large snapshot remains buffered until its newline arrives");

    result.expectTrue(largeFramer.push("\n" + syncPreamble, collectLarge) == JsonLineFramer::Result::Accepted,
                      "one coalesced push terminates the snapshot and stops immediately before the sync.complete kind");
    result.expectTrue(largeFrames == std::vector<std::string>{snapshotFrame} && largeFramer.bufferedSize() == syncPreamble.size(),
                      "the newline emits the large snapshot exactly once and retains only the next frame preamble");
    result.expectTrue(largeFramer.push(syncKind, collectLarge) == JsonLineFramer::Result::Accepted,
                      "the next fragment stops immediately after the sync.complete kind");
    result.expectTrue(largeFrames.size() == 1 && largeFramer.bufferedSize() == syncPreamble.size() + syncKind.size(),
                      "the unterminated sync.complete frame is retained without another emission");
    result.expectTrue(largeFramer.push(syncSuffix + "\n", collectLarge) == JsonLineFramer::Result::Accepted,
                      "the trailing sync.complete suffix and newline finish the coalesced handshake");
    result.expectTrue(largeFrames == std::vector<std::string>{snapshotFrame, syncFrame},
                      "the large snapshot and trailing sync.complete frames are emitted exactly once and in order");
    result.expectTrue(largeFramer.bufferedSize() == 0, "no protocol bytes remain buffered after the trailing sync.complete newline");

    return result.processResult();
}
