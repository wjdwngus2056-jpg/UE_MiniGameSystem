#include "MiniGameSystem.h"
#include "Tags/MiniGameNativeTags.h"

#define LOCTEXT_NAMESPACE "FMiniGameSystemModule"

void FMiniGameSystemModule::StartupModule()
{
	FMiniGameNativeTags::InitializeNativeTags();
}

void FMiniGameSystemModule::ShutdownModule()
{
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMiniGameSystemModule, MiniGameSystem)
