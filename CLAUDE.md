# UnrealMCP - AI Assistant Instructions

## This is THE Main Repository

**All edits to MCP plugin code (C++ and Python) should be made HERE.**

```
/Users/dominikbilski/private/unreal-mcp/    ← YOU ARE HERE
├── Python/                                  ← MCP Python server
│   ├── unreal_mcp_server.py                ← Main server entry point
│   └── tools/                              ← Tool implementations
│       ├── editor_tools.py                 ← Actor/level operations
│       ├── blueprint_tools.py              ← Blueprint operations
│       ├── project_tools.py                ← Enhanced Input, project settings
│       └── ...
│
└── MCPGameProject/Plugins/UnrealMCP/Source/ ← C++ Plugin
    ├── Private/Commands/                    ← Command handlers
    │   ├── UnrealMCPEditorCommands.cpp
    │   ├── UnrealMCPBlueprintCommands.cpp
    │   ├── UnrealMCPProjectCommands.cpp
    │   └── ...
    ├── Private/UnrealMCPBridge.cpp         ← Command routing
    └── Public/Commands/                     ← Headers
```

## MCP Server runs from HERE

The MCP server is configured in `~/.mcp.json` to run from this directory:
```json
{
  "mcpServers": {
    "unrealMCP": {
      "command": "uv",
      "args": ["run", "--directory", "/Users/dominikbilski/private/unreal-mcp/Python", "python", "unreal_mcp_server.py"]
    }
  }
}
```

## Workflow

### Python changes (MCP tools)
1. Edit files in `Python/tools/`
2. Commit and push: `git add -A && git commit -m "feat: ..." && git push`
3. Restart MCP server: `pkill -f "unreal_mcp_server.py"`

### C++ changes (Plugin)
1. Edit files in `MCPGameProject/Plugins/UnrealMCP/Source/`
2. Commit and push: `git add -A && git commit -m "feat: ..." && git push`
3. Update submodule in Metanoia: `cd /Users/dominikbilski/private/Metanoia/external/unreal-mcp && git fetch && git checkout origin/main`
4. Rebuild plugin: `cd /Users/dominikbilski/private/Metanoia && ./scripts/rebuild-plugin.sh`

## Relationship with Metanoia

Metanoia uses this repo as a **git submodule** at `Metanoia/external/unreal-mcp/`.

**DO NOT edit files in the submodule** - always edit here in the main repo, then update the submodule reference.

## Adding New MCP Commands

### 1. Add C++ handler
- Create handler in appropriate `*Commands.cpp` file
- Add routing in `UnrealMCPBridge.cpp`
- Add declaration in header file

### 2. Add Python tool
- Add `@mcp.tool()` decorated function in appropriate `*_tools.py` file
- Function calls `unreal.send_command("command_name", params)`

### 3. Test
- Rebuild plugin (for C++ changes)
- Restart MCP server (for Python changes)
- Restart Claude Code to pick up new tools
