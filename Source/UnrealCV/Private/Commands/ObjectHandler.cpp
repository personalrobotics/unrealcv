#include "UnrealCVPrivate.h"
#include "ObjectHandler.h"
#include "ObjectPainter.h"


FExecStatus GetObjectMobility(const TArray<FString>& Args);

void FObjectCommandHandler::RegisterCommands()
{
	FDispatcherDelegate Cmd;
	FString Help;

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjects);
	Help = "Get the name of all objects";
	CommandDispatcher->BindCommand(TEXT("vget /objects"), Cmd, Help);

	// The order matters
	// Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::CurrentObjectHandler); // Redirect to current
	// CommandDispatcher->BindCommand(TEXT("[str] /object/_/[str]"), Cmd, "Get current object");

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectPoses);
	Help = "Get poses of all objects by prefix";
	CommandDispatcher->BindCommand(TEXT("vget /object_poses [str]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectColor);
	Help = "Get the labeling color of an object (used in object instance mask)";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/color"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::SetObjectColor);
	Help = "Set the labeling color of an object [r, g, b]";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/color [uint] [uint] [uint]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectName);
	Help = "[debug] Get the object name";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/name"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectLocation);
	Help = "Get object location [x, y, z]";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/location"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectRotation);
	Help = "Get object rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/rotation"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectPose);
	Help = "Get object pose [x, y, z, pitch, yaw, roll]";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/pose"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::SetObjectPose);
	Help = "Set object pose [x, y, z, pitch, yaw, roll]";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/pose [float] [float] [float] [float] [float] [float]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::SetObjectLocation);
	Help = "Set object location [x, y, z]";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/location [float] [float] [float]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::SetObjectRotation);
	Help = "Set object rotation [pitch, yaw, roll]";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/rotation [float] [float] [float]"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::GetObjectBoundingBox);
	Help = "Get object bounding box [Max.x Max.y Max.z, Min.x Min.y Min.z]";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/boundingbox"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::SetObjectLocation);

	Cmd = FDispatcherDelegate::CreateStatic(GetObjectMobility);
	Help = "Is the object static or movable?";
	CommandDispatcher->BindCommand(TEXT("vget /object/[str]/mobility"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::ShowObject);
	Help = "Show object";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/show"), Cmd, Help);

	Cmd = FDispatcherDelegate::CreateRaw(this, &FObjectCommandHandler::HideObject);
	Help = "Hide object";
	CommandDispatcher->BindCommand(TEXT("vset /object/[str]/hide"), Cmd, Help);
}

FExecStatus FObjectCommandHandler::GetObjects(const TArray<FString>& Args)
{
  TArray<FString> Keys = FObjectPainter::Get().GetObjectList();
	FString Message = "";
	for (auto ActorId : Keys)
	{
		Message += ActorId + " ";
	}
	Message = Message.LeftChop(1);
	return FExecStatus::OK(Message);
}

FExecStatus FObjectCommandHandler::SetObjectColor(const TArray<FString>& Args)
{
	// ObjectName, R, G, B, A = 255
	// The color format is RGBA
	if (Args.Num() == 4)
	{
		FString ObjectName = Args[0];
		uint32 R = FCString::Atoi(*Args[1]), G = FCString::Atoi(*Args[2]), B = FCString::Atoi(*Args[3]), A = 255; // A = FCString::Atoi(*Args[4]);
		FColor NewColor(R, G, B, A);

		return FObjectPainter::Get().SetActorColor(ObjectName, NewColor);
	}

	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectColor(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		return FObjectPainter::Get().GetActorColor(ObjectName);
	}

	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectName(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		return FExecStatus::OK(Args[0]);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::CurrentObjectHandler(const TArray<FString>& Args)
{
	// At least one parameter
	if (Args.Num() >= 2)
	{
		FString Uri = "";
		// Get the name of current object
		FHitResult HitResult;
		// The original version for the shooting game use CameraComponent
		APawn* Pawn = FUE4CVServer::Get().GetPawn();
		FVector StartLocation = Pawn->GetActorLocation();
		// FRotator Direction = GetActorRotation();
		FRotator Direction = Pawn->GetControlRotation();

		FVector EndLocation = StartLocation + Direction.Vector() * 10000;
		FCollisionQueryParams QueryParams;
		QueryParams.AddIgnoredActor(Pawn);

		APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
		if (PlayerController != nullptr)
		{
			FHitResult TraceResult(ForceInit);
			PlayerController->GetHitResultUnderCursor(ECollisionChannel::ECC_WorldDynamic, false, TraceResult);
			FString TraceString;
			if (TraceResult.GetActor() != nullptr)
			{
				TraceString += FString::Printf(TEXT("Trace Actor %s."), *TraceResult.GetActor()->GetName());
			}
			if (TraceResult.GetComponent() != nullptr)
			{
				TraceString += FString::Printf(TEXT("Trace Comp %s."), *TraceResult.GetComponent()->GetName());
			}
			// TheHud->TraceResultText = TraceString;
			// Character->ConsoleOutputDevice->Log(TraceString);
		}
		// TODO: This is not working well.

		if (this->GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Visibility, QueryParams))
		{
			AActor* HitActor = HitResult.GetActor();

			// UE_LOG(LogUnrealCV, Warning, TEXT("%s"), *HitActor->GetActorLabel());
			// Draw a bounding box of the hitted object and also output the name of it.
			FString ActorName = HitActor->GetHumanReadableName();
			FString Method = Args[0], Property = Args[1];
			Uri = FString::Printf(TEXT("%s /object/%s/%s"), *Method, *ActorName, *Property); // Method name

			for (int32 ArgIndex = 2; ArgIndex < Args.Num(); ArgIndex++) // Vargs
			{
				Uri += " " + Args[ArgIndex];
			}
			FExecStatus ExecStatus = CommandDispatcher->Exec(Uri);
			return ExecStatus;
		}
		else
		{
			return FExecStatus::Error("Cannot find current object");
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectPoses(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
    FString ObjectPrefix = Args[0];

    FString RstMsg = "";

    TArray<FString> Keys = FObjectPainter::Get().GetObjectList();
    for (auto ActorId : Keys)
    {
      if (ActorId.Contains(ObjectPrefix))
      {
        AActor* Object = FObjectPainter::Get().GetObject(ActorId);
        if (Object == NULL)
        {
          return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ActorId));
        }

        FVector Location = Object->GetActorLocation();
        FRotator Rotation = Object->GetActorRotation();
        FBox BoundingBox = Object->GetComponentsBoundingBox();

        RstMsg += ActorId + FString::Printf(TEXT(" %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f "),
              Location.X, Location.Y, Location.Z,
              Rotation.Pitch, Rotation.Yaw, Rotation.Roll,
              BoundingBox.Max.X, BoundingBox.Max.Y, BoundingBox.Max.Z,
              BoundingBox.Min.X, BoundingBox.Min.Y, BoundingBox.Min.Z);
      }
    }
    return FExecStatus::OK(RstMsg);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectBoundingBox(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		FBox BoundingBox = Object->GetComponentsBoundingBox();
		return FExecStatus::OK(FString::Printf(TEXT("%.2f %.2f %.2f %.2f %.2f %.2f"),
			BoundingBox.Max.X, BoundingBox.Max.Y, BoundingBox.Max.Z,
			BoundingBox.Min.X, BoundingBox.Min.Y, BoundingBox.Min.Z));
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectPose(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		FVector Location = Object->GetActorLocation();
		FRotator Rotation = Object->GetActorRotation();

		return FExecStatus::OK(FString::Printf(TEXT("%.2f %.2f %.2f %.2f %.2f %.2f"),
          Location.X, Location.Y, Location.Z,
          Rotation.Pitch, Rotation.Yaw, Rotation.Roll));
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectLocation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		FVector Location = Object->GetActorLocation();
		return FExecStatus::OK(FString::Printf(TEXT("%.2f %.2f %.2f"), Location.X, Location.Y, Location.Z));
	}

	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::GetObjectRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		// TODO: support quaternion
		FRotator Rotation = Object->GetActorRotation();
		return FExecStatus::OK(FString::Printf(TEXT("%.2f %.2f %.2f"), Rotation.Pitch, Rotation.Yaw, Rotation.Roll));
	}
	return FExecStatus::InvalidArgument;
}

/** There is no guarantee this will always succeed, for example, hitting a wall */
FExecStatus FObjectCommandHandler::SetObjectPose(const TArray<FString>& Args)
{
	if (Args.Num() == 7)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		// TODO: Check whether this object is movable
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector NewLocation = FVector(X, Y, Z);
		bool Success = Object->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

		float Pitch = FCString::Atof(*Args[4]), Yaw = FCString::Atof(*Args[5]), Roll = FCString::Atof(*Args[6]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);
		Success = Success && Object->SetActorRotation(Rotator);

		if (Success)
		{
			return FExecStatus::OK();
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Failed to move object %s"), *ObjectName));
		}
	}
	return FExecStatus::InvalidArgument;
}

/** There is no guarantee this will always succeed, for example, hitting a wall */
FExecStatus FObjectCommandHandler::SetObjectLocation(const TArray<FString>& Args)
{
	if (Args.Num() == 4)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		// TODO: Check whether this object is movable
		float X = FCString::Atof(*Args[1]), Y = FCString::Atof(*Args[2]), Z = FCString::Atof(*Args[3]);
		FVector NewLocation = FVector(X, Y, Z);
		bool Success = Object->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);

		if (Success)
		{
			return FExecStatus::OK();
		}
		else
		{
			return FExecStatus::Error(FString::Printf(TEXT("Failed to move object %s"), *ObjectName));
		}
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::SetObjectRotation(const TArray<FString>& Args)
{
	if (Args.Num() == 4)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		// TODO: Check whether this object is movable
		float Pitch = FCString::Atof(*Args[1]), Yaw = FCString::Atof(*Args[2]), Roll = FCString::Atof(*Args[3]);
		FRotator Rotator = FRotator(Pitch, Yaw, Roll);
		bool Success = Object->SetActorRotation(Rotator);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus GetObjectMobility(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		FString MobilityName = "";
		EComponentMobility::Type Mobility = Object->GetRootComponent()->Mobility.GetValue();
		switch (Mobility)
		{
		case EComponentMobility::Type::Movable: MobilityName = "Movable"; break;
		case EComponentMobility::Type::Static: MobilityName = "Static"; break;
		case EComponentMobility::Type::Stationary: MobilityName = "Stationary"; break;
		default: MobilityName = "Unknown";
		}
		return FExecStatus::OK(MobilityName);
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::ShowObject(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		Object->SetActorHiddenInGame(false);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}

FExecStatus FObjectCommandHandler::HideObject(const TArray<FString>& Args)
{
	if (Args.Num() == 1)
	{
		FString ObjectName = Args[0];
		AActor* Object = FObjectPainter::Get().GetObject(ObjectName);
		if (Object == NULL)
		{
			return FExecStatus::Error(FString::Printf(TEXT("Cannot find object %s"), *ObjectName));
		}

		Object->SetActorHiddenInGame(true);
		return FExecStatus::OK();
	}
	return FExecStatus::InvalidArgument;
}
