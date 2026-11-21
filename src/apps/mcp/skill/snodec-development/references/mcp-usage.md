# Using the development MCP server

Use the installed executables `snodec-dev-stdio` and `snodec-dev-http`. Select the installed header tree with `snodec-dev-stdio mcp --header-root DIRECTORY` or `snodec-dev-http mcp-http mcp --header-root DIRECTORY` when it differs from the configured installation prefix. Read the relevant section's `--help` for options and limits; transport setup and protocol qualifications are documented in `docs/mcp/README.md` when a source checkout is available.

## API evidence

1. Call `api.find` with `{"query":"SNodeC","limit":5}`. Search is literal and case-sensitive. Results contain paths relative to the confined header root, one-based lines, and byte offsets. A `truncated` result means the result or traversal budget stopped the search; absence is not proof that an API does not exist elsewhere.
2. Call `api.read` with a returned relative path, for example `{"path":"core/SNodeC.h","offset":0,"bytes":8192}`. Reads are bounded UTF-8 byte ranges; a range splitting an encoding is rejected. Absolute paths, `..`, symlinks, non-header files, and special files are rejected.
3. Compare the declaration with the current checkout's implementation and callers before using it. The MCP tools inspect headers; they do not establish runtime behavior or verify that an installed tree matches the checkout.

The server performs bounded slices between event-loop iterations. Cancel work that is no longer needed. Transport deadlines and configured byte/file limits also bound searches. Tool input-value validation and tool execution failures are returned with `isError`; JSON-RPC errors identify malformed calls or protocol failures. Inspect both layers.

## Previews and review

Call `skeleton.preview` with `{"kind":"application"}` or `{"kind":"module"}`. The result's `structuredContent.files` maps relative paths to proposed text. Review instance names, dependencies, central export registration, ownership, and lifecycle before applying a preview within an authorized editing task. The tool writes no files.

Read the resource `snodec://development/house-style` for the concise runtime conventions. Get prompt `snodec-review` with a string `code` argument of at most 16384 bytes to obtain a review instruction containing that code as data. The prompt does not execute or review code by itself.

The first application exposes exactly these three tools, one resource, and one prompt. It has no build/run, configuration-inspection, command-execution, or file-writing tool. Do not infer those capabilities from the broader MCP framework.
