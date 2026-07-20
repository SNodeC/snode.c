/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend-client/JsonLineFramer.h"
#include "support/TestResult.h"

#include <string>
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

    return result.processResult();
}
