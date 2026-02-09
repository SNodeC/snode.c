/* Express compatibility reference server (Node.js/Express v4)
 * Mirrors the behavior expected from SNode.C/Express after the compatibility patch sets.
 */
const express = require("express");

const app = express();
const PORT = Number(process.env.PORT || 3000);

function snapshot(req, label, extra = {}) {
  return Object.assign(
    {
      label,
      method: req.method,
      url: req.url,
      originalUrl: req.originalUrl,
      baseUrl: req.baseUrl,
      path: req.path,
      params: req.params,
      query: req.query,
      headers: {
        "x-test": req.get("x-test") || ""
      }
    },
    extra
  );
}

function tracePush(req, label) {
  if (!req._trace) req._trace = [];
  req._trace.push(snapshot(req, label));
}

// Meta
app.get("/__meta", (req, res) => {
  res.json({ ok: true, server: "node", express: true });
});

// Basic
app.get("/health", (req, res) => res.json({ ok: true, label: "health" }));

// Case-insensitive routing demo (route is cased, request can be different)
app.get("/Case/Path", (req, res) => res.json(snapshot(req, "case")));

// Trailing slash demo: strict routing off => "/trail/" matches both "/trail" and "/trail/"
app.get("/trail/", (req, res) => res.json(snapshot(req, "trail")));

// Wildcards
app.get("/file/*", (req, res) => res.json(snapshot(req, "file")));
app.get("/a/*/b/*", (req, res) => res.json(snapshot(req, "multi_wild")));

// Param decoding
app.get("/p/:x", (req, res) => res.json(snapshot(req, "param")));

// Query decoding/casing
app.get("/query/echo", (req, res) => res.json(snapshot(req, "query_echo")));

// HEAD -> GET fallback, and no body on HEAD
app.get("/head-demo", (req, res) => {
  res.set("X-Demo", "1");
  res.send("head body");
});

// next('route') demo
app.get(
  "/nr/:id(\\d+)",
  (req, res, next) => {
    if (req.params.id === "0") return next("route");
    next();
  },
  (req, res) => res.json(snapshot(req, "nr_primary"))
);
app.get("/nr/:id(\\d+)", (req, res) => res.json(snapshot(req, "nr_fallback")));

// next('router') demo
const guarded = express.Router();
guarded.use((req, res, next) => {
  if (req.query.allow !== "true") return next("router");
  next();
});
guarded.get("/stats", (req, res) => res.json(snapshot(req, "guarded_stats")));
app.use("/guarded", guarded);
app.get("/guarded/*", (req, res) => res.status(403).json({ label: "guarded_fallback", status: 403 }));

// Root-mounted router (baseUrl should be empty string)
const root = express.Router();
root.get("/root/test", (req, res) => res.json(snapshot(req, "root_mount")));
app.use("/", root);

// Nested routers: req.url / baseUrl stripping trace
const api = express.Router();
api.use((req, res, next) => {
  tracePush(req, "api.use");
  next();
});
const v1 = express.Router();
v1.use((req, res, next) => {
  tracePush(req, "v1.use");
  next();
});
v1.get("/users/:id", (req, res) => {
  tracePush(req, "handler");
  res.json({ label: "nested_trace", trace: req._trace || [] });
});
api.use("/v1", v1);
app.use("/api", api);

// mergeParams demo
const mpMerge = express.Router({ mergeParams: true });
mpMerge.get("/users/:id", (req, res) => res.json(snapshot(req, "mp_merge")));
app.use("/mp/merge/t/:tenant", mpMerge);

const mpNoMerge = express.Router();
mpNoMerge.get("/users/:id", (req, res) => res.json(snapshot(req, "mp_nomerge")));
app.use("/mp/nomerge/t/:tenant", mpNoMerge);

// Params scoping trace: merge vs no-merge
function makeScopeRouter(merge) {
  const parent = express.Router({ mergeParams: true });
  parent.use((req, res, next) => {
    tracePush(req, merge ? "scopeMerge.parent" : "scopeNoMerge.parent");
    next();
  });

  const child = express.Router({ mergeParams: merge });
  child.use((req, res, next) => {
    tracePush(req, merge ? "scopeMerge.child" : "scopeNoMerge.child");
    next();
  });
  child.get("/end", (req, res) => {
    tracePush(req, merge ? "scopeMerge.handler" : "scopeNoMerge.handler");
    res.json({ label: merge ? "scope_merge" : "scope_nomerge", trace: req._trace || [] });
  });

  parent.use("/b/:b", child);
  return parent;
}
app.use("/scope/nomerge/:a", makeScopeRouter(false));
app.use("/scope/merge/:a", makeScopeRouter(true));

// Param decoding error behavior (invalid percent escapes) => 400 (Express does this)
app.get("/decode/:p", (req, res) => res.json(snapshot(req, "decode_ok")));

// 404
app.use((req, res) => res.status(404).json({ label: "not_found", path: req.path }));

// Error handler (URIError etc.)
app.use((err, req, res, next) => {
  res.status(400).json({ label: "error", error: String(err && err.message ? err.message : err) });
});

app.listen(PORT, () => {
  console.log(`Node Express compat server listening on http://127.0.0.1:${PORT}`);
});
