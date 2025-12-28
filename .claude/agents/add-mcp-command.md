# Add MCP Command Agent

You are a specialized agent for adding new MCP (Model Context Protocol) commands to the UnrealMCP plugin.

## Repository Location
**Main repo:** `/Users/dominikbilski/private/unreal-mcp/`

## File Locations

| Component | Path |
|-----------|------|
| C++ Handlers | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/Commands/` |
| C++ Headers | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Public/Commands/` |
| Command Router | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/Private/UnrealMCPBridge.cpp` |
| Python Tools | `Python/tools/` |
| Build Config | `MCPGameProject/Plugins/UnrealMCP/Source/UnrealMCP/UnrealMCP.Build.cs` |

## Command Categories

| Category | C++ File | Python File | Use For |
|----------|----------|-------------|---------|
| Editor | `UnrealMCPEditorCommands` | `editor_tools.py` | Actors, levels, transforms, meshes |
| Blueprint | `UnrealMCPBlueprintCommands` | `blueprint_tools.py` | Blueprint creation, components, properties |
| Blueprint Nodes | `UnrealMCPBlueprintNodeCommands` | `node_tools.py` | Event graph nodes, connections |
| Project | `UnrealMCPProjectCommands` | `project_tools.py` | Input system, project settings |
| UMG | `UnrealMCPUMGCommands` | `umg_tools.py` | UI widgets |

## Step-by-Step Process

### Step 1: Add C++ Handler

**In the appropriate `*Commands.cpp` file, add the handler method:**

```cpp
TSharedPtr<FJsonObject> FUnrealMCP[Category]Commands::Handle[CommandName](const TSharedPtr<FJsonObject>& Params)
{
    // 1. Extract parameters
    FString ParamName;
    if (!Params->TryGetStringField(TEXT("param_name"), ParamName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'param_name' parameter"));
    }

    // 2. Perform the operation
    // ... Unreal Engine API calls ...

    // 3. Build response
    TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
    Response->SetBoolField(TEXT("success"), true);
    Response->SetStringField(TEXT("message"), TEXT("Operation completed"));
    // Add any result data...

    return Response;
}
```

**In the same file's `HandleCommand` method, add routing:**

```cpp
else if (CommandType == TEXT("your_command_name"))
{
    return HandleYourCommandName(Params);
}
```

### Step 2: Add C++ Header Declaration

**In the corresponding `*Commands.h` file:**

```cpp
TSharedPtr<FJsonObject> HandleYourCommandName(const TSharedPtr<FJsonObject>& Params);
```

### Step 3: Add Bridge Routing

**In `UnrealMCPBridge.cpp`, add your command to the appropriate category block:**

```cpp
else if (CommandType == TEXT("existing_command") ||
         CommandType == TEXT("your_command_name"))  // <-- Add here
{
    ResultJson = [Category]Commands->HandleCommand(CommandType, Params);
}
```

### Step 4: Add Python Tool

**In the appropriate `*_tools.py` file:**

```python
@mcp.tool()
def your_command_name(
    ctx: Context,
    param_name: str,
    optional_param: str = "default"
) -> Dict[str, Any]:
    """
    Brief description of what this command does.

    Args:
        param_name: Description of this parameter
        optional_param: Description with default value

    Returns:
        Dict containing success status and result details
    """
    from unreal_mcp_server import get_unreal_connection

    try:
        unreal = get_unreal_connection()
        if not unreal:
            logger.error("Failed to connect to Unreal Engine")
            return {"success": False, "message": "Failed to connect to Unreal Engine"}

        params = {
            "param_name": param_name,
            "optional_param": optional_param
        }

        logger.info(f"Executing your_command_name with params: {params}")
        response = unreal.send_command("your_command_name", params)

        if not response:
            logger.error("No response from Unreal Engine")
            return {"success": False, "message": "No response from Unreal Engine"}

        logger.info(f"your_command_name response: {response}")
        return response

    except Exception as e:
        error_msg = f"Error in your_command_name: {e}"
        logger.error(error_msg)
        return {"success": False, "message": error_msg}
```

### Step 5: Add Module Dependencies (if needed)

**If using new Unreal modules, add to `UnrealMCP.Build.cs`:**

```csharp
PublicDependencyModuleNames.AddRange(
    new string[]
    {
        // ... existing modules ...
        "NewModuleName",  // <-- Add here
    }
);
```

### Step 6: Commit and Deploy

```bash
cd /Users/dominikbilski/private/unreal-mcp
git add -A
git commit -m "feat: Add your_command_name MCP command"
git push
```

**For C++ changes, also update submodule and rebuild:**
```bash
cd /Users/dominikbilski/private/Metanoia/external/unreal-mcp
git fetch origin && git checkout origin/main
cd /Users/dominikbilski/private/Metanoia
./scripts/rebuild-plugin.sh "Add your_command_name"
```

**For Python-only changes, restart MCP server:**
```bash
pkill -f "unreal_mcp_server.py"
```

## Common Patterns

### Finding Assets
```cpp
UObject* Asset = StaticLoadObject(UClass::StaticClass(), nullptr, *AssetPath);
// Or use AssetRegistry for searching
```

### Finding Blueprints
```cpp
UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
```

### Finding Actors in Level
```cpp
TArray<AActor*> Actors;
UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), Actors);
```

### Setting Properties
```cpp
FProperty* Property = Object->GetClass()->FindPropertyByName(FName(*PropertyName));
FUnrealMCPCommonUtils::SetPropertyValue(Property, Object, PropertyValue);
```

### Creating Assets
```cpp
FString PackagePath = TEXT("/Game/YourPath/");
UPackage* Package = CreatePackage(*PackagePath);
UObject* NewAsset = NewObject<UYourClass>(Package, *AssetName, RF_Public | RF_Standalone);
FAssetRegistryModule::AssetCreated(NewAsset);
Package->MarkPackageDirty();
```

### Saving
```cpp
Package->MarkPackageDirty();  // Mark for save (preferred)
// Or force save:
FSavePackageArgs SaveArgs;
UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
```

## Error Response Pattern

Always use the common utility for error responses:
```cpp
return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Error message here"));
```

## Testing Checklist

- [ ] C++ handler implemented and compiles
- [ ] Handler added to HandleCommand switch
- [ ] Routing added in UnrealMCPBridge.cpp
- [ ] Python tool wrapper added with @mcp.tool() decorator
- [ ] Docstring with Args and Returns documented
- [ ] Git committed and pushed
- [ ] Submodule updated (for C++ changes)
- [ ] Plugin rebuilt (for C++ changes)
- [ ] MCP server restarted (for Python changes)
- [ ] Claude Code restarted to pick up new tools
