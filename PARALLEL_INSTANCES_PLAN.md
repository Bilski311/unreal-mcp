# Parallel MCP Instances Implementation Plan

## Current Architecture

```
Claude Code Session
    ↓ (spawns via ~/.mcp.json)
Python MCP Server (stdio transport)
    ↓ (TCP socket to port 55557 - HARDCODED)
UE Plugin (listens on port 55557 - HARDCODED)
    ↓
Unreal Editor
```

**Problem:** All instances share the same port, so only one editor can work at a time.

## Target Architecture

```
Worktree A (.mcp.json UE_PORT=55557)     Worktree B (.mcp.json UE_PORT=55558)
    ↓                                         ↓
Python Server → port 55557               Python Server → port 55558
    ↓                                         ↓
UE Plugin A (Config: port 55557)         UE Plugin B (Config: port 55558)
    ↓                                         ↓
Editor A (/Metanoia-combat)              Editor B (/Metanoia-ui)
```

---

## Implementation Steps

### Step 1: Python Server - Environment Variable Support

**File:** `/Python/unreal_mcp_server.py`

**Current (line 28):**
```python
UNREAL_PORT = 55557
```

**Change to:**
```python
UNREAL_PORT = int(os.environ.get('MCP_UE_PORT', '55557'))
```

**Also add logging:**
```python
logger.info(f"MCP Server configured to connect to Unreal on port {UNREAL_PORT}")
```

**Complexity:** Low (2 lines changed)

---

### Step 2: UE Plugin - Config File Support

**File:** `/MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp`

**Current (lines 63, 92):**
```cpp
#define MCP_SERVER_PORT 55557
// ...
Port = MCP_SERVER_PORT;
```

**Change to:** Read from project's `DefaultGame.ini`

**New code in `Initialize()`:**
```cpp
// Read port from config, default to 55557
Port = 55557;
if (GConfig)
{
    int32 ConfigPort;
    if (GConfig->GetInt(TEXT("/Script/UnrealMCP.MCPSettings"), TEXT("ServerPort"), ConfigPort, GGameIni))
    {
        Port = static_cast<uint16>(ConfigPort);
    }
}
UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Using port %d"), Port);
```

**Config file (`Config/DefaultGame.ini`):**
```ini
[/Script/UnrealMCP.MCPSettings]
ServerPort=55557
```

**Alternative:** Read from environment variable (simpler but less UE-idiomatic):
```cpp
Port = 55557;
FString EnvPort = FPlatformMisc::GetEnvironmentVariable(TEXT("MCP_UE_PORT"));
if (!EnvPort.IsEmpty())
{
    Port = FCString::Atoi(*EnvPort);
}
```

**Complexity:** Medium (10-15 lines, need to handle config reading)

---

### Step 3: Per-Worktree `.mcp.json`

**File:** `<worktree_root>/.mcp.json`

**Example for Worktree A (port 55557):**
```json
{
  "mcpServers": {
    "unrealMCP": {
      "type": "stdio",
      "command": "/Users/dominikbilski/.local/bin/uv",
      "args": [
        "run",
        "--directory",
        "/Users/dominikbilski/private/Metanoia-combat/external/unreal-mcp/Python",
        "python",
        "unreal_mcp_server.py"
      ],
      "env": {
        "MCP_UE_PORT": "55557"
      }
    }
  }
}
```

**Example for Worktree B (port 55558):**
```json
{
  "mcpServers": {
    "unrealMCP": {
      "type": "stdio",
      "command": "/Users/dominikbilski/.local/bin/uv",
      "args": [
        "run",
        "--directory",
        "/Users/dominikbilski/private/Metanoia-ui/external/unreal-mcp/Python",
        "python",
        "unreal_mcp_server.py"
      ],
      "env": {
        "MCP_UE_PORT": "55558"
      }
    }
  }
}
```

**Complexity:** Low (just config files)

---

### Step 4: Worktree Setup Script

**File:** `/setup-parallel-worktrees.sh`

```bash
#!/bin/bash
# Creates parallel worktrees for Metanoia development

BASE_DIR="/Users/dominikbilski/private"
MAIN_REPO="$BASE_DIR/Metanoia"

# Define worktrees
declare -A WORKTREES=(
    ["Metanoia-combat"]="feature/combat:55557"
    ["Metanoia-ui"]="feature/ui:55558"
    ["Metanoia-ai"]="feature/ai:55559"
)

cd "$MAIN_REPO"

for name in "${!WORKTREES[@]}"; do
    IFS=':' read -r branch port <<< "${WORKTREES[$name]}"
    worktree_path="$BASE_DIR/$name"

    echo "Creating worktree: $name (branch: $branch, port: $port)"

    # Create worktree
    git worktree add "$worktree_path" -b "$branch" 2>/dev/null || \
        git worktree add "$worktree_path" "$branch"

    # Create per-worktree .mcp.json
    cat > "$worktree_path/.mcp.json" << EOF
{
  "mcpServers": {
    "unrealMCP": {
      "type": "stdio",
      "command": "/Users/dominikbilski/.local/bin/uv",
      "args": [
        "run",
        "--directory",
        "$worktree_path/external/unreal-mcp/Python",
        "python",
        "unreal_mcp_server.py"
      ],
      "env": {
        "MCP_UE_PORT": "$port"
      }
    }
  }
}
EOF

    # Create DefaultGame.ini with port config (if not exists)
    mkdir -p "$worktree_path/Config"
    if ! grep -q "ServerPort" "$worktree_path/Config/DefaultGame.ini" 2>/dev/null; then
        echo "" >> "$worktree_path/Config/DefaultGame.ini"
        echo "[/Script/UnrealMCP.MCPSettings]" >> "$worktree_path/Config/DefaultGame.ini"
        echo "ServerPort=$port" >> "$worktree_path/Config/DefaultGame.ini"
    fi

    echo "  Created: $worktree_path with port $port"
done

echo ""
echo "Worktrees created! To start parallel development:"
echo "  Terminal 1: cd $BASE_DIR/Metanoia-combat && open Metanoia.uproject && claude"
echo "  Terminal 2: cd $BASE_DIR/Metanoia-ui && open Metanoia.uproject && claude"
echo "  Terminal 3: cd $BASE_DIR/Metanoia-ai && open Metanoia.uproject && claude"
```

---

## Testing Plan

1. **Single Instance Test:**
   - Modify Python to read env var
   - Modify UE plugin to read config
   - Test with default port 55557
   - Verify MCP still works

2. **Dual Instance Test:**
   - Create one worktree with port 55558
   - Start two editors (main on 55557, worktree on 55558)
   - Start two Claude sessions
   - Verify each connects to correct editor

3. **Memory Test:**
   - Monitor RAM usage with 2-3 editors open
   - Ensure system stays responsive

---

## File Changes Summary

| File | Change | Complexity |
|------|--------|------------|
| `Python/unreal_mcp_server.py` | Add env var for port | Low |
| `UnrealMCPBridge.cpp` | Read port from config | Medium |
| Per-worktree `.mcp.json` | Create for each worktree | Low |
| `Config/DefaultGame.ini` | Add port setting per worktree | Low |
| `setup-parallel-worktrees.sh` | Automation script | Low |

**Total estimated time:** 1-2 hours

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| DDC cache conflicts | Each worktree uses separate Intermediate/Saved folders by default |
| Git merge conflicts | Divide work by system (Combat, UI, AI) to minimize overlap |
| Memory pressure | Monitor with Activity Monitor; close unused editors |
| Port conflicts | Use well-separated ports (55557, 55558, 55559) |

---

## Questions to Resolve

1. **Config approach:** Use `DefaultGame.ini` or environment variable for UE plugin port?
   - Recommendation: `DefaultGame.ini` (more UE-idiomatic, persists in version control)

2. **Submodule handling:** Each worktree has its own copy of `external/unreal-mcp`
   - This is fine - they're all on the same commit unless explicitly changed

3. **Branch strategy:** Feature branches that merge back to main?
   - Recommendation: Yes, with frequent rebasing to avoid drift
