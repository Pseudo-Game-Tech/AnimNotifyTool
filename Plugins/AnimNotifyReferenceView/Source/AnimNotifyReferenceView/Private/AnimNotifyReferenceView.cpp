#include "AnimNotifyReferenceView.h"

#include "EditorStyleSet.h"
#include "SAnimNotifyReferenceView.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

#define LOCTEXT_NAMESPACE "FAnimNotifyReferenceViewModule"

void FAnimNotifyReferenceViewModule::StartupModule()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(AnimNotifyReferenceViewTabName, FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args)
    {
        const TSharedRef<SDockTab> Tab = SNew(SDockTab).TabRole(ETabRole::NomadTab);
        TSharedPtr<SWidget> TabContent= SNew(SAnimNotifyReferenceView, Tab, Args.GetOwnerWindow());

        Tab->SetContent(TabContent.ToSharedRef());

        return Tab;
    }))
        .SetDisplayName(NSLOCTEXT("SAnimNotifyReferenceView", "TabTitle", "动画通知引用查看器"))
        .SetTooltipText(NSLOCTEXT("SAnimNotifyReferenceView", "TooltipText", "打开动画通知引用查看器"))
        .SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
        .SetIcon(FSlateIcon(FEditorStyle::GetStyleSetName(), "DebugTools.TabIcon"));
}

void FAnimNotifyReferenceViewModule::ShutdownModule()
{
    
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAnimNotifyReferenceViewModule, AnimNotifyReferenceView)