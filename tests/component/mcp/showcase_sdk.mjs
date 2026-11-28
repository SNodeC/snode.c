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
import {pathToFileURL} from 'node:url';

const [sdk, kind, target, era, root] = process.argv.slice(2);
const {Client, StreamableHTTPClientTransport} = await import(pathToFileURL(`${sdk}/dist/index.mjs`));
const {StdioClientTransport} = await import(pathToFileURL(`${sdk}/dist/stdio.mjs`));
const modern = era === '2026-07-28';
const counts = {inputs: 0, progress: 0, updated: 0, tools: 0, resources: 0, prompts: 0, logs: 0};
const clientInfo = {name: 'snodec-feature-qualification', version: '1'};
const capabilities = {elicitation: {form: {}, url: {}}, extensions: {'io.modelcontextprotocol/tasks': {}}};
const client = new Client(clientInfo, {
    capabilities,
    versionNegotiation: {mode: modern ? {pin: era} : 'legacy'}
});
client.setRequestHandler('elicitation/create', async request => {
    ++counts.inputs;
    await new Promise(resolve => setTimeout(resolve, 10));
    return request.params.mode === 'url' ? {action: 'accept'} : {action: 'accept', content: {label: 'sdk-only'}};
});
for (const [method, key] of [['notifications/resources/updated', 'updated'], ['notifications/tools/list_changed', 'tools'],
    ['notifications/resources/list_changed', 'resources'], ['notifications/prompts/list_changed', 'prompts'], ['notifications/message', 'logs']]) {
    client.setNotificationHandler(method, () => { ++counts[key]; });
}
const transport = kind === 'stdio'
    ? new StdioClientTransport({command: target, args: ['--config-file', '/dev/null', '--quiet', 'mcp', '--header-root', root, '--page-size', '3', '--deadline-ms', '2000'], stderr: 'pipe'})
    : new StreamableHTTPClientTransport(new URL(target), {requestInit: {headers: {Authorization: 'Bearer sdk-only'}}});
if (kind === 'stdio') {
    transport.stderr.on('data', data => { assert.ok(!data.toString().includes('AddressSanitizer')); });
}
const passed = [];
const failures = [];
const wireProgress = [];
const trace = [];
const sdkErrors = [];
const originalSend = transport.send.bind(transport);
transport.send = (...args) => {
    trace.push({direction: "client-to-server", message: args[0]});
    return originalSend(...args);
};
transport.onmessage = message => {
    trace.push({direction: "server-to-client", message});
    if (message.method === 'notifications/progress') {
        wireProgress.push(message.params.progress);
    }
};
client.onerror = error => sdkErrors.push(error.message);
const step = async (name, run) => {
    try {
        await run();
        passed.push(name);
        console.log(JSON.stringify({feature: name, outcome: 'PASS'}));
    } catch (error) {
        failures.push({feature: name, message: error.message, stack: error.stack});
        console.log(JSON.stringify({feature: name, outcome: 'FAIL', error: error.message}));
    }
};
const wait = async predicate => {
    const end = Date.now() + 3000;
    while (!predicate() && Date.now() < end) {
        await new Promise(resolve => setTimeout(resolve, 10));
    }
    assert.ok(predicate(), JSON.stringify(counts));
};
let subscription;
try {
    await client.connect(transport);
    assert.equal(client.getNegotiatedProtocolVersion(), era);
    await step('pagination', async () => {
        assert.equal((await client.listTools()).tools.length, 37);
        assert.equal((await client.listResources()).resources.length, 15);
        assert.equal((await client.listResourceTemplates()).resourceTemplates.length, 12);
        assert.equal((await client.listPrompts()).prompts.length, 12);
    });
    for (const name of ['text', 'tool.error', 'structured.valid', 'structured.invalid', 'content', 'principal']) {
        await step(name, async () => {
            const result = await client.callTool({name});
            assert.equal(!!result.isError, ['tool.error', 'structured.invalid'].includes(name));
            if (name === 'content') {
                assert.deepEqual(result.content.map(value => value.type), ['text', 'image', 'audio', 'resource_link', 'resource', 'resource']);
            }
        });
    }
    await step('protocol.error', () => assert.rejects(client.callTool({name: 'protocol.error'}), /Deliberate protocol error/));
    await step('schema', async () => {
        const result = await client.callTool({name: 'schema', arguments: {word: 'Grüße世界', choice: 2}});
        assert.equal(result.structuredContent.word, 'Grüße世界');
    });
    await step('headers', async () => {
        const result = await client.callTool({name: 'headers', arguments: {region: 'Hello, 世界'}});
        assert.equal(result.structuredContent.region, 'Hello, 世界');
    });
    await step('resources', async () => {
        assert.equal((await client.readResource({uri: 'showcase://text'})).contents[0].text, 'SNode.C\n');
        assert.equal((await client.readResource({uri: 'showcase://blob'})).contents[0].blob, 'AAEC');
        assert.equal((await client.readResource({uri: 'showcase://greeting/Ada'})).contents[0].text, 'Hello Ada');
        assert.equal((await client.readResource({uri: 'showcase://path/a/b'})).contents[0].text, 'a/b');
    });
    await step('prompt', async () => { assert.equal((await client.getPrompt({name: 'conversation', arguments: {name: 'Ada'}})).messages.length, 2); });
    await step('completion', async () => {
        for (const ref of [{type: 'ref/prompt', name: 'conversation'}, {type: 'ref/resource', uri: 'showcase://greeting/{name}'}]) {
            assert.deepEqual((await client.complete({ref, argument: {name: 'name', value: 'a'}})).completion.values, ['alpha']);
        }
    });
    await step('subscriptions', async () => {
        if (modern) {
            subscription = await client.listen({resourceSubscriptions: ['showcase://counter'], toolsListChanged: true, resourcesListChanged: true, promptsListChanged: true});
        } else {
            await client.subscribeResource({uri: 'showcase://counter'});
        }
        await client.callTool({name: 'update'});
        await wait(() => counts.updated === 1);
    });
    await step('dynamic', async () => {
        await client.callTool({name: 'dynamic', arguments: {action: 'add'}});
        assert.ok((await client.listTools()).tools.some(tool => tool.name === 'dynamic.tool'));
        await client.callTool({name: 'dynamic', arguments: {action: 'remove'}});
        await wait(() => counts.tools === 2 && counts.resources === 2 && counts.prompts === 2);
        assert.ok(!(await client.listTools()).tools.some(tool => tool.name === 'dynamic.tool'));
    });
    await step('elicitation', async () => {
        for (const name of ['input.form', 'input.url', 'input.rounds']) {
            const result = await client.callTool({name});
            assert.equal(result.structuredContent.action, 'accept');
        }
        const parallel = await client.callTool({name: 'input.parallel'});
        assert.equal(parallel.structuredContent.first.label, 'sdk-only');
        assert.equal(parallel.structuredContent.second.label, 'sdk-only');
        const resource = await client.readResource({uri: 'showcase://input'});
        assert.equal(JSON.parse(resource.contents[0].text).label, 'sdk-only');
        const prompt = await client.getPrompt({name: 'input'});
        assert.equal(JSON.parse(prompt.messages[0].content.text).label, 'sdk-only');
        assert.equal(counts.inputs, 8);
    });
    await step('tasks', async () => {
        const {createTaskSessionFromClient, resultFromTaskOutcome, toolDeclarationFromMcpTool} = await import(pathToFileURL(`${sdk}/../ext-tasks/dist/client/index.js`));
        const tools = (await client.listTools()).tools;
        // ext-tasks documents this host-owned raw dispatch for modern framing.
        // String ids cannot collide with the public client's numeric request ids.
        const pending = new Map();
        let sequence = 0;
        const previous = transport.onmessage;
        transport.onmessage = (message, ...args) => {
            const entry = pending.get(message.id);
            if (entry) {
                trace.push({direction: 'server-to-client', message});
                pending.delete(message.id);
                clearTimeout(entry.timer);
                entry.resolve(message.error ? {kind: 'error', error: message.error} : {kind: 'result', result: message.result});
            } else {
                previous(message, ...args);
            }
        };
        const rawDispatch = (request, options = {}) => new Promise((resolve, reject) => {
            const id = 'showcase-extension-' + (++sequence);
            const timer = setTimeout(() => {
                pending.delete(id);
                reject(new Error('Task extension dispatch timeout'));
            }, 5000);
            pending.set(id, {resolve, reject, timer});
            transport.send({jsonrpc: '2.0', ...request, id}, {requestSignal: options.signal}).catch(error => {
                clearTimeout(timer);
                pending.delete(id);
                reject(error);
            });
        });
        const session = createTaskSessionFromClient(client, {
            endpointId: 'snodec-feature-test',
            tools: {currentTool: name => toolDeclarationFromMcpTool(tools.find(tool => tool.name === name))},
            ...(modern ? {rawDispatch, v2RequestFraming: {protocolVersion: era, clientInfo, clientCapabilities: capabilities}} : {})
        });
        try {
            for (const name of ['task.optional', 'task.required']) {
                const execution = await session.callTool(name, {steps: 3}, {task: {preference: 'require', retentionMs: 5000}});
                assert.equal(execution.kind, 'task');
                const {outcome} = await execution.settle();
                assert.equal(resultFromTaskOutcome(outcome).structuredContent.completed, 3);
            }
            if (!modern) {
                assert.ok((await session.listTasks()).tasks.length >= 2);
            }
            const execution = await session.callTool('task.required', {steps: 20, delayMs: 100}, {task: {preference: 'require'}});
            assert.equal(execution.kind, 'task');
            await execution.cancel();
            assert.equal((await execution.result()).status, 'cancelled');
        } finally {
            await session.close();
            transport.onmessage = previous;
            for (const entry of pending.values()) {
                clearTimeout(entry.timer);
                entry.reject(new Error('Task extension session closed'));
            }
        }
    });
    await step('progress', async () => {
        const result = await client.callTool({name: 'work', arguments: {steps: 3}}, {onprogress: () => { ++counts.progress; }});
        assert.equal(result.structuredContent.completed, 3);
        assert.deepEqual(wireProgress, [1, 2, 3], 'all progress messages precede the final response on the public transport');
        console.log(JSON.stringify({progress: {wire: wireProgress, callbacks: counts.progress, sdkErrors}}));
        assert.equal(counts.progress, 3);
    });
    await step('cancellation', async () => {
        const abort = new AbortController();
        const timer = setTimeout(() => abort.abort(), 50);
        try {
            await assert.rejects(client.callTool({name: 'work', arguments: {steps: 20, delayMs: 100}}, {signal: abort.signal}));
        } finally {
            clearTimeout(timer);
        }
        assert.ok((await client.callTool({name: 'text'})).content.length);
    });
    await step('deadline', async () => {
        const result = await client.callTool({name: 'work', arguments: {steps: 20, delayMs: 1000}});
        assert.equal(result.isError, true);
        assert.equal(result.content[0].text, 'Request deadline exceeded');
    });
    if (!modern) {
        await step('logging', async () => {
            await client.setLoggingLevel('debug');
            await client.callTool({name: 'logging'});
            await wait(() => counts.logs === 2);
        });
    }
    if (subscription) {
        await subscription.close();
    } else {
        await client.unsubscribeResource({uri: 'showcase://counter'});
    }
    console.log(JSON.stringify({sdk: failures.length ? 'FAIL' : 'PASS', era, kind, passed, counts, failures}));
    assert.equal(failures.length, 0, JSON.stringify(failures));
} finally {
    await client.close();
    await new Promise((resolve, reject) => {
        process.stdout.write(JSON.stringify({wireTrace: trace}, null, 4) + '\n', error => {
            if (error) {
                reject(error);
            } else {
                resolve();
            }
        });
    });
}
