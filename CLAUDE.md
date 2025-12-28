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

**Use the custom agent:** `.claude/agents/add-mcp-command.md`

This agent knows exactly how to add new MCP commands. Invoke it by saying:

```
Use the add-mcp-command agent to add a command called "your_command" that does X
```

The agent provides:
- Step-by-step instructions for C++ handlers, headers, routing, and Python tools
- Code templates for each component
- Common UE5 patterns (finding assets, blueprints, actors, setting properties)
- Complete deployment workflow
- Testing checklist

### Quick Reference (if not using the agent)

1. **C++ handler** in `Private/Commands/*Commands.cpp`
2. **Header declaration** in `Public/Commands/*Commands.h`
3. **Routing** in `Private/UnrealMCPBridge.cpp`
4. **Python tool** in `Python/tools/*_tools.py`
5. **Commit, push, update submodule, rebuild**
