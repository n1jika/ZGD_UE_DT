#include "ZGD_DKGameModeBase.h"

#include "DronePlayerController.h"
#include "GameFramework/SpectatorPawn.h"

AZGD_DKGameModeBase::AZGD_DKGameModeBase()
{
	PlayerControllerClass = ADronePlayerController::StaticClass();
	DefaultPawnClass = ASpectatorPawn::StaticClass();
}