#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Light.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Components/LightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FileHelpers.h"
#include "Engine/World.h"
#include "UObject/SavePackage.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    else if (CommandType == TEXT("get_actor_components"))
    {
        return HandleGetActorComponents(Params);
    }
    else if (CommandType == TEXT("set_actor_component_property"))
    {
        return HandleSetActorComponentProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Save commands
    else if (CommandType == TEXT("save_all") || CommandType == TEXT("save_current_level"))
    {
        return HandleSaveAll(Params);
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);
        
        // If mesh_path is provided, set the mesh immediately
        if (NewActor && Params->HasField(TEXT("mesh_path")))
        {
            FString MeshPath;
            if (Params->TryGetStringField(TEXT("mesh_path"), MeshPath))
            {
                UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
                if (Mesh)
                {
                    AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(NewActor);
                    if (MeshActor && MeshActor->GetStaticMeshComponent())
                    {
                        MeshActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
                    }
                }
                else
                {
                    // Clean up the actor we just spawned since mesh loading failed
                    NewActor->Destroy();
                    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load mesh: %s. Common paths: /Engine/BasicShapes/Cube.Cube, /Engine/BasicShapes/Sphere.Sphere"), *MeshPath));
                }
            }
        }
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

    // Special handling for StaticMeshActor - set static mesh
    if (AStaticMeshActor* MeshActor = Cast<AStaticMeshActor>(TargetActor))
    {
        if (PropertyName.Equals(TEXT("StaticMesh"), ESearchCase::IgnoreCase))
        {
            FString MeshPath = PropertyValue->AsString();
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
            if (Mesh)
            {
                UStaticMeshComponent* MeshComponent = MeshActor->GetStaticMeshComponent();
                if (MeshComponent)
                {
                    MeshComponent->SetStaticMesh(Mesh);

                    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                    ResultObj->SetStringField(TEXT("actor"), ActorName);
                    ResultObj->SetStringField(TEXT("property"), PropertyName);
                    ResultObj->SetStringField(TEXT("value"), MeshPath);
                    ResultObj->SetBoolField(TEXT("success"), true);
                    return ResultObj;
                }
                else
                {
                    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("StaticMeshComponent not found on actor"));
                }
            }
            else
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to load mesh: %s. Common paths: /Engine/BasicShapes/Cube.Cube, /Engine/BasicShapes/Sphere.Sphere"), *MeshPath));
            }
        }
    }
    // Also handle StaticMesh property for non-StaticMeshActor (provide helpful error)
    else if (PropertyName.Equals(TEXT("StaticMesh"), ESearchCase::IgnoreCase))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor '%s' is not a StaticMeshActor (class: %s). Cannot set StaticMesh property."), *ActorName, *TargetActor->GetClass()->GetName()));
    }

    // Special handling for light actors - check if we need to access the light component
    ULightComponent* LightComponent = nullptr;
    if (ALight* LightActor = Cast<ALight>(TargetActor))
    {
        LightComponent = LightActor->GetLightComponent();
    }

    // Handle light-specific properties
    if (LightComponent && PropertyName.Equals(TEXT("LightColor"), ESearchCase::IgnoreCase))
    {
        // Parse color from string format "(R=255,G=105,B=180)" or array [255, 105, 180]
        FColor NewColor;
        FString ValueStr = PropertyValue->AsString();

        if (ValueStr.Contains(TEXT("R=")))
        {
            // Parse from "(R=255,G=105,B=180)" format
            int32 R = 255, G = 255, B = 255, A = 255;
            FParse::Value(*ValueStr, TEXT("R="), R);
            FParse::Value(*ValueStr, TEXT("G="), G);
            FParse::Value(*ValueStr, TEXT("B="), B);
            FParse::Value(*ValueStr, TEXT("A="), A);
            NewColor = FColor(R, G, B, A);
        }
        else
        {
            // Try parsing as comma-separated values "255,105,180"
            TArray<FString> Parts;
            ValueStr.ParseIntoArray(Parts, TEXT(","), true);
            if (Parts.Num() >= 3)
            {
                NewColor = FColor(
                    FCString::Atoi(*Parts[0]),
                    FCString::Atoi(*Parts[1]),
                    FCString::Atoi(*Parts[2]),
                    Parts.Num() > 3 ? FCString::Atoi(*Parts[3]) : 255
                );
            }
            else
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Invalid color format. Use 'R,G,B' or '(R=255,G=105,B=180)'"));
            }
        }

        LightComponent->SetLightColor(NewColor);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetStringField(TEXT("value"), FString::Printf(TEXT("R=%d,G=%d,B=%d,A=%d"), NewColor.R, NewColor.G, NewColor.B, NewColor.A));
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    // Handle light intensity
    if (LightComponent && PropertyName.Equals(TEXT("Intensity"), ESearchCase::IgnoreCase))
    {
        float Intensity = FCString::Atof(*PropertyValue->AsString());
        LightComponent->SetIntensity(Intensity);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetNumberField(TEXT("value"), Intensity);
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        
        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorComponents(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get all components
    TArray<TSharedPtr<FJsonValue>> ComponentArray;
    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
            CompObj->SetStringField(TEXT("name"), Component->GetName());
            CompObj->SetStringField(TEXT("class"), Component->GetClass()->GetName());
            
            // Add some common properties for known component types
            if (UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(Component))
            {
                CompObj->SetNumberField(TEXT("MaxWalkSpeed"), MoveComp->MaxWalkSpeed);
                CompObj->SetNumberField(TEXT("MaxWalkSpeedCrouched"), MoveComp->MaxWalkSpeedCrouched);
                CompObj->SetNumberField(TEXT("JumpZVelocity"), MoveComp->JumpZVelocity);
                CompObj->SetNumberField(TEXT("GravityScale"), MoveComp->GravityScale);
            }
            else if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
            {
                CompObj->SetBoolField(TEXT("SimulatePhysics"), PrimComp->IsSimulatingPhysics());
            }
            
            ComponentArray.Add(MakeShared<FJsonValueObject>(CompObj));
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetArrayField(TEXT("components"), ComponentArray);
    ResultObj->SetNumberField(TEXT("component_count"), ComponentArray.Num());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorComponentProperty(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get component name
    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Find the component (by name or class name)
    UActorComponent* TargetComponent = nullptr;
    TArray<UActorComponent*> Components;
    TargetActor->GetComponents(Components);

    for (UActorComponent* Component : Components)
    {
        if (Component)
        {
            // Match by component name or class name (without the U prefix)
            FString CompName = Component->GetName();
            FString ClassName = Component->GetClass()->GetName();
            
            if (CompName.Equals(ComponentName, ESearchCase::IgnoreCase) ||
                ClassName.Equals(ComponentName, ESearchCase::IgnoreCase) ||
                ClassName.Equals(ComponentName + TEXT("Component"), ESearchCase::IgnoreCase) ||
                CompName.Contains(ComponentName))
            {
                TargetComponent = Component;
                break;
            }
        }
    }

    if (!TargetComponent)
    {
        // Build helpful error message listing available components
        FString AvailableComps;
        for (UActorComponent* Component : Components)
        {
            if (Component)
            {
                AvailableComps += FString::Printf(TEXT("\n  - %s (%s)"), *Component->GetName(), *Component->GetClass()->GetName());
            }
        }
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
            TEXT("Component '%s' not found on actor '%s'. Available components:%s"), 
            *ComponentName, *ActorName, *AvailableComps));
    }

    // Special handling for CharacterMovementComponent
    if (UCharacterMovementComponent* MoveComp = Cast<UCharacterMovementComponent>(TargetComponent))
    {
        float FloatValue = FCString::Atof(*PropertyValue->AsString());
        
        if (PropertyName.Equals(TEXT("MaxWalkSpeed"), ESearchCase::IgnoreCase))
        {
            MoveComp->MaxWalkSpeed = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("MaxWalkSpeedCrouched"), ESearchCase::IgnoreCase))
        {
            MoveComp->MaxWalkSpeedCrouched = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("JumpZVelocity"), ESearchCase::IgnoreCase))
        {
            MoveComp->JumpZVelocity = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("GravityScale"), ESearchCase::IgnoreCase))
        {
            MoveComp->GravityScale = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("MaxAcceleration"), ESearchCase::IgnoreCase))
        {
            MoveComp->MaxAcceleration = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("BrakingDecelerationWalking"), ESearchCase::IgnoreCase))
        {
            MoveComp->BrakingDecelerationWalking = FloatValue;
        }
        else if (PropertyName.Equals(TEXT("GroundFriction"), ESearchCase::IgnoreCase))
        {
            MoveComp->GroundFriction = FloatValue;
        }
        else
        {
            // Try generic property setting
            FString ErrorMessage;
            if (!FUnrealMCPCommonUtils::SetObjectProperty(MoveComp, PropertyName, PropertyValue, ErrorMessage))
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
            }
        }

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("component"), TargetComponent->GetName());
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetStringField(TEXT("value"), PropertyValue->AsString());
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    // Special handling for mesh component materials
    if (UMeshComponent* MeshComp = Cast<UMeshComponent>(TargetComponent))
    {
        if (PropertyName.Equals(TEXT("Material"), ESearchCase::IgnoreCase) ||
            PropertyName.Equals(TEXT("OverrideMaterial"), ESearchCase::IgnoreCase))
        {
            FString MaterialPath = PropertyValue->AsString();
            UMaterialInterface* Material = nullptr;

            // Check if it's a color specification like "Color:1,0,0" for red
            if (MaterialPath.StartsWith(TEXT("Color:")))
            {
                FString ColorStr = MaterialPath.RightChop(6); // Remove "Color:"
                TArray<FString> ColorComponents;
                ColorStr.ParseIntoArray(ColorComponents, TEXT(","));

                if (ColorComponents.Num() >= 3)
                {
                    float R = FCString::Atof(*ColorComponents[0]);
                    float G = FCString::Atof(*ColorComponents[1]);
                    float B = FCString::Atof(*ColorComponents[2]);

                    // Create a dynamic material instance with the specified color
                    UMaterial* BaseMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
                    if (BaseMaterial)
                    {
                        UMaterialInstanceDynamic* DynMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, MeshComp);
                        DynMaterial->SetVectorParameterValue(TEXT("BaseColor"), FLinearColor(R, G, B));
                        Material = DynMaterial;
                    }
                }
            }
            else
            {
                Material = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
            }

            if (!Material)
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(
                    TEXT("Failed to load material: %s"), *MaterialPath));
            }

            // Get material index (default to 0, or parse from property name like "Material_1")
            int32 MaterialIndex = 0;
            if (Params->HasField(TEXT("material_index")))
            {
                MaterialIndex = static_cast<int32>(Params->GetNumberField(TEXT("material_index")));
            }

            // Mark for undo/redo and modification tracking
            MeshComp->Modify();
            TargetActor->Modify();

            // For StaticMeshComponent, set override materials properly for editor
            if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(MeshComp))
            {
                // Ensure the OverrideMaterials array is large enough
                int32 NumMats = StaticMeshComp->GetNumMaterials();
                if (MaterialIndex < 0)
                {
                    // Apply to all slots
                    for (int32 i = 0; i < NumMats; i++)
                    {
                        StaticMeshComp->SetMaterial(i, Material);
                    }
                }
                else
                {
                    StaticMeshComp->SetMaterial(MaterialIndex, Material);
                }
            }
            else
            {
                // For other mesh types, use SetMaterial directly
                if (MaterialIndex < 0)
                {
                    int32 NumMaterials = MeshComp->GetNumMaterials();
                    for (int32 i = 0; i < NumMaterials; i++)
                    {
                        MeshComp->SetMaterial(i, Material);
                    }
                }
                else
                {
                    MeshComp->SetMaterial(MaterialIndex, Material);
                }
            }

            // Get the OverrideMaterials property for proper notification
            FProperty* OverrideMaterialsProp = MeshComp->GetClass()->FindPropertyByName(TEXT("OverrideMaterials"));

            // Notify editor of property change with the specific property
            if (OverrideMaterialsProp)
            {
                FPropertyChangedEvent PropertyChangedEvent(OverrideMaterialsProp);
                MeshComp->PostEditChangeProperty(PropertyChangedEvent);
            }
            else
            {
                FPropertyChangedEvent PropertyChangedEvent(nullptr);
                MeshComp->PostEditChangeProperty(PropertyChangedEvent);
            }

            // Force visual update
            MeshComp->MarkRenderStateDirty();
            MeshComp->RecreateRenderState_Concurrent();
            TargetActor->MarkPackageDirty();

            // Request viewport redraw
            if (GEditor)
            {
                GEditor->RedrawAllViewports();
            }

            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetStringField(TEXT("actor"), ActorName);
            ResultObj->SetStringField(TEXT("component"), TargetComponent->GetName());
            ResultObj->SetStringField(TEXT("property"), PropertyName);
            ResultObj->SetStringField(TEXT("material"), MaterialPath);
            ResultObj->SetNumberField(TEXT("material_index"), MaterialIndex);
            ResultObj->SetBoolField(TEXT("success"), true);
            return ResultObj;
        }
    }

    // Generic property setting for other component types
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetComponent, PropertyName, PropertyValue, ErrorMessage))
    {
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("component"), TargetComponent->GetName());
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString Root      = TEXT("/Game/Blueprints/");
    FString AssetPath = Root + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found – it must reside under /Game/Blueprints"), *BlueprintName));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'filepath' parameter"));
    }
    
    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray64<uint8> CompressedBitmap;
            FImageUtils::PNGCompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);
            
            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
            {
                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("filepath"), FilePath);
                return ResultObj;
            }
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to take screenshot"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSaveAll(const TSharedPtr<FJsonObject>& Params)
{
    // Get the current world
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No world loaded"));
    }

    // Track what we saved
    TArray<FString> SavedItems;
    bool bSuccess = true;

    // Save the current level using FEditorFileUtils
    ULevel* CurrentLevel = World->GetCurrentLevel();
    if (CurrentLevel)
    {
        UPackage* LevelPackage = CurrentLevel->GetOutermost();
        if (LevelPackage && LevelPackage->IsDirty())
        {
            FString PackageFileName = FPackageName::LongPackageNameToFilename(LevelPackage->GetName(), FPackageName::GetMapPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Standalone;
            if (UPackage::SavePackage(LevelPackage, World, *PackageFileName, SaveArgs))
            {
                SavedItems.Add(FString::Printf(TEXT("Level: %s"), *LevelPackage->GetName()));
            }
            else
            {
                bSuccess = false;
            }
        }
    }

    // Save all dirty packages (assets, blueprints, etc.)
    TArray<UPackage*> PackagesToSave;
    FEditorFileUtils::GetDirtyContentPackages(PackagesToSave);
    FEditorFileUtils::GetDirtyWorldPackages(PackagesToSave);

    for (UPackage* Package : PackagesToSave)
    {
        if (Package && Package->IsDirty())
        {
            FString PackageFileName;
            if (FPackageName::TryConvertLongPackageNameToFilename(Package->GetName(), PackageFileName, FPackageName::GetAssetPackageExtension()))
            {
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Standalone;
                if (UPackage::SavePackage(Package, nullptr, *PackageFileName, SaveArgs))
                {
                    SavedItems.Add(FString::Printf(TEXT("Package: %s"), *Package->GetName()));
                }
            }
        }
    }

    // Build response
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), bSuccess);
    ResultObj->SetNumberField(TEXT("saved_count"), SavedItems.Num());

    TArray<TSharedPtr<FJsonValue>> SavedArray;
    for (const FString& Item : SavedItems)
    {
        SavedArray.Add(MakeShared<FJsonValueString>(Item));
    }
    ResultObj->SetArrayField(TEXT("saved_items"), SavedArray);

    if (SavedItems.Num() == 0)
    {
        ResultObj->SetStringField(TEXT("message"), TEXT("No dirty packages to save"));
    }
    else
    {
        ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Saved %d item(s)"), SavedItems.Num()));
    }

    return ResultObj;
}