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

    // Real `--show-config=extensive` output captured from a throwaway program (utils::SubCommand/utils::Config
    // only, no network stack) built against the repository sources. It registers: a custom
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
#@   "version": 1
#@ }
#@ snodec.meta end document
#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "application",
#@   "kindSource": "root",
#@   "displayName": "nested_fixture",
#@   "path": [],
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Configuration for Application 'nested_fixture'
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": []
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Log level (0/off, 1/critical, 2/error, 3/warn, 4/info, 5/debug, 6/trace)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Log level (0/off, 1/critical, 2/error, 3/warn, 4/info, 5/debug, 6/trace)
#log-level=4

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "verbose-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Legacy verbose level (0..10; no semantic-log effect)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "integer",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Legacy verbose level (0..10; no semantic-log effect)
#verbose-level=2

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-format",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic log format",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Semantic log format
#log-format="text"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-origin-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic origin log level override (framework|application=level)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Semantic origin log level override (framework|application=level)
#log-origin-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-boundary-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic boundary log level override (application|configuration|instance|connection|context|system=level)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Semantic boundary log level override (application|configuration|instance|connection|context|system=level)
#log-boundary-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-component-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic component log level override (component=level)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Semantic component log level override (component=level)
#log-component-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-instance-level",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Semantic instance log level override (instance=level)",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "list"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Semantic instance log level override (instance=level)
#log-instance-level=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "log-file",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Log file",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Log file
#log-file="/tmp/codex/build/snodec/config-metadata-minimal/xdg-state/snode.c/nested_fixture.log"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "monochrom",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Monochrom log output",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Monochrom log output
#monochrom=true

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "quiet",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Quiet mode",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Quiet mode
#quiet=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "enforce-log-file",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Enforce writing of logs to file for foreground applications",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Enforce writing of logs to file for foreground applications
#enforce-log-file=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "daemonize",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Start application as daemon",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "boolean",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Start application as daemon
#daemonize=false

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "user-name",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Run daemon under specific user permissions",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Run daemon under specific user permissions
#user-name="voc"

#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "group-name",
#@   "nodePath": [],
#@   "group": "Options (persistent)",
#@   "description": "Run daemon under specific group permissions",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Run daemon under specific group permissions
#group-name="voc"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "displayName": "tool",
#@   "path": ["tool"],
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Top-level custom subcommand alongside an Instances-style node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": ["tool"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "tool.tool-value",
#@   "nodePath": ["tool"],
#@   "group": "Options (persistent)",
#@   "description": "A top-level tool option",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# A top-level tool option
#tool.tool-value="tool"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "instance",
#@   "kindSource": "cli11-group",
#@   "displayName": "echoserver",
#@   "path": ["echoserver"],
#@   "disabled": false
#@ }
#@ snodec.meta end node
### A stand-in instance node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": ["echoserver"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "echoserver.enabled",
#@   "nodePath": ["echoserver"],
#@   "group": "Options (persistent)",
#@   "description": "Whether the instance is enabled",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# Whether the instance is enabled
#echoserver.enabled=true

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "displayName": "outer",
#@   "path": ["echoserver", "outer"],
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Outer custom node containing another custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": ["echoserver", "outer"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "echoserver.outer.outer-value",
#@   "nodePath": ["echoserver", "outer"],
#@   "group": "Options (persistent)",
#@   "description": "An outer option",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# An outer option
#echoserver.outer.outer-value="outer"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "displayName": "inner",
#@   "path": ["echoserver", "outer", "inner"],
#@   "disabled": false
#@ }
#@ snodec.meta end node
### Innermost custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": ["echoserver", "outer", "inner"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "echoserver.outer.inner.depth-value",
#@   "nodePath": ["echoserver", "outer", "inner"],
#@   "group": "Options (persistent)",
#@   "description": "A deeply nested option",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# A deeply nested option
#echoserver.outer.inner.depth-value="deep"

#@ snodec.meta begin node
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "kind": "category",
#@   "kindSource": "heuristic-cli11-group",
#@   "displayName": "legacy",
#@   "path": ["legacy"],
#@   "disabled": true
#@ }
#@ snodec.meta end node
### A disabled custom node
#@ snodec.meta begin group
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "name": "Options (persistent)",
#@   "kind": "persistent",
#@   "nodePath": ["legacy"]
#@ }
#@ snodec.meta end group

## Options (persistent)
#@ snodec.meta begin option
#@ {
#@   "schema": "snodec.config.comment-meta",
#@   "version": 1,
#@   "key": "legacy.legacy-value",
#@   "nodePath": ["legacy"],
#@   "group": "Options (persistent)",
#@   "description": "An option under a disabled node",
#@   "required": {
#@     "source": "cli11-current-state"
#@   },
#@   "type": {
#@     "kind": "string",
#@     "kindSource": "heuristic-name",
#@     "items": "single"
#@   },
#@   "relations": {
#@     "needs": [
#@     ],
#@     "excludes": [
#@     ]
#@   }
#@ }
#@ snodec.meta end option
# An option under a disabled node
#legacy.legacy-value="legacy"

)NESTEDFIXTURE";
    // NOLINTEND

} // namespace snodec::control::test::fixtures

#endif // SNODECCONTROL_FIXTURES_NESTEDSUBCOMMANDFIXTURE_H
