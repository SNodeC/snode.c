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

import {pathToFileURL} from 'node:url';
import {resolve} from 'node:path';
import {createServer} from 'node:http';
const [root, kind, port] = process.argv.slice(2);
const sdk = await import(pathToFileURL(resolve(root, '../server/dist/index.mjs')));
const {StdioServerTransport} = await import(pathToFileURL(resolve(root, '../server/dist/stdio.mjs')));
function create() {
  const server = new sdk.McpServer({name:'official-sdk-fixture',version:'2.4.0'});
  server.registerTool('echo', {inputSchema:sdk.fromJsonSchema({type:'object',properties:{value:{type:'string'}},required:['value']})}, async ({value}) => ({content:[{type:'text',text:value}]}));
  server.registerResource('resource', 'sdk://resource', {mimeType:'text/plain'}, async () => ({contents:[{uri:'sdk://resource',mimeType:'text/plain',text:'SDK resource'}]}));
  server.registerPrompt('prompt', {}, async () => ({messages:[{role:'user',content:{type:'text',text:'SDK prompt'}}]}));
  return server;
}
if (kind === 'stdio') {
  await create().connect(new StdioServerTransport());
} else {
  const handler = sdk.createMcpHandler(create);
  const server = createServer(async (incoming, outgoing) => {
    try {
      const chunks = [];
      for await (const chunk of incoming) { chunks.push(chunk); }
      const abort = new AbortController();
      outgoing.on('close', () => abort.abort());
      const request = new Request('http://127.0.0.1:' + port + incoming.url, {
        method:incoming.method,headers:incoming.headers,signal:abort.signal,
        ...(incoming.method !== 'GET' && incoming.method !== 'HEAD' ? {body:Buffer.concat(chunks)} : {})
      });
      const response = await handler.fetch(request);
      if (response.status >= 400) { console.error(response.status, await response.clone().text()); }
      outgoing.writeHead(response.status, Object.fromEntries(response.headers));
      if (response.body) { for await (const chunk of response.body) { outgoing.write(chunk); } }
      outgoing.end();
    } catch (error) { console.error(error); if (!outgoing.headersSent) { outgoing.writeHead(500); } outgoing.end(); }
  });
  server.listen(Number(port), '127.0.0.1');
  process.on('SIGTERM', () => { server.closeAllConnections(); server.close(); });
}
