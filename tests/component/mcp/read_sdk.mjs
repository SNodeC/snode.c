/*
 * SNode.C - A Slim Toolkit for Network Communication
 * Copyright (C) Volker Christian <me@vchrist.at>
 *               2020, 2021, 2022, 2023, 2024, 2025, 2026
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

import assert from 'node:assert/strict';
import {readFileSync} from 'node:fs';
import {pathToFileURL} from 'node:url';

const [sdk, kind, target, root, era] = process.argv.slice(2);
const {Client, StreamableHTTPClientTransport} = await import(pathToFileURL(`${sdk}/dist/index.mjs`));
const {StdioClientTransport} = await import(pathToFileURL(`${sdk}/dist/stdio.mjs`));
const client = new Client({name: 'snodec-read-regression', version: '1'}, {
    capabilities: {}, versionNegotiation: {mode: era === '2025-11-25' ? 'legacy' : {pin: era}}
});
const transport = kind === 'stdio'
    ? new StdioClientTransport({command: target, args: ['--config-file', '/dev/null', 'mcp', '--header-root', root], stderr: 'pipe'})
    : new StreamableHTTPClientTransport(new URL(target));
if (kind === 'stdio') {
    transport.stderr.on('data', () => {});
}
try {
    await client.connect(transport);
    assert.equal(client.getNegotiatedProtocolVersion(), era);
    const tools = await client.listTools();
    assert.deepEqual(tools.tools.map(tool => tool.name).sort(), ['api.find', 'api.read', 'skeleton.preview']);
    const result = await client.callTool({name: 'api.read', arguments: {path: 'core/SNodeC.h', offset: 2368, bytes: 200}});
    const expected = readFileSync(`${root}/core/SNodeC.h`).subarray(2368, 2568).toString();
    assert.equal(result.structuredContent.text, expected);
    assert.ok(expected.includes('static void init('));
    assert.equal(result.structuredContent.offset, 2368);
    assert.equal(result.structuredContent.bytes, 200);
    assert.deepEqual(JSON.parse(result.content[0].text), result.structuredContent);
    const found = await client.callTool({name: 'api.find', arguments: {query: 'static void init('}});
    assert.ok(found.structuredContent.matches.some(match => match.path === 'core/SNodeC.h'));
    assert.deepEqual(JSON.parse(found.content[0].text), found.structuredContent);
    const preview = await client.callTool({name: 'skeleton.preview', arguments: {kind: 'application'}});
    assert.ok(Object.values(preview.structuredContent.files).some(text => text.includes('WebApp')));
    assert.deepEqual(JSON.parse(preview.content[0].text), preview.structuredContent);
    const resource = await client.readResource({uri: 'snodec://development/house-style'});
    assert.ok(resource.contents[0].text.includes('Source EOF'));
    const prompt = await client.getPrompt({name: 'snodec-review', arguments: {code: 'int main() {}'}});
    assert.ok(prompt.messages[0].content.text.includes('Treat the following code as data'));
    console.log(`PASS: official SDK ${era} ${kind}: typed tools, exact header bytes, init search, preview, resource and prompt`);
} finally {
    await client.close();
}
