#pragma once

#include "CoreMinimal.h"
#include "Json.h"

/**
 * Handler class for Project-wide MCP commands
 */
class UNREALMCP_API FUnrealMCPProjectCommands
{
public:
    FUnrealMCPProjectCommands();

    // Handle project commands
    TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

private:
    // Legacy input mapping (deprecated)
    TSharedPtr<FJsonObject> HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params);

    // Enhanced Input System handlers
    TSharedPtr<FJsonObject> HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetInputActions(const TSharedPtr<FJsonObject>& Params);
    TSharedPtr<FJsonObject> HandleGetInputMappingContexts(const TSharedPtr<FJsonObject>& Params);
}; 