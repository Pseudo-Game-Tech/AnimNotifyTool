#include "SAnimNotifyReferenceView.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "EditorFontGlyphs.h"
#include "EditorStyleSet.h"
#include "IContentBrowserSingleton.h"
#include "IStructureDetailsView.h"
#include "PropertyCustomizationHelpers.h"
#include "SlateOptMacros.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/ObjectLibrary.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "Toolkits/AssetEditorManager.h"
#include "UObject/UObjectIterator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/Views/SListView.h"

#define LOCTEXT_NAMESPACE "SAnimNotifyReferenceView"

/** 多列小部件 */
class SAnimNotifieInfoRow : public SMultiColumnTableRow< TSharedPtr<FAnimNotifieInfo> >
{
public:

	SLATE_BEGIN_ARGS(SAnimNotifieInfoRow) {}
		SLATE_ARGUMENT(TSharedPtr<FAnimNotifieInfo>, Info)
	SLATE_END_ARGS()

public:

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Info = InArgs._Info;
		SMultiColumnTableRow< TSharedPtr<FAnimNotifieInfo> >::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
	virtual TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == TEXT("NotifyName"))
		{
			return	SNew(STextBlock)
					.Text(FText::FromName(Info->Name));
		}
		else if (ColumnName == TEXT("TrackName"))
		{
		    return	SNew(STextBlock)
                    .Text(FText::FromName(Info->TrackName));
		}
		else if (ColumnName == TEXT("NotifyClass"))
		{
		    return	SNew(STextBlock)
                    .Text(FText::FromString(Info->ClassName));
		}
		else if (ColumnName == TEXT("TriggerTimeOffset"))
		{
		    return	SNew(STextBlock)
                    .Text(FText::FromString(FString::SanitizeFloat(Info->TriggerTimeOffset)));
		}

		return SNullWidget::NullWidget;
	}
	END_SLATE_FUNCTION_BUILD_OPTIMIZATION

private:
	TSharedPtr<FAnimNotifieInfo> Info;
};

void SAnimNotifyReferenceView::Construct(const FArguments& InArgs, const TSharedRef<SDockTab>& ConstructUnderMajorTab, const TSharedPtr<SWindow>& ConstructUnderWindow)
{
    FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

    FDetailsViewArgs DetailsViewArgs;
    DetailsViewArgs.NameAreaSettings = FDetailsViewArgs::HideNameArea;
    DetailsViewArgs.bShowScrollBar = false;
    DetailsViewArgs.bAllowSearch = false;
    DetailsViewArgs.bShowPropertyMatrixButton = false;
    DetailsViewArgs.bAllowFavoriteSystem = false;
    
    TSharedPtr<IStructureDetailsView> StructureDetailsView;
    StructureDetailsView = PropertyEditorModule.CreateStructureDetailView(DetailsViewArgs, FStructureDetailsViewArgs(), nullptr, LOCTEXT("StructureDetailsView", "123"));
    
    FStructOnScope* Struct = new FStructOnScope(FAnimNotifyReferenceView::StaticStruct(), (uint8*)&EditProperties);
    StructureDetailsView->SetStructureData(MakeShareable(Struct));

    FContentBrowserModule& ContentBrowserModule = FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

    FilterAnim.ClassNames.Add(UAnimSequenceBase::StaticClass()->GetFName());
    FilterAnim.bRecursiveClasses = true;
    FilterAnim.bRecursivePaths = true;
    
    FAssetPickerConfig AssetPickerConfig;
    AssetPickerConfig.Filter = FilterAnim;
    AssetPickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);
    AssetPickerConfig.SetFilterDelegates.Add(&SetARFilterDelegate);
    AssetPickerConfig.OnAssetSelected = FOnAssetSelected::CreateSP(this, &SAnimNotifyReferenceView::OnAnimSelected);
    AssetPickerConfig.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateLambda([](const FAssetData& AssetData)
    {
        FAssetEditorManager::Get().OpenEditorForAsset(AssetData.ToSoftObjectPath().GetAssetPathString());
    });
    AssetPickerConfig.bAllowNullSelection = false;
    AssetPickerConfig.InitialAssetViewType = EAssetViewType::List;
    AssetPickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateLambda([this](const FAssetData& AssetData)
    {
        for (const auto& FilterAnimInfo : FilterAnimArray)
        {
            if(FilterAnimInfo.AssetData == AssetData)
                return false;
        }
        return true;
    });

    FilterAnimPicker = StaticCastSharedRef<SWidget>(ContentBrowserModule.Get().CreateAssetPicker(AssetPickerConfig));
    
    this->ChildSlot
    [
        SNew(SSplitter)
        .Orientation(Orient_Horizontal)
        
        + SSplitter::Slot()
        .Value(0.4f)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            .Padding( 5.0f )
            [
                SNew(SVerticalBox)
                
                +SVerticalBox::Slot()
                .AutoHeight()
                .Padding( 5.0f )
                [
                    StructureDetailsView->GetWidget().ToSharedRef()
                ]
                
                +SVerticalBox::Slot()
                .Padding( 5.0f )
                [
                     SNew(SBorder)
                     .BorderImage(FEditorStyle::GetBrush( "DetailsView.CategoryTop" ))
                    [
                        SAssignNew(ListViewWidget_AnimNotifyClass, SListView<TSharedPtr<FAnimNotifyClassInfo>>)
                        .ListItemsSource(&AnimNotifyClassInfos)
                        .OnGenerateRow(this, &SAnimNotifyReferenceView::OnGenerateRow_AnimNotifyClass)
                    ]
                ]
            ]
        ]
        
        + SSplitter::Slot()
        .Value(0.3f)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            .Padding( 5.0f )
            [
                SNew(SVerticalBox)
                
                +SVerticalBox::Slot()
                .AutoHeight()
                .VAlign(VAlign_Center)
                .Padding( 5.0f )
                [
                    SNew(SBorder)
                    .BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
                    .Padding( 5.0f )
                    [
                        SNew(SButton)
					    .AddMetaData<FTagMetaData>(FTagMetaData(TEXT("Actor.ConvertToBlueprint")))
					    .OnClicked(this, &SAnimNotifyReferenceView::OnUpdateFilterAnim)
					    .ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
					    .ContentPadding(FMargin(10,0))
					    .ToolTipText(LOCTEXT("OnUpdateFilterAnimTip", "根据左边窗口的搜索路径和筛选条件查找动画片段"))
					    [
					    	SNew(SHorizontalBox)
					    	.Clipping(EWidgetClipping::ClipToBounds)
					    	
					    	+ SHorizontalBox::Slot()
					    	.VAlign(VAlign_Center)
					    	.Padding(3.f)
					    	.AutoWidth()
					    	[
					    		SNew(STextBlock)
					    		.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
					    		.Font( FEditorStyle::Get().GetFontStyle( "FontAwesome.10" ) )
					    		.Text( FEditorFontGlyphs::Cogs )
					    	]
    
					    	+ SHorizontalBox::Slot()
					    	.VAlign(VAlign_Center)
					    	.Padding(3.f)
					    	.AutoWidth()
					    	[
					    		SNew(STextBlock)
					    		.TextStyle(FEditorStyle::Get(), "ContentBrowser.TopBar.Font")
					    		.Text(LOCTEXT("OnUpdateFilterAnim", "刷新"))
					    	]
					    ]
                    ]
                ]

                +SVerticalBox::Slot()
                .VAlign(VAlign_Fill)
                .Padding( 5.0f )
                [
                    SNew(SBorder)
                    .BorderImage(FEditorStyle::GetBrush("DetailsView.CategoryTop"))
                    .Padding( 5.0f )
                    [
                        FilterAnimPicker.ToSharedRef()
                    ]
                ]
            ]
        ]
        
        + SSplitter::Slot()
        .Value(0.3f)
        [
            SNew(SBorder)
            .BorderImage(FEditorStyle::GetBrush("ToolPanel.DarkGroupBorder"))
            .Padding( 10.0f )
            [
                SNew(SBorder)
                .Padding( 5.0f )
                .BorderImage(FEditorStyle::GetBrush( "DetailsView.CategoryTop" ))
                [
                    SAssignNew(ListViewWidget_SelectedAnimNotifies, SListView<TSharedPtr<FAnimNotifieInfo>>)
                    .ListItemsSource(&NowSelectedAnimNotifies)
                    .OnGenerateRow(this, &SAnimNotifyReferenceView::OnGenerateRow_SelectedAnimNotifies)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("TrackName")
                            .FillWidth(1.5)
                            //.HAlignCell(HAlign_Center)
                            .DefaultLabel(LOCTEXT("TrackName", "TrackName"))
                            .DefaultTooltip(LOCTEXT("TrackNameTip", "动画通知所在的轨道名称"))
                            .SortMode(this, &SAnimNotifyReferenceView::ColumnSortMode_TrackName)
                            .OnSort(FOnSortModeChanged::CreateLambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
                            {
                                if(NewSortMode == EColumnSortMode::Ascending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                    {
                                        FString AS = A->TrackName.ToString();
                                        FString BS = B->TrackName.ToString();
                                        if(AS.IsNumeric() && BS.IsNumeric())
                                            return FCString::Atoi(*AS) < FCString::Atoi(*BS);
                                        else
                                            return AS < BS;
                                    });
                                }
                                else if(NewSortMode == EColumnSortMode::Descending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                   {
                                        FString AS = A->TrackName.ToString();
                                        FString BS = B->TrackName.ToString();
                                        if(AS.IsNumeric() && BS.IsNumeric())
                                            return FCString::Atoi(*AS) > FCString::Atoi(*BS);
                                        else
                                            return AS > BS;
                                   });
                                }
                                columnSortMode_NotifyName = EColumnSortMode::None;
                                columnSortMode_TrackName = NewSortMode;
                                columnSortMode_NotifyClass = EColumnSortMode::None;
                                columnSortMode_Time = EColumnSortMode::None;
                                ListViewWidget_SelectedAnimNotifies->RebuildList();
                            }))

                        + SHeaderRow::Column("TriggerTimeOffset")
                                .FillWidth(1.5)
                                //.HAlignCell(HAlign_Center)
                                .DefaultLabel(LOCTEXT("TriggerTimeOffset", "StratTime"))
                                .DefaultTooltip(LOCTEXT("TriggerTimeOffsetTip", "动画通知开始时间"))
                                .SortMode(this, &SAnimNotifyReferenceView::ColumnSortMode_Time)
                                .OnSort(FOnSortModeChanged::CreateLambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
                                {
                                    if(NewSortMode == EColumnSortMode::Ascending)
                                    {
                                        NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                        {
                                            return A->TriggerTimeOffset < B->TriggerTimeOffset;
                                        });
                                    }
                                    else if(NewSortMode == EColumnSortMode::Descending)
                                    {
                                        NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                       {
                                           return A->TriggerTimeOffset > B->TriggerTimeOffset;
                                       });
                                    }
                                    columnSortMode_NotifyName = EColumnSortMode::None;
                                    columnSortMode_TrackName = EColumnSortMode::None;
                                    columnSortMode_NotifyClass = EColumnSortMode::None;
                                    columnSortMode_Time = NewSortMode;
                                    ListViewWidget_SelectedAnimNotifies->RebuildList();
                                }))
                                
                        + SHeaderRow::Column("NotifyName")
                            .FillWidth(3)
                            .DefaultLabel(LOCTEXT("NotifyName", "NotifyName"))
                            .DefaultTooltip(LOCTEXT("NotifyNameTip", "动画通知名称"))
                            .SortMode(this, &SAnimNotifyReferenceView::ColumnSortMode_NotifyName)
                            .OnSort(FOnSortModeChanged::CreateLambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
                            {
                                if(NewSortMode == EColumnSortMode::Ascending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                    {
                                        return A->Name.ToString() < B->Name.ToString();
                                    });
                                }
                                else if(NewSortMode == EColumnSortMode::Descending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                   {
                                       return A->Name.ToString() > B->Name.ToString();
                                   });
                                }
                                columnSortMode_NotifyName = NewSortMode;
                                columnSortMode_TrackName = EColumnSortMode::None;
                                columnSortMode_NotifyClass = EColumnSortMode::None;
                                columnSortMode_Time = EColumnSortMode::None;
                                ListViewWidget_SelectedAnimNotifies->RebuildList();
                            }))
                        
                        + SHeaderRow::Column("NotifyClass")
                            .FillWidth(3)
                            .DefaultLabel(LOCTEXT("NotifyClass", "NotifyClass"))
                            .DefaultTooltip(LOCTEXT("NotifyClassTip", "动画通知所在的轨道名称"))
                            .SortMode(this, &SAnimNotifyReferenceView::ColumnSortMode_NotifyClass)
                            .OnSort(FOnSortModeChanged::CreateLambda([this](const EColumnSortPriority::Type SortPriority, const FName& ColumnName, const EColumnSortMode::Type NewSortMode)
                            {
                                if(NewSortMode == EColumnSortMode::Ascending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                    {
                                        return A->ClassName < B->ClassName;
                                    });
                                }
                                else if(NewSortMode == EColumnSortMode::Descending)
                                {
                                    NowSelectedAnimNotifies.Sort([](const TSharedPtr<FAnimNotifieInfo>& A, const TSharedPtr<FAnimNotifieInfo>& B) -> bool
                                   {
                                       return A->ClassName > B->ClassName;
                                   });
                                }
                                columnSortMode_NotifyName = EColumnSortMode::None;
                                columnSortMode_TrackName = EColumnSortMode::None;
                                columnSortMode_NotifyClass = NewSortMode;
                                columnSortMode_Time = EColumnSortMode::None;
                                ListViewWidget_SelectedAnimNotifies->RebuildList();
                            }))
                            
                    )
                ]
            ]
        ]
    ];

    UpadteAnimNotifyClassInfos();
}

FReply SAnimNotifyReferenceView::OnUpdateFilterAnim()
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

    TArray<FName> PackagePaths;
    TArray<FString> ScanPaths;
    for(const auto& Path: EditProperties.ScanPaths)
    {
        if(Path.Path.IsEmpty()) continue;;
        ScanPaths.Add(Path.Path);
        PackagePaths.Add(*Path.Path);
    }
    if(ScanPaths.Num() == 0) return FReply::Handled();
    FilterAnimArray.Empty();
    AssetRegistry.ScanPathsSynchronous(ScanPaths);
    
    FARFilter ARFilter;
    ARFilter.bRecursiveClasses = true;
    ARFilter.bRecursivePaths = true;
    ARFilter.PackagePaths = PackagePaths;
    ARFilter.ClassNames.Add(UAnimSequenceBase::StaticClass()->GetFName());

    TArray<FAssetData>	AssetDataList;
    AssetRegistry.GetAssets(ARFilter, AssetDataList);

    FScopedSlowTask LoadAnimProgress(AssetDataList.Num(), LOCTEXT("SAnimNotifyReferenceView::OnUpdateFilterAnim", "加载动画数据"));
    LoadAnimProgress.MakeDialog();
    for(const auto& AssetData : AssetDataList)
    {
        LoadAnimProgress.EnterProgressFrame(1.f, FText::FromString(TEXT("加载动画数据: ") + AssetData.ObjectPath.ToString()));
        if(UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(AssetData.GetAsset()))
        {
            TArray<FAnimNotifyEvent> Notifies;
            Notifies.Append(AnimSequenceBase->Notifies);

            bool bIsContain = false;
            for(const auto& Notifie : Notifies)
            {
                UClass* NotifieClass = nullptr;
                if(Notifie.Notify)
                {
                    NotifieClass = Notifie.Notify->GetClass();
                }
                else if(Notifie.NotifyStateClass)
                {
                    NotifieClass = Notifie.NotifyStateClass->GetClass();
                }
                
                for(const auto& Info : AnimNotifyClassInfos)
                {
                    if(Info->CheckBoxState == ECheckBoxState::Checked)
                    {
                        if(NotifieClass->IsChildOf(Info->Class))
                        {
                            bIsContain = true;
                            goto BreakFor;
                        }
                    }
                }
            }
            BreakFor :
            
            if(bIsContain) FilterAnimArray.Emplace(AnimSequenceBase, AssetData);
        }
    }

    RefreshAssetViewDelegate.ExecuteIfBound(true);
    FilterAnim.PackagePaths = PackagePaths;
    SetARFilterDelegate.ExecuteIfBound(FilterAnim);
    return FReply::Handled();
}

void SAnimNotifyReferenceView::UpadteAnimNotifyClassInfos()
{
    AnimNotifyClassInfos.Empty();

    /* 收集 UAnimNotify */
    {
        TArray<UClass*> AnimNotifyClassArray;
        for(TObjectIterator<UClass> It; It; ++It)
        {
            if(It->IsChildOf(UAnimNotify::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract) && It->HasAnyClassFlags(CLASS_Native))
            {
                AnimNotifyClassArray.Add(*It);
                AnimNotifyClassInfos.Add(MakeShared<FAnimNotifyClassInfo>(*(It->GetDisplayNameText().ToString()), *It, false));
            }
        }
        
        TArray<FAssetData> AnimNotifyBlueprintClassArray;
        UObjectLibrary* AnimNotifyClassLibrary = UObjectLibrary::CreateLibrary(UAnimNotify::StaticClass(), true, true);
        AnimNotifyClassLibrary->AddToRoot();
        AnimNotifyClassLibrary->LoadBlueprintAssetDataFromPath(TEXT("/Game/"));
        AnimNotifyClassLibrary->GetAssetDataList(AnimNotifyBlueprintClassArray);
        for(const auto& AssetData : AnimNotifyBlueprintClassArray)
        {
            UClass* AssetDataClass = Cast<UBlueprint>(AssetData.GetAsset())->GeneratedClass;
            AnimNotifyClassInfos.Add(MakeShared<FAnimNotifyClassInfo>(AssetData.AssetName, AssetDataClass, false));
        }
    }

    /* 收集 UAnimNotifyState*/
    {
        TArray<UClass*> AnimNotifyClassArray;
        for(TObjectIterator<UClass> It; It; ++It)
        {
            if(It->IsChildOf(UAnimNotifyState::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract) && It->HasAnyClassFlags(CLASS_Native))
            {
                AnimNotifyClassArray.Add(*It);
                AnimNotifyClassInfos.Add(MakeShared<FAnimNotifyClassInfo>(*(It->GetDisplayNameText().ToString()), *It, true));
            }
        }
    
        TArray<FAssetData> AnimNotifyStateBlueprintClassArray;
        UObjectLibrary* AnimNotifyStateClassLibrary = UObjectLibrary::CreateLibrary(UAnimNotifyState::StaticClass(), true, true);
        AnimNotifyStateClassLibrary->AddToRoot();
        AnimNotifyStateClassLibrary->LoadBlueprintAssetDataFromPath(TEXT("/Game/"));
        AnimNotifyStateClassLibrary->GetAssetDataList(AnimNotifyStateBlueprintClassArray);
        for(const auto& AssetData : AnimNotifyStateBlueprintClassArray)
        {
            UClass* AssetDataClass = Cast<UBlueprint>(AssetData.GetAsset())->GeneratedClass;
            AnimNotifyClassInfos.Add(MakeShared<FAnimNotifyClassInfo>(AssetData.AssetName, AssetDataClass, true));
        }
    }

    AnimNotifyClassInfos.Sort([](const TSharedPtr<FAnimNotifyClassInfo> &A, const TSharedPtr<FAnimNotifyClassInfo> &B)
    {
        return A->ClassName.ToString() < B->ClassName.ToString();
    });
    
    ListViewWidget_AnimNotifyClass->RebuildList();
}

TSharedRef<ITableRow> SAnimNotifyReferenceView::OnGenerateRow_AnimNotifyClass(const TSharedPtr<FAnimNotifyClassInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(STableRow<TSharedPtr<FAnimNotifyClassInfo>>, OwnerTable)
        .Padding(FMargin(0.f, 2.5f, 0.f, 2.5f))
        .Content()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            .AutoWidth()
            .Padding(5.f, 0.f)
            [
                SNew(SCheckBox)
                .IsChecked(TAttribute<ECheckBoxState>::Create(TAttribute<ECheckBoxState>::FGetter::CreateLambda([Item]() {return Item->CheckBoxState;})))
                .OnCheckStateChanged(FOnCheckStateChanged::CreateLambda([Item](ECheckBoxState NewState)
                {
                    Item->CheckBoxState = NewState;
                }))
            ]
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text(FText::FromName(Item->ClassName))
            ]
            + SHorizontalBox::Slot()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Center)
            .AutoWidth()
            .Padding(0.f, 0.f, 5.f, 0.f)
            [
                SNew(STextBlock).Text(FText::FromString(Item->bIsAnimNotifyState ? TEXT("AnimNotifyState") : TEXT("AnimNotify")))
            ]
        ];
}

TSharedRef<ITableRow> SAnimNotifyReferenceView::OnGenerateRow_SelectedAnimNotifies(const TSharedPtr<FAnimNotifieInfo> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    return SNew(SAnimNotifieInfoRow, OwnerTable)
    .Info(Item);
}

void SAnimNotifyReferenceView::OnAnimSelected(const FAssetData& AssetData)
{
    NowSelectedAnimNotifies.Empty();

    if(AssetData.IsValid())
    {
        UAnimSequenceBase* AnimSequenceBase = Cast<UAnimSequenceBase>(AssetData.GetAsset());
        TArray<FAnimNotifyEvent>& Notifies = AnimSequenceBase->Notifies;

        for(const auto& Notifie : Notifies)
        {
            bool bIsBlueprint = false;
            UClass* NotifieClass = Notifie.Notify ? Notifie.Notify->GetClass() : Notifie.NotifyStateClass->GetClass();
            if(Cast<UBlueprintGeneratedClass>(NotifieClass))
                NowSelectedAnimNotifies.Add(MakeShared<FAnimNotifieInfo>(Notifie.NotifyName, NotifieClass->GetName(), AnimSequenceBase->AnimNotifyTracks[Notifie.TrackIndex].TrackName, Notifie.GetTriggerTime()));
            else
                NowSelectedAnimNotifies.Add(MakeShared<FAnimNotifieInfo>(Notifie.NotifyName, NotifieClass->GetDisplayNameText().ToString(), AnimSequenceBase->AnimNotifyTracks[Notifie.TrackIndex].TrackName, Notifie.GetTriggerTime()));
        }
    }
    
    ListViewWidget_SelectedAnimNotifies->RebuildList();
}

#undef LOCTEXT_NAMESPACE
