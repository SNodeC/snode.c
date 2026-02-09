#!/usr/bin/env python3

"""
Express compatibility suite runner.

Features:
- Validate one server against expected values (--expect-only)
- Compare Node vs SNode responses (default mode)
- Dump full JSON/text responses for failing cases (--dump-on-fail)
- Run only a single case by id (--only <id>)

Usage examples:
  python3 tests/run_suite.py --cases tests/cases.json --node http://127.0.0.1:3000 --snode http://127.0.0.1:8080
  python3 tests/run_suite.py --cases tests/cases.json --expect-only --snode http://127.0.0.1:8080
  python3 tests/run_suite.py --cases tests/cases.json --node http://127.0.0.1:3000 --snode http://127.0.0.1:8080 --dump-on-fail
  python3 tests/run_suite.py --cases tests/cases.json --node http://127.0.0.1:3000 --snode http://127.0.0.1:8080 --only merge_params_disabled --dump-on-fail
"""
from __future__ import annotations

import argparse
import http.client
import json
import sys
import urllib.parse
from typing import Any, Dict, List, Optional, Tuple


def _lower_headers(h: Dict[str, str]) -> Dict[str, str]:
    return {k.lower(): v for (k, v) in h.items()}


def _http_request(base_url: str, method: str, path: str, headers: Optional[Dict[str, str]] = None) -> Tuple[int, Dict[str, str], bytes]:
    """
    Perform a single HTTP request and return (status, headers, body).

    Note: For HEAD we still read the socket response but the caller may choose to ignore the body.
    """
    url = urllib.parse.urlparse(base_url)
    if url.scheme not in ("http", "https"):
        raise ValueError(f"Unsupported scheme in base_url: {base_url!r}")

    conn_cls = http.client.HTTPSConnection if url.scheme == "https" else http.client.HTTPConnection
    port = url.port or (443 if url.scheme == "https" else 80)
    conn = conn_cls(url.hostname, port, timeout=10)

    if not path.startswith("/"):
        path = "/" + path

    hdrs = headers or {}
    conn.request(method, path, headers=hdrs)
    resp = conn.getresponse()

    status = resp.status
    resp_headers = {k: v for (k, v) in resp.getheaders()}
    body = resp.read()
    conn.close()
    return status, resp_headers, body


def _try_json(body_bytes: bytes) -> Optional[Any]:
    try:
        return json.loads(body_bytes.decode("utf-8"))
    except Exception:
        return None


def _subset_match(expected: Any, actual: Any, path: str = "") -> Tuple[bool, str]:
    """
    Check whether `expected` is a subset of `actual`:
      - for dicts: all expected keys must exist and match recursively
      - for lists: must be same length and each element must match
      - for scalars: equality
    Returns (ok, message).
    """
    if expected is None:
        return True, ""
    if isinstance(expected, dict):
        if not isinstance(actual, dict):
            return False, f"{path}: expected object, got {type(actual).__name__}"
        for k, v in expected.items():
            if k not in actual:
                return False, f"{path + '.' if path else ''}{k}: missing"
            ok, msg = _subset_match(v, actual[k], f"{path + '.' if path else ''}{k}")
            if not ok:
                return ok, msg
        return True, ""
    if isinstance(expected, list):
        if not isinstance(actual, list):
            return False, f"{path}: expected array, got {type(actual).__name__}"
        if len(expected) != len(actual):
            return False, f"{path}: expected length {len(expected)}, got {len(actual)}"
        for i, (e, a) in enumerate(zip(expected, actual)):
            ok, msg = _subset_match(e, a, f"{path}[{i}]")
            if not ok:
                return ok, msg
        return True, ""
    if expected != actual:
        return False, f"{path}: expected {expected!r}, got {actual!r}"
    return True, ""


def _dump_case_response(base_url: str, case: Dict[str, Any], label: str) -> None:
    method = case["method"]
    path = case.get("map_to") or case["path"]
    headers = case.get("headers", {})

    status, resp_headers, body = _http_request(base_url, method, path, headers=headers)
    lh = _lower_headers(resp_headers)

    print(f"\n=== {label}: {base_url} {method} {path} -> {status} ===")
    # Useful header subset
    for hk in ["content-type", "content-length", "x-demo", "x-powered-by"]:
        if hk in lh:
            print(f"{hk}: {lh[hk]}")

    if method.upper() == "HEAD":
        print("(HEAD) body intentionally not shown")
        return

    j = _try_json(body)
    if j is not None:
        print(json.dumps(j, indent=2, sort_keys=True))
    else:
        print(body.decode("utf-8", errors="replace"))


def run_cases(base_url: str, cases: List[Dict[str, Any]]) -> List[Tuple[str, str]]:
    """
    Validate a server against expected outputs.
    Returns list of (case_id, message) for failures.
    """
    failures: List[Tuple[str, str]] = []

    for c in cases:
        case_id = c["id"]
        method = c["method"]
        path = c.get("map_to") or c["path"]
        headers = c.get("headers", {})
        exp = c.get("expect", {})

        status, resp_headers, body = _http_request(base_url, method, path, headers=headers)
        lh = _lower_headers(resp_headers)

        # Status check
        exp_status = exp.get("status")
        if exp_status is not None and status != exp_status:
            failures.append((case_id, f"status expected {exp_status}, got {status}"))
            continue

        # Headers subset check (case-insensitive)
        exp_headers = exp.get("headers")
        if exp_headers:
            bad = False
            for hk, hv in exp_headers.items():
                if lh.get(hk.lower()) != hv:
                    failures.append((case_id, f"header {hk} expected {hv!r}, got {lh.get(hk.lower())!r}"))
                    bad = True
                    break
            if bad:
                continue

        # Body / JSON checks
        if method.upper() == "HEAD":
            body_text = ""  # treat as empty for HEAD (client view)
        else:
            body_text = body.decode("utf-8", errors="replace")

        exp_body = exp.get("body")
        if exp_body is not None:
            if body_text != exp_body:
                failures.append((case_id, f"body expected {exp_body!r}, got {body_text!r}"))
                continue

        exp_json = exp.get("json")
        if exp_json is not None:
            parsed = _try_json(body) if method.upper() != "HEAD" else None
            if parsed is None:
                failures.append((case_id, "expected JSON, got non-JSON body"))
                continue
            ok, msg = _subset_match(exp_json, parsed)
            if not ok:
                failures.append((case_id, f"json mismatch: {msg}"))
                continue

    return failures


def compare_servers(node_url: str, snode_url: str, cases: List[Dict[str, Any]]) -> List[Tuple[str, str]]:
    """
    Strict compare node vs snode:
      - status must match
      - if JSON parseable: JSON must match exactly
      - else: body bytes must match (HEAD treated as empty)
      - a few critical headers compared
    """
    failures: List[Tuple[str, str]] = []

    for c in cases:
        case_id = c["id"]
        method = c["method"]
        path = c.get("map_to") or c["path"]
        headers = c.get("headers", {})

        n_status, n_headers, n_body = _http_request(node_url, method, path, headers=headers)
        s_status, s_headers, s_body = _http_request(snode_url, method, path, headers=headers)

        if n_status != s_status:
            failures.append((case_id, f"status differs node={n_status} snode={s_status}"))
            continue

        # HEAD: compare as empty body (client view)
        if method.upper() == "HEAD":
            n_body_cmp = b""
            s_body_cmp = b""
        else:
            n_body_cmp = n_body
            s_body_cmp = s_body

        n_json = _try_json(n_body_cmp)
        s_json = _try_json(s_body_cmp)

        if n_json is not None or s_json is not None:
            if n_json != s_json:
                failures.append((case_id, "JSON differs between node and snode"))
                continue
        else:
            if n_body_cmp != s_body_cmp:
                failures.append((case_id, "body differs between node and snode"))
                continue

        # Compare a few critical headers (lowercase)
        ln = _lower_headers(n_headers)
        ls = _lower_headers(s_headers)
        for hk in ["x-demo"]:
            if ln.get(hk) != ls.get(hk):
                failures.append((case_id, f"header {hk} differs node={ln.get(hk)!r} snode={ls.get(hk)!r}"))
                break

    return failures


def _case_by_id(cases: List[Dict[str, Any]], case_id: str) -> Optional[Dict[str, Any]]:
    for c in cases:
        if c.get("id") == case_id:
            return c
    return None


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases", default="cases.json")
    ap.add_argument("--node", default=None)
    ap.add_argument("--snode", default=None)
    ap.add_argument("--expect-only", action="store_true", help="Validate only against expected values, do not cross-compare.")
    ap.add_argument("--dump-on-fail", action="store_true", help="Dump HTTP responses (JSON/body) for failing cases.")
    ap.add_argument("--only", default=None, help="Run only a single case id (exact match).")
    args = ap.parse_args()

    with open(args.cases, "r", encoding="utf-8") as f:
        cases: List[Dict[str, Any]] = json.load(f)

    if args.only:
        cases = [c for c in cases if c.get("id") == args.only]
        if not cases:
            print(f"No case found with id={args.only!r}", file=sys.stderr)
            return 2

    any_fail = False

    if args.expect_only:
        if args.node:
            fails = run_cases(args.node, cases)
            if fails:
                any_fail = True
                print("Node expected-value failures:")
                for cid, msg in fails:
                    print(f"  - {cid}: {msg}")
                if args.dump_on_fail:
                    for cid, _ in fails:
                        case = _case_by_id(cases, cid)
                        if case:
                            _dump_case_response(args.node, case, "node")
        if args.snode:
            fails = run_cases(args.snode, cases)
            if fails:
                any_fail = True
                print("SNode expected-value failures:")
                for cid, msg in fails:
                    print(f"  - {cid}: {msg}")
                if args.dump_on_fail:
                    for cid, _ in fails:
                        case = _case_by_id(cases, cid)
                        if case:
                            _dump_case_response(args.snode, case, "snode")
    else:
        if not (args.node and args.snode):
            print("Need --node and --snode for compare mode.", file=sys.stderr)
            return 2

        # Expected-value sanity for each server
        n_fails = run_cases(args.node, cases)
        s_fails = run_cases(args.snode, cases)

        if n_fails:
            any_fail = True
            print("Node expected-value failures:")
            for cid, msg in n_fails:
                print(f"  - {cid}: {msg}")
            if args.dump_on_fail:
                for cid, _ in n_fails:
                    case = _case_by_id(cases, cid)
                    if case:
                        _dump_case_response(args.node, case, "node")

        if s_fails:
            any_fail = True
            print("SNode expected-value failures:")
            for cid, msg in s_fails:
                print(f"  - {cid}: {msg}")
            if args.dump_on_fail:
                for cid, _ in s_fails:
                    case = _case_by_id(cases, cid)
                    if case:
                        _dump_case_response(args.snode, case, "snode")

        # Cross-compare
        comp = compare_servers(args.node, args.snode, cases)
        if comp:
            any_fail = True
            print("Node vs SNode differences:")
            for cid, msg in comp:
                print(f"  - {cid}: {msg}")
            if args.dump_on_fail:
                for cid, _ in comp:
                    case = _case_by_id(cases, cid)
                    if case:
                        _dump_case_response(args.node, case, "node")
                        _dump_case_response(args.snode, case, "snode")

    if any_fail:
        print("\nFAIL")
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
