// SPDX-License-Identifier: LGPL-3.0-or-later OR MIT
import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {pathToFileURL} from 'node:url';

const [sdk, kind, target, root] = process.argv.slice(2);
const {Client, StreamableHTTPClientTransport} = await import(pathToFileURL(`${sdk}/dist/index.mjs`));
const {StdioClientTransport} = await import(pathToFileURL(`${sdk}/dist/stdio.mjs`));
const client = new Client({name: 'snodec-read-regression', version: '1'}, {capabilities: {}});
const transport = kind === 'stdio'
    ? new StdioClientTransport({command: target, args: ['--config-file', '/dev/null', 'mcp', '--header-root', root], stderr: 'pipe'})
    : new StreamableHTTPClientTransport(new URL(target));
if (kind === 'stdio') {
    transport.stderr.on('data', () => {});
}
try {
    await client.connect(transport);
    const result = await client.callTool({name: 'api.read', arguments: {path: 'core/SNodeC.h', offset: 2368, bytes: 200}});
    const expected = readFileSync(`${root}/core/SNodeC.h`).subarray(2368, 2568).toString();
    assert.equal(result.structuredContent.text, expected);
    assert.ok(expected.includes('static void init('));
    assert.equal(result.structuredContent.offset, 2368);
    assert.equal(result.structuredContent.bytes, 200);
    assert.deepEqual(JSON.parse(result.content[0].text), result.structuredContent);
    console.log(`PASS: official SDK ${kind} api.read exact source bytes`);
} finally {
    await client.close();
}
