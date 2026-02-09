/*
 * Express compatibility server for SNode.C/Express
 *
 * Intended to be used alongside the Node.js/Express reference server in this suite.
 */
#include "core/SNodeC.h"
#include "express/legacy/in/WebApp.h"
#include "log/Logger.h"

#include <nlohmann/json.hpp>
#include <string>

// IWYU pragma: no_include <nlohmann/detail/json_ref.hpp>
// IWYU pragma: no_include <nlohmann/json_fwd.hpp>

using express::Router;
using nlohmann::json;

using Trace = std::vector<json>;

static json snapshot(const std::shared_ptr<express::Request>& req, const std::string& label) {
    json out = json::object();
    out["label"] = label;
    out["method"] = req->method;
    out["url"] = req->url;
    out["originalUrl"] = req->originalUrl;
    out["baseUrl"] = req->baseUrl;
    out["path"] = req->path;

    json params = json::object();
    for (const auto& [k, v] : req->params) {
        params[k] = v;
    }
    out["params"] = params;

    json query = json::object();
    for (const auto& [k, v] : req->queries) {
        query[k] = v;
    }
    out["query"] = query;

    json headers = json::object();
    headers["x-test"] = req->get("X-Test");
    out["headers"] = headers;

    return out;
}

static void ensureTrace(const std::shared_ptr<express::Request>& req) {
    // Create the trace attribute if it doesn't exist yet.
    req->setAttribute<Trace>(Trace{}, "trace");
}

static void tracePush(const std::shared_ptr<express::Request>& req, const std::string& label) {
    ensureTrace(req);
    req->getAttribute<Trace>(
        [&](Trace& t) {
            t.emplace_back(snapshot(req, label));
        },
        "trace");
}

static json traceGet(const std::shared_ptr<express::Request>& req) {
    json arr = json::array();
    req->getAttribute<Trace>(
        [&](Trace& t) {
            for (auto const& e : t) {
                arr.push_back(e);
            }
        },
        "trace");
    return arr;
}

int main(int argc, char* argv[]) {
    core::SNodeC::init(argc, argv);

    const express::legacy::in::WebApp app("express-compat");

#ifdef DO
    // Meta
    app.get("/__meta", [] APPLICATION(req, res) {
        res->json({{"ok", true}, {"server", "snodec"}, {"express", true}});
    });

    // Basic
    app.get("/health", [] APPLICATION(req, res) {
        res->json({{"ok", true}, {"label", "health"}});
    });

    // Case-insensitive routing demo
    app.get("/Case/Path", [] APPLICATION(req, res) {
        res->json(snapshot(req, "case"));
    });

    // Trailing slash demo (strictRouting off => matches both)
    app.get("/trail/", [] APPLICATION(req, res) {
        res->json(snapshot(req, "trail"));
    });

    // Wildcards
    app.get("/file/*", [] APPLICATION(req, res) {
        res->json(snapshot(req, "file"));
    });
    app.get("/a/*/b/*", [] APPLICATION(req, res) {
        res->json(snapshot(req, "multi_wild"));
    });

    // Param decoding
    app.get("/p/:x", [] APPLICATION(req, res) {
        res->json(snapshot(req, "param"));
    });

    // Query decoding/casing
    app.get("/query/echo", [] APPLICATION(req, res) {
        res->json(snapshot(req, "query_echo"));
    });

    // HEAD -> GET fallback (handled by dispatcher/response code)
    app.get("/head-demo", [] APPLICATION(req, res) {
        res->set("X-Demo", "1");
        res->send("head body");
    });

    // next('route') demo
    app.get(
        "/nr/:id(\\d+)",
        [] MIDDLEWARE(req, res, next) {
            if (req->params["id"] == "0") {
                next("route");
                return;
            }
            next();
        },
        [] APPLICATION(req, res) {
            res->json(snapshot(req, "nr_primary"));
        });
    app.get("/nr/:id(\\d+)", [] APPLICATION(req, res) {
        res->json(snapshot(req, "nr_fallback"));
    });

    // next('router') demo
    const Router guarded;
    guarded.use([] MIDDLEWARE(req, res, next) {
        if (req->queries["allow"] != "true") {
            next("router");
            return;
        }
        next();
    });
    guarded.get("/stats", [] APPLICATION(req, res) {
        res->json(snapshot(req, "guarded_stats"));
    });
    app.use("/guarded", guarded);

    // Fallback when router skipped
    app.get("/guarded/:rest(.*)", [] APPLICATION(req, res) {
        res->status(403).json({{"label", "guarded_fallback"}, {"status", 403}});
    });

    // Root-mounted router (baseUrl should be empty string)
    const Router root;
    root.get("/root/test", [] APPLICATION(req, res) {
        res->json(snapshot(req, "root_mount"));
    });
    app.use("/", root);

    // Nested routers trace: req.url / baseUrl stripping
    const Router api;
    api.use([] MIDDLEWARE(req, res, next) {
        tracePush(req, "api.use");
        next();
    });

    const Router v1;
    v1.use([] MIDDLEWARE(req, res, next) {
        tracePush(req, "v1.use");
        next();
    });
    v1.get("/users/:id", [] APPLICATION(req, res) {
        tracePush(req, "handler");
        res->json({{"label", "nested_trace"}, {"trace", traceGet(req)}});
    });

    api.use("/v1", v1);
    app.use("/api", api);

    // mergeParams demo (requires your mergeParams patch-set)
    const Router mpMerge;
    mpMerge.setMergeParams(true);
    mpMerge.get("/users/:id", [] APPLICATION(req, res) {
        res->json(snapshot(req, "mp_merge"));
    });
    app.use("/mp/merge/t/:tenant", mpMerge);

    const Router mpNoMerge;
    mpNoMerge.get("/users/:id", [] APPLICATION(req, res) {
        res->json(snapshot(req, "mp_nomerge"));
    });
    app.use("/mp/nomerge/t/:tenant", mpNoMerge);

#endif

    // Params scoping trace: merge vs no-merge
    auto makeScope = [](bool merge) {
        const Router parent;
        parent.use([merge](auto const& req, auto const&, auto& next) {
            tracePush(req, merge ? "scopeMerge.parent" : "scopeNoMerge.parent");
            VLOG(0) << "############# 1";
            next();
        });

        const Router child;
        child.setMergeParams(merge);
        child.use([merge](auto const& req, auto const&, auto& next) {
            tracePush(req, merge ? "scopeMerge.child" : "scopeNoMerge.child");
            VLOG(0) << "############# 2";
            next();
        });
        child.get("/end", [merge](auto const& req, auto const& res) {
            tracePush(req, merge ? "scopeMerge.handler" : "scopeNoMerge.handler");
            res->json({{"label", merge ? "scope_merge" : "scope_nomerge"}, {"trace", traceGet(req)}});
            VLOG(0) << "############# 3";
        });

        parent.use("/b/:b", child);
        return parent;
    };

    app.use("/scope/nomerge/:a", makeScope(false));
    app.use("/scope/merge/:a", makeScope(true));

    // Param decoding (valid)
    app.get("/decode/:p", [] APPLICATION(req, res) {
        res->json(snapshot(req, "decode_ok"));
    });

    // 404 (GET only, enough for this suite)
    app.get("/:rest(.*)", [] APPLICATION(req, res) {
        res->status(404).json({{"label", "not_found"}, {"path", req->path}});
    });

    app.listen(8080, [](const express::legacy::in::WebApp::SocketAddress& socketAddress, const core::socket::State& state) {
        switch (state) {
            case core::socket::State::OK:
                VLOG(1) << "express-compat listening on '" << socketAddress.toString() << "'";
                break;
            case core::socket::State::DISABLED:
                VLOG(1) << "express-compat disabled";
                break;
            case core::socket::State::ERROR:
                LOG(ERROR) << "express-compat " << socketAddress.toString() << ": " << state.what();
                break;
            case core::socket::State::FATAL:
                LOG(FATAL) << "express-compat " << socketAddress.toString() << ": " << state.what();
                break;
        }
    });

    return core::SNodeC::start();
}
