#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/InputSettings.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"

FUnrealMCPProjectCommands::FUnrealMCPProjectCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("create_input_mapping"))
    {
        return HandleCreateInputMapping(Params);
    }
    else if (CommandType == TEXT("create_input_action"))
    {
        return HandleCreateInputAction(Params);
    }
    else if (CommandType == TEXT("create_input_mapping_context"))
    {
        return HandleCreateInputMappingContext(Params);
    }
    else if (CommandType == TEXT("add_mapping_to_context"))
    {
        return HandleAddMappingToContext(Params);
    }
    else if (CommandType == TEXT("remove_mapping_from_context"))
    {
        return HandleRemoveMappingFromContext(Params);
    }
    else if (CommandType == TEXT("get_input_actions"))
    {
        return HandleGetInputActions(Params);
    }
    else if (CommandType == TEXT("get_input_mapping_contexts"))
    {
        return HandleGetInputMappingContexts(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown project command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMapping(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString Key;
    if (!Params->TryGetStringField(TEXT("key"), Key))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Get the input settings
    UInputSettings* InputSettings = GetMutableDefault<UInputSettings>();
    if (!InputSettings)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get input settings"));
    }

    // Create the input action mapping
    FInputActionKeyMapping ActionMapping;
    ActionMapping.ActionName = FName(*ActionName);
    ActionMapping.Key = FKey(*Key);

    // Add modifiers if provided
    if (Params->HasField(TEXT("shift")))
    {
        ActionMapping.bShift = Params->GetBoolField(TEXT("shift"));
    }
    if (Params->HasField(TEXT("ctrl")))
    {
        ActionMapping.bCtrl = Params->GetBoolField(TEXT("ctrl"));
    }
    if (Params->HasField(TEXT("alt")))
    {
        ActionMapping.bAlt = Params->GetBoolField(TEXT("alt"));
    }
    if (Params->HasField(TEXT("cmd")))
    {
        ActionMapping.bCmd = Params->GetBoolField(TEXT("cmd"));
    }

    // Add the mapping
    InputSettings->AddActionMapping(ActionMapping);
    InputSettings->SaveConfig();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("action_name"), ActionName);
    ResultObj->SetStringField(TEXT("key"), Key);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputAction(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional path (default to /Game/Input/Actions)
    FString Path = TEXT("/Game/Input/Actions");
    Params->TryGetStringField(TEXT("path"), Path);

    // Get optional value type (default to Digital/Boolean)
    FString ValueTypeStr = TEXT("Digital");
    Params->TryGetStringField(TEXT("value_type"), ValueTypeStr);

    // Create package path
    FString PackagePath = Path / Name;

    // Check if asset already exists
    if (FPackageName::DoesPackageExist(PackagePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputAction '%s' already exists at %s"), *Name, *PackagePath));
    }

    // Create package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    // Create the InputAction
    UInputAction* NewAction = NewObject<UInputAction>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
    if (!NewAction)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create InputAction"));
    }

    // Set value type
    if (ValueTypeStr.Equals(TEXT("Axis1D"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis1D;
    }
    else if (ValueTypeStr.Equals(TEXT("Axis2D"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis2D;
    }
    else if (ValueTypeStr.Equals(TEXT("Axis3D"), ESearchCase::IgnoreCase))
    {
        NewAction->ValueType = EInputActionValueType::Axis3D;
    }
    else
    {
        NewAction->ValueType = EInputActionValueType::Boolean;
    }

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewAction);
    Package->MarkPackageDirty();

    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, NewAction, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSaved);
    ResultObj->SetStringField(TEXT("name"), Name);
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    ResultObj->SetStringField(TEXT("value_type"), ValueTypeStr);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleCreateInputMappingContext(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString Name;
    if (!Params->TryGetStringField(TEXT("name"), Name))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional path (default to /Game/Input)
    FString Path = TEXT("/Game/Input");
    Params->TryGetStringField(TEXT("path"), Path);

    // Create package path
    FString PackagePath = Path / Name;

    // Check if asset already exists
    if (FPackageName::DoesPackageExist(PackagePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputMappingContext '%s' already exists at %s"), *Name, *PackagePath));
    }

    // Create package
    UPackage* Package = CreatePackage(*PackagePath);
    if (!Package)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create package"));
    }

    // Create the InputMappingContext
    UInputMappingContext* NewIMC = NewObject<UInputMappingContext>(Package, *Name, RF_Public | RF_Standalone | RF_Transactional);
    if (!NewIMC)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create InputMappingContext"));
    }

    // Notify asset registry
    FAssetRegistryModule::AssetCreated(NewIMC);
    Package->MarkPackageDirty();

    // Save the package
    FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    bool bSaved = UPackage::SavePackage(Package, NewIMC, *PackageFileName, SaveArgs);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSaved);
    ResultObj->SetStringField(TEXT("name"), Name);
    ResultObj->SetStringField(TEXT("path"), PackagePath);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleAddMappingToContext(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ContextName;
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString KeyName;
    if (!Params->TryGetStringField(TEXT("key"), KeyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Find the InputMappingContext
    UInputMappingContext* IMC = nullptr;

    // Try direct path first
    if (ContextName.StartsWith(TEXT("/")))
    {
        IMC = LoadObject<UInputMappingContext>(nullptr, *ContextName);
    }

    // Try common paths if not found
    if (!IMC)
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/Input/%s.%s"), *ContextName, *ContextName),
            FString::Printf(TEXT("/Game/TopDown/Input/%s.%s"), *ContextName, *ContextName),
            FString::Printf(TEXT("/Game/%s.%s"), *ContextName, *ContextName)
        };

        for (const FString& SearchPath : SearchPaths)
        {
            IMC = LoadObject<UInputMappingContext>(nullptr, *SearchPath);
            if (IMC) break;
        }
    }

    if (!IMC)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputMappingContext not found: %s"), *ContextName));
    }

    // Find the InputAction
    UInputAction* Action = nullptr;

    // Try direct path first
    if (ActionName.StartsWith(TEXT("/")))
    {
        Action = LoadObject<UInputAction>(nullptr, *ActionName);
    }

    // Try common paths if not found
    if (!Action)
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/Input/Actions/%s.%s"), *ActionName, *ActionName),
            FString::Printf(TEXT("/Game/TopDown/Input/Actions/%s.%s"), *ActionName, *ActionName),
            FString::Printf(TEXT("/Game/%s.%s"), *ActionName, *ActionName)
        };

        for (const FString& SearchPath : SearchPaths)
        {
            Action = LoadObject<UInputAction>(nullptr, *SearchPath);
            if (Action) break;
        }
    }

    if (!Action)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputAction not found: %s"), *ActionName));
    }

    // Add the mapping
    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *KeyName));
    }

    IMC->MapKey(Action, Key);

    // Mark as dirty and save
    IMC->MarkPackageDirty();

    // Save the IMC
    UPackage* Package = IMC->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, IMC, *PackageFileName, SaveArgs);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context"), IMC->GetName());
    ResultObj->SetStringField(TEXT("action"), Action->GetName());
    ResultObj->SetStringField(TEXT("key"), KeyName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleRemoveMappingFromContext(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ContextName;
    if (!Params->TryGetStringField(TEXT("context_name"), ContextName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'context_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    FString KeyName;
    if (!Params->TryGetStringField(TEXT("key"), KeyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'key' parameter"));
    }

    // Find the InputMappingContext
    UInputMappingContext* IMC = nullptr;

    // Try direct path first
    if (ContextName.StartsWith(TEXT("/")))
    {
        IMC = LoadObject<UInputMappingContext>(nullptr, *ContextName);
    }

    // Try common paths if not found
    if (!IMC)
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/Input/%s.%s"), *ContextName, *ContextName),
            FString::Printf(TEXT("/Game/TopDown/Input/%s.%s"), *ContextName, *ContextName),
            FString::Printf(TEXT("/Game/%s.%s"), *ContextName, *ContextName)
        };

        for (const FString& SearchPath : SearchPaths)
        {
            IMC = LoadObject<UInputMappingContext>(nullptr, *SearchPath);
            if (IMC) break;
        }
    }

    if (!IMC)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputMappingContext not found: %s"), *ContextName));
    }

    // Find the InputAction
    UInputAction* Action = nullptr;

    // Try direct path first
    if (ActionName.StartsWith(TEXT("/")))
    {
        Action = LoadObject<UInputAction>(nullptr, *ActionName);
    }

    // Try common paths if not found
    if (!Action)
    {
        TArray<FString> SearchPaths = {
            FString::Printf(TEXT("/Game/Input/Actions/%s.%s"), *ActionName, *ActionName),
            FString::Printf(TEXT("/Game/TopDown/Input/Actions/%s.%s"), *ActionName, *ActionName),
            FString::Printf(TEXT("/Game/%s.%s"), *ActionName, *ActionName)
        };

        for (const FString& SearchPath : SearchPaths)
        {
            Action = LoadObject<UInputAction>(nullptr, *SearchPath);
            if (Action) break;
        }
    }

    if (!Action)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("InputAction not found: %s"), *ActionName));
    }

    // Validate the key
    FKey Key(*KeyName);
    if (!Key.IsValid())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid key: %s"), *KeyName));
    }

    // Find and remove the mapping
    const TArray<FEnhancedActionKeyMapping>& Mappings = IMC->GetMappings();
    bool bFound = false;
    int32 MappingIndexToRemove = -1;

    for (int32 i = 0; i < Mappings.Num(); ++i)
    {
        const FEnhancedActionKeyMapping& Mapping = Mappings[i];
        if (Mapping.Action == Action && Mapping.Key == Key)
        {
            MappingIndexToRemove = i;
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Mapping not found: %s -> %s in %s"), *ActionName, *KeyName, *ContextName));
    }

    // Remove the mapping using UnmapKey
    IMC->UnmapKey(Action, Key);

    // Mark as dirty and save
    IMC->MarkPackageDirty();

    // Save the IMC
    UPackage* Package = IMC->GetOutermost();
    if (Package)
    {
        FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, IMC, *PackageFileName, SaveArgs);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("context"), IMC->GetName());
    ResultObj->SetStringField(TEXT("action"), Action->GetName());
    ResultObj->SetStringField(TEXT("key"), KeyName);
    ResultObj->SetStringField(TEXT("message"), TEXT("Mapping removed successfully"));
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleGetInputActions(const TSharedPtr<FJsonObject>& Params)
{
    // Get optional path filter
    FString PathFilter = TEXT("/Game");
    Params->TryGetStringField(TEXT("path"), PathFilter);

    // Query asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssetsByClass(UInputAction::StaticClass()->GetClassPathName(), AssetList);

    TArray<TSharedPtr<FJsonValue>> ActionsArray;
    for (const FAssetData& Asset : AssetList)
    {
        FString PackagePath = Asset.PackageName.ToString();
        if (PackagePath.StartsWith(PathFilter))
        {
            TSharedPtr<FJsonObject> ActionObj = MakeShared<FJsonObject>();
            ActionObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
            ActionObj->SetStringField(TEXT("path"), PackagePath);
            ActionsArray.Add(MakeShared<FJsonValueObject>(ActionObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("input_actions"), ActionsArray);
    ResultObj->SetNumberField(TEXT("count"), ActionsArray.Num());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPProjectCommands::HandleGetInputMappingContexts(const TSharedPtr<FJsonObject>& Params)
{
    // Get optional path filter
    FString PathFilter = TEXT("/Game");
    Params->TryGetStringField(TEXT("path"), PathFilter);

    // Query asset registry
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FAssetData> AssetList;
    AssetRegistry.GetAssetsByClass(UInputMappingContext::StaticClass()->GetClassPathName(), AssetList);

    TArray<TSharedPtr<FJsonValue>> ContextsArray;
    for (const FAssetData& Asset : AssetList)
    {
        FString PackagePath = Asset.PackageName.ToString();
        if (PackagePath.StartsWith(PathFilter))
        {
            TSharedPtr<FJsonObject> ContextObj = MakeShared<FJsonObject>();
            ContextObj->SetStringField(TEXT("name"), Asset.AssetName.ToString());
            ContextObj->SetStringField(TEXT("path"), PackagePath);

            // Load the IMC to get mapping details
            UInputMappingContext* IMC = LoadObject<UInputMappingContext>(nullptr, *FString::Printf(TEXT("%s.%s"), *PackagePath, *Asset.AssetName.ToString()));
            if (IMC)
            {
                TArray<TSharedPtr<FJsonValue>> MappingsArray;
                const TArray<FEnhancedActionKeyMapping>& Mappings = IMC->GetMappings();
                for (const FEnhancedActionKeyMapping& Mapping : Mappings)
                {
                    if (Mapping.Action)
                    {
                        TSharedPtr<FJsonObject> MappingObj = MakeShared<FJsonObject>();
                        MappingObj->SetStringField(TEXT("action"), Mapping.Action->GetName());
                        MappingObj->SetStringField(TEXT("key"), Mapping.Key.GetFName().ToString());
                        MappingsArray.Add(MakeShared<FJsonValueObject>(MappingObj));
                    }
                }
                ContextObj->SetArrayField(TEXT("mappings"), MappingsArray);
            }

            ContextsArray.Add(MakeShared<FJsonValueObject>(ContextObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("input_mapping_contexts"), ContextsArray);
    ResultObj->SetNumberField(TEXT("count"), ContextsArray.Num());
    return ResultObj;
} 