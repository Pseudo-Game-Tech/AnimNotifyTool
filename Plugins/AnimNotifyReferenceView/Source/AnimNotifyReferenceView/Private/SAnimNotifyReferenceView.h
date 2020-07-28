#pragma once
#include "ARFilter.h"
#include "AssetData.h"
#include "ContentBrowserDelegates.h"
#include "Animation/AnimSequenceBase.h"
#include "Engine/EngineTypes.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Views/STableRow.h"
#include "SAnimNotifyReferenceView.generated.h"

static const FName AnimNotifyReferenceViewTabName = FName("动画通知引用查看器");

USTRUCT()
struct FAnimNotifyReferenceView
{
    GENERATED_USTRUCT_BODY()
    
    /* 搜索范围 */
    UPROPERTY(Config, EditAnywhere, Meta = (ContentDir))
    TArray<FDirectoryPath> ScanPaths;
};

struct FAnimNotifyClassInfo
{
    FName ClassName;
    UClass* Class;
    ECheckBoxState CheckBoxState;
    bool bIsAnimNotifyState;

    FAnimNotifyClassInfo(FName ClassName, UClass* Class, bool bIsAnimNotifyState)
        :ClassName(ClassName), Class(Class), CheckBoxState(ECheckBoxState::Unchecked), bIsAnimNotifyState(bIsAnimNotifyState)
    {
    }
};

struct FFilterAnimInfo
{
    UAnimSequenceBase* Object;
    FAssetData AssetData;

    FFilterAnimInfo(UAnimSequenceBase* Object, const FAssetData& AssetData)
        : Object(Object),
          AssetData(AssetData)
    {
    }
};

struct FAnimNotifieInfo
{
    FName Name;
    FString ClassName;
    FName TrackName;
    float TriggerTimeOffset;
        
    FAnimNotifieInfo(const FName& Name, const FString& ClassName, FName TrackName, float TriggerTimeOffset)
        : Name(Name),
          ClassName(ClassName),
          TrackName(TrackName),
          TriggerTimeOffset(TriggerTimeOffset)
    {
    }
};

class SAnimNotifyReferenceView : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAnimNotifyReferenceView)
    { }
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow);
    
    EColumnSortMode::Type ColumnSortMode_NotifyName() const
    {
        return columnSortMode_NotifyName;
    }

    EColumnSortMode::Type ColumnSortMode_TrackName() const
    {
        return columnSortMode_TrackName;
    }

    EColumnSortMode::Type ColumnSortMode_NotifyClass() const
    {
        return columnSortMode_NotifyClass;
    }

    EColumnSortMode::Type ColumnSortMode_Time() const
    {
        return columnSortMode_Time;
    }
private:
    FAnimNotifyReferenceView EditProperties;
    TArray<FFilterAnimInfo> FilterAnimArray;
    TSharedPtr<SWidget> FilterAnimPicker;
    FRefreshAssetViewDelegate RefreshAssetViewDelegate;
    FSetARFilterDelegate SetARFilterDelegate;
    FARFilter FilterAnim;
    FReply OnUpdateFilterAnim();
    void OnAnimSelected(const struct FAssetData& AssetData);

    TArray<TSharedPtr<FAnimNotifyClassInfo>> AnimNotifyClassInfos;
    TSharedPtr<SListView<TSharedPtr<FAnimNotifyClassInfo>>> ListViewWidget_AnimNotifyClass;
    void UpadteAnimNotifyClassInfos();
    TSharedRef< ITableRow > OnGenerateRow_AnimNotifyClass(const TSharedPtr<FAnimNotifyClassInfo> Item, const TSharedRef< STableViewBase >& OwnerTable);

    EColumnSortMode::Type columnSortMode_NotifyName = EColumnSortMode::None;
    EColumnSortMode::Type columnSortMode_TrackName = EColumnSortMode::None;
    EColumnSortMode::Type columnSortMode_NotifyClass = EColumnSortMode::None;
    EColumnSortMode::Type columnSortMode_Time = EColumnSortMode::Ascending;
    TArray<TSharedPtr<FAnimNotifieInfo>> NowSelectedAnimNotifies;
    TSharedPtr<SListView<TSharedPtr<FAnimNotifieInfo>>> ListViewWidget_SelectedAnimNotifies;
    TSharedRef< ITableRow > OnGenerateRow_SelectedAnimNotifies(const TSharedPtr<FAnimNotifieInfo> Item, const TSharedRef< STableViewBase >& OwnerTable);
};
