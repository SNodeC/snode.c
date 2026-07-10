/*
 * snodec-control - Out-of-tree companion tool for SNode.C applications
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

#ifndef SNODECCONTROL_FIXTURES_NESTEDSUBCOMMANDFIXTURE_H
#define SNODECCONTROL_FIXTURES_NESTEDSUBCOMMANDFIXTURE_H

#include <string>

namespace snodec::control::test::fixtures {

    // Real `--show-config` output captured from a throwaway program (utils::SubCommand/utils::Config
    // only, no network stack) built against current SNode.C master. It registers: a custom
    // subcommand ("tool") at the application's top level; a custom subcommand ("echoserver", group
    // "Instances") containing another custom subcommand ("outer") which itself contains a third
    // ("inner"), i.e. three levels of genuine nesting beyond the root; and a disabled custom
    // subcommand ("legacy"). Captured verbatim, not hand-written, so parser/tree-builder tests
    // against it exercise the framework's real emission logic rather than the test author's
    // assumptions about it. See snodec-control/tests/fixtures/README.md for how it was produced.
    // NOLINTBEGIN
    const std::string nestedSubCommandShowConfigOutput = R"NESTEDFIXTURE(#@ snodec.meta begin document
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "document",
#@   "application": {
#@     "name": "/tmp/claude-0/-home-user-snode-c/5dabe021-4734-57a8-8805-2d422168d219/scratchpad/nested_fixture",
#@     "description": "Configuration for Application 'nested_fixture'"
#@   },
#@   "scope": "configurable-options-only",
#@   "syntax": "ini-with-comment-metadata"
#@ }
#@ snodec.meta end document
#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "application",
#@   "kindSource": "root",
#@   "name": "/tmp/claude-0/-home-user-snode-c/5dabe021-4734-57a8-8805-2d422168d219/scratchpad/nested_fixture",
#@   "displayName": "/tmp/claude-0/-home-user-snode-c/5dabe021-4734-57a8-8805-2d422168d219/scratchpad/nested_fixture",
#@   "path": [],
#@   "configPrefix": "",
#@   "group": "",
#@   "description": "Configuration for Application 'nested_fixture'",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Configuration for Application 'nested_fixture'
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": []
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-level",
#@   "key": "log-level",
#@   "localName": "log-level",
#@   "displayName": "log-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Log level (0/off, 1/critical, 2/error, 3/warn, 4/info, 5/debug, 6/trace)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-level",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "log-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "level:LOG_LEVEL",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "4",
#@     "configured": null,
#@     "effective": "4",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "4",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "4"
#@   }
#@ }
#@ snodec.meta end option
# Log level (0/off, 1/critical, 2/error, 3/warn, 4/info, 5/debug, 6/trace)
#log-level=4

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "verbose-level",
#@   "key": "verbose-level",
#@   "localName": "verbose-level",
#@   "displayName": "verbose-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Legacy verbose level (0..10; no semantic-log effect)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--verbose-level",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "verbose-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "level:INT in [0 - 10]",
#@     "kind": "integer",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "2",
#@     "configured": null,
#@     "effective": "2",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "2",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "2"
#@   }
#@ }
#@ snodec.meta end option
# Legacy verbose level (0..10; no semantic-log effect)
#verbose-level=2

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-format",
#@   "key": "log-format",
#@   "localName": "log-format",
#@   "displayName": "log-format",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic log format",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-format",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "log-format",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "text|json:FORMAT",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "text",
#@     "configured": null,
#@     "effective": "text",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"text\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"text\""
#@   }
#@ }
#@ snodec.meta end option
# Semantic log format
#log-format="text"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-origin-level",
#@   "key": "log-origin-level",
#@   "localName": "log-origin-level",
#@   "displayName": "log-origin-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic origin log level override (framework|application=level)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-origin-level",
#@     "short": null,
#@     "takesValue": false,
#@     "repeatable": true
#@   },
#@   "configFile": {
#@     "key": "log-origin-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "origin=level:(TEXT) AND (ORIGIN_LEVEL)",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Semantic origin log level override (framework|application=level)
#log-origin-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-boundary-level",
#@   "key": "log-boundary-level",
#@   "localName": "log-boundary-level",
#@   "displayName": "log-boundary-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic boundary log level override (application|configuration|instance|connection|context|system=level)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-boundary-level",
#@     "short": null,
#@     "takesValue": false,
#@     "repeatable": true
#@   },
#@   "configFile": {
#@     "key": "log-boundary-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "boundary=level:(TEXT) AND (BOUNDARY_LEVEL)",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Semantic boundary log level override (application|configuration|instance|connection|context|system=level)
#log-boundary-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-component-level",
#@   "key": "log-component-level",
#@   "localName": "log-component-level",
#@   "displayName": "log-component-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic component log level override (component=level)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-component-level",
#@     "short": null,
#@     "takesValue": false,
#@     "repeatable": true
#@   },
#@   "configFile": {
#@     "key": "log-component-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "component=level:(TEXT) AND (COMPONENT_LEVEL)",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Semantic component log level override (component=level)
#log-component-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-instance-level",
#@   "key": "log-instance-level",
#@   "localName": "log-instance-level",
#@   "displayName": "log-instance-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic instance log level override (instance=level)",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-instance-level",
#@     "short": null,
#@     "takesValue": false,
#@     "repeatable": true
#@   },
#@   "configFile": {
#@     "key": "log-instance-level",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "instance=level:(TEXT) AND (INSTANCE_LEVEL)",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Semantic instance log level override (instance=level)
#log-instance-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "log-file",
#@   "key": "log-file",
#@   "localName": "log-file",
#@   "displayName": "log-file",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Log file",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--log-file",
#@     "short": "-l",
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "log-file",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "logFile:NOT DIR",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "/var/log/snode.c/nested_fixture.log",
#@     "configured": null,
#@     "effective": "/var/log/snode.c/nested_fixture.log",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"/var/log/snode.c/nested_fixture.log\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"/var/log/snode.c/nested_fixture.log\""
#@   }
#@ }
#@ snodec.meta end option
# Log file
#log-file="/var/log/snode.c/nested_fixture.log"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "monochrom",
#@   "key": "monochrom",
#@   "localName": "monochrom",
#@   "displayName": "monochrom",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Monochrom log output",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--monochrom",
#@     "short": "-m",
#@     "takesValue": false,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "monochrom",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "BOOL:{true,false}",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "true",
#@     "configured": null,
#@     "effective": "true",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "true",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "true"
#@   }
#@ }
#@ snodec.meta end option
# Monochrom log output
#monochrom=true

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "quiet",
#@   "key": "quiet",
#@   "localName": "quiet",
#@   "displayName": "quiet",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Quiet mode",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--quiet",
#@     "short": "-q",
#@     "takesValue": false,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "quiet",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "BOOL:{true,false}",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Quiet mode
#quiet=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "enforce-log-file",
#@   "key": "enforce-log-file",
#@   "localName": "enforce-log-file",
#@   "displayName": "enforce-log-file",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Enforce writing of logs to file for foreground applications",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--enforce-log-file",
#@     "short": "-e",
#@     "takesValue": false,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "enforce-log-file",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "BOOL:{true,false}",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Enforce writing of logs to file for foreground applications
#enforce-log-file=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "daemonize",
#@   "key": "daemonize",
#@   "localName": "daemonize",
#@   "displayName": "daemonize",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Start application as daemon",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--daemonize",
#@     "short": "-d",
#@     "takesValue": false,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "daemonize",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "BOOL:{true,false}",
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "false",
#@     "configured": null,
#@     "effective": "false",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "false",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "false"
#@   }
#@ }
#@ snodec.meta end option
# Start application as daemon
#daemonize=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "user-name",
#@   "key": "user-name",
#@   "localName": "user-name",
#@   "displayName": "user-name",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Run daemon under specific user permissions",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--user-name",
#@     "short": "-u",
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "user-name",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "username:TEXT",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "root",
#@     "configured": null,
#@     "effective": "root",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"root\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"root\""
#@   }
#@ }
#@ snodec.meta end option
# Run daemon under specific user permissions
#user-name="root"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "group-name",
#@   "key": "group-name",
#@   "localName": "group-name",
#@   "displayName": "group-name",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Run daemon under specific group permissions",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--group-name",
#@     "short": "-g",
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "group-name",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "groupname:TEXT",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "root",
#@     "configured": null,
#@     "effective": "root",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"root\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"root\""
#@   }
#@ }
#@ snodec.meta end option
# Run daemon under specific group permissions
#group-name="root"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "name": "tool",
#@   "displayName": "tool",
#@   "path": ["tool"],
#@   "configPrefix": "tool.",
#@   "group": "Tools",
#@   "description": "Top-level custom subcommand alongside an Instances-style node",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Top-level custom subcommand alongside an Instances-style node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": ["tool"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "tool.tool-value",
#@   "key": "tool.tool-value",
#@   "localName": "tool-value",
#@   "displayName": "tool-value",
#@   "nodePath": ["tool"],
#@   "group": "Options (persistent)",
#@   "description": "A top-level tool option",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--tool-value",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "tool.tool-value",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "value",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "tool",
#@     "configured": null,
#@     "effective": "tool",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"tool\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"tool\""
#@   }
#@ }
#@ snodec.meta end option
# A top-level tool option
#tool.tool-value="tool"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "instance",
#@   "kindSource": "cli11-group",
#@   "name": "echoserver",
#@   "displayName": "echoserver",
#@   "path": ["echoserver"],
#@   "configPrefix": "echoserver.",
#@   "group": "Instances",
#@   "description": "A stand-in instance node",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": false
#@ }
#@ snodec.meta end node
### A stand-in instance node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": ["echoserver"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "echoserver.enabled",
#@   "key": "echoserver.enabled",
#@   "localName": "enabled",
#@   "displayName": "enabled",
#@   "nodePath": ["echoserver"],
#@   "group": "Options (persistent)",
#@   "description": "Whether the instance is enabled",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--enabled",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "echoserver.enabled",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "BOOL:{true,false}",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "true",
#@     "configured": null,
#@     "effective": "true",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "true",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "true"
#@   }
#@ }
#@ snodec.meta end option
# Whether the instance is enabled
#echoserver.enabled=true

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "name": "outer",
#@   "displayName": "outer",
#@   "path": ["echoserver", "outer"],
#@   "configPrefix": "echoserver.outer.",
#@   "group": "Custom",
#@   "description": "Outer custom node containing another custom node",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Outer custom node containing another custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": ["echoserver", "outer"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "echoserver.outer.outer-value",
#@   "key": "echoserver.outer.outer-value",
#@   "localName": "outer-value",
#@   "displayName": "outer-value",
#@   "nodePath": ["echoserver", "outer"],
#@   "group": "Options (persistent)",
#@   "description": "An outer option",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--outer-value",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "echoserver.outer.outer-value",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "value",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "outer",
#@     "configured": null,
#@     "effective": "outer",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"outer\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"outer\""
#@   }
#@ }
#@ snodec.meta end option
# An outer option
#echoserver.outer.outer-value="outer"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "name": "inner",
#@   "displayName": "inner",
#@   "path": ["echoserver", "outer", "inner"],
#@   "configPrefix": "echoserver.outer.inner.",
#@   "group": "Deep",
#@   "description": "Innermost custom node",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Innermost custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": ["echoserver", "outer", "inner"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "echoserver.outer.inner.depth-value",
#@   "key": "echoserver.outer.inner.depth-value",
#@   "localName": "depth-value",
#@   "displayName": "depth-value",
#@   "nodePath": ["echoserver", "outer", "inner"],
#@   "group": "Options (persistent)",
#@   "description": "A deeply nested option",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--depth-value",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "echoserver.outer.inner.depth-value",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "value",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "deep",
#@     "configured": null,
#@     "effective": "deep",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"deep\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"deep\""
#@   }
#@ }
#@ snodec.meta end option
# A deeply nested option
#echoserver.outer.inner.depth-value="deep"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "node",
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "name": "legacy",
#@   "displayName": "legacy",
#@   "path": ["legacy"],
#@   "configPrefix": "legacy.",
#@   "group": "Custom",
#@   "description": "A disabled custom node",
#@   "configurable": false,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "disabled": true
#@ }
#@ snodec.meta end node
### A disabled custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "group",
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "kindSource": "heuristic-group-name",
#@   "nodePath": ["legacy"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "entity": "option",
#@   "id": "legacy.legacy-value",
#@   "key": "legacy.legacy-value",
#@   "localName": "legacy-value",
#@   "displayName": "legacy-value",
#@   "nodePath": ["legacy"],
#@   "group": "Options (persistent)",
#@   "description": "An option under a disabled node",
#@   "configurable": true,
#@   "required": {
#@     "effective": false,
#@     "source": "cli11-current-state"
#@   },
#@   "persistent": true,
#@   "commandLine": {
#@     "long": "--legacy-value",
#@     "short": null,
#@     "takesValue": true,
#@     "repeatable": false
#@   },
#@   "configFile": {
#@     "key": "legacy.legacy-value",
#@     "section": null,
#@     "writable": true
#@   },
#@   "type": {
#@     "name": "value",
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "constraints": [],
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   },
#@   "value": {
#@     "registrationDefault": null,
#@     "registrationDefaultSource": "not-tracked",
#@     "cppDefault": "legacy",
#@     "configured": null,
#@     "effective": "legacy",
#@     "source": "cpp-default",
#@     "isCppDefault": true,
#@     "isExplicitlyConfigured": false,
#@     "isMissingRequired": false,
#@     "registrationDefaultLiteral": null,
#@     "cppDefaultLiteral": "\"legacy\"",
#@     "configuredLiteral": null,
#@     "effectiveLiteral": "\"legacy\""
#@   }
#@ }
#@ snodec.meta end option
# An option under a disabled node
#legacy.legacy-value="legacy"

)NESTEDFIXTURE";
    // NOLINTEND

} // namespace snodec::control::test::fixtures

#endif // SNODECCONTROL_FIXTURES_NESTEDSUBCOMMANDFIXTURE_H
