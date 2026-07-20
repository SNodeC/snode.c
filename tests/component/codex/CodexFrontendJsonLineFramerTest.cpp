/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
 */

#include "apps/codex-backend/JsonLineFramer.h"
#include "support/TestResult.h"

#include <string>
#include <utility>
#include <vector>

int main() {
    using apps::codex_backend::JsonLineFramer;

    tests::support::TestResult result;
    std::vector<std::string> frames;
    const auto collect = [&frames](std::string frame) {
        frames.push_back(std::move(frame));
    };

    JsonLineFramer framer(64);
    result.expectTrue(framer.push("{\"kind\":\"hel", collect) == JsonLineFramer::Result::Accepted, "a fragmented frame prefix is accepted");
    result.expectTrue(frames.empty(), "a fragmented frame is not emitted early");
    result.expectTrue(framer.push("lo\"}\n{\"a\":1}\n{\"b\":", collect) == JsonLineFramer::Result::Accepted,
                      "coalesced frames and a trailing fragment are accepted");
    result.expectTrue(frames.size() == 2, "two complete coalesced frames are emitted");
    result.expectTrue(frames[0] == "{\"kind\":\"hello\"}", "the fragmented hello is reconstructed exactly");
    result.expectTrue(frames[1] == "{\"a\":1}", "the second coalesced frame is reconstructed exactly");
    result.expectTrue(framer.bufferedSize() == 5, "only the unterminated tail remains buffered");

    result.expectTrue(framer.push("\"line\\ninside\"}\r\n", collect) == JsonLineFramer::Result::Accepted,
                      "an escaped newline and CRLF terminator are accepted");
    result.expectTrue(frames.size() == 3, "the completed trailing frame is emitted");
    result.expectTrue(frames[2] == "{\"b\":\"line\\ninside\"}", "escaped JSON newlines do not split a frame");

    JsonLineFramer bounded(8);
    result.expectTrue(bounded.push("12345678", collect) == JsonLineFramer::Result::Accepted,
                      "a frame exactly at the configured bound is accepted while unterminated");
    result.expectTrue(bounded.bufferedSize() == 8, "the receive buffer never exceeds its configured bound");
    result.expectTrue(bounded.push("9", collect) == JsonLineFramer::Result::FrameTooLarge,
                      "the first byte beyond the configured bound rejects the frame");
    result.expectTrue(bounded.bufferedSize() == 0, "an oversized frame releases all accumulated storage");

    return result.processResult();
}
