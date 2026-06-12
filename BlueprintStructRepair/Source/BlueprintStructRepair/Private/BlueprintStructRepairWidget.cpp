#include "BlueprintStructRepairWidget.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/DataTable.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/ScopedSlowTask.h"
#include "Modules/ModuleManager.h"
#include "StructUtils/UserDefinedStruct.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "UObject/Package.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "SBlueprintStructRepairWidget"

void SBlueprintStructRepairWidget::Construct(const FArguments& InArgs)
{
	StatusMessage = TEXT("Enter a Blueprint asset name or path under Content.");

	ChildSlot
	[
		SNew(SBorder)
		.Padding(10.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(SearchTextBox, SEditableTextBox)
					.HintText(LOCTEXT("SearchHint", "AC_StoredStuff or AC_StoredStuff/AC_StoredStuff or /Game/..."))
					.OnTextCommitted_Lambda([this](const FText&, ETextCommit::Type CommitType)
					{
						if (CommitType == ETextCommit::OnEnter)
						{
							FindAsset();
						}
					})
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("FindAssetButton", "Find Asset"))
					.OnClicked(this, &SBlueprintStructRepairWidget::FindAsset)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("OpenAssetButton", "Open Asset"))
					.IsEnabled(this, &SBlueprintStructRepairWidget::CanUseFoundAsset)
					.OnClicked(this, &SBlueprintStructRepairWidget::OpenAsset)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("RefreshNodesButton", "Refresh/Compile"))
					.IsEnabled(this, &SBlueprintStructRepairWidget::CanUseFoundAsset)
					.OnClicked(this, &SBlueprintStructRepairWidget::RefreshNodes)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 8.0f, 0.0f, 8.0f)
			[
				SNew(STextBlock)
				.AutoWrapText(true)
				.Text(this, &SBlueprintStructRepairWidget::GetStatusText)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 0.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("StructSummaryLabel", "User-defined structs used by selected asset"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.3f)
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(StructRowsBox, SVerticalBox)
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0f, 10.0f, 0.0f, 4.0f)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("BuildLogBatchLabel", "Build log batch repair"))
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SAssignNew(LogPathTextBox, SEditableTextBox)
					.Text(FText::FromString(GetDefaultLogPath()))
					.HintText(LOCTEXT("LogPathHint", "Project build log path"))
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("ParseBuildLogButton", "Parse Log"))
					.ButtonColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.08f, 0.06f, 1.0f)))
					.OnClicked(this, &SBlueprintStructRepairWidget::ParseBuildLog)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(6.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SButton)
					.Text(LOCTEXT("CollectErrorStructsButton", "Collect Structs"))
					.OnClicked(this, &SBlueprintStructRepairWidget::CollectStructsFromErrorAssets)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(0.7f)
			.Padding(0.0f, 6.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.Padding(0.0f, 0.0f, 4.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(ErrorAssetRowsBox, SVerticalBox)
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SButton)
							.Text(LOCTEXT("RefreshSelectedErrorAssetButton", "Refresh/Compile Selected"))
							.IsEnabled(this, &SBlueprintStructRepairWidget::CanUseSelectedErrorAsset)
							.OnClicked(this, &SBlueprintStructRepairWidget::RefreshSelectedErrorAsset)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(6.0f, 0.0f, 0.0f, 0.0f)
						[
							SNew(SButton)
							.Text(LOCTEXT("RefreshAllErrorAssetsButton", "Refresh/Compile All"))
							.IsEnabled(this, &SBlueprintStructRepairWidget::CanUseErrorAssets)
							.OnClicked(this, &SBlueprintStructRepairWidget::RefreshErrorAssets)
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.5f)
				.Padding(4.0f, 0.0f, 0.0f, 0.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.FillHeight(1.0f)
					[
						SNew(SScrollBox)
						+ SScrollBox::Slot()
						[
							SAssignNew(BatchStructRowsBox, SVerticalBox)
						]
					]
				]
			]
		]
	];

	RebuildStructRows();
	RebuildErrorAssetRows();
	RebuildBatchStructRows();
}

FReply SBlueprintStructRepairWidget::FindAsset()
{
	bHasFoundAsset = false;
	FoundAssetData = FAssetData();
	ClearStructRows();

	const FString RawQuery = SearchTextBox.IsValid() ? SearchTextBox->GetText().ToString() : FString();
	const FString Query = NormalizeSearchText(RawQuery);
	if (Query.IsEmpty())
	{
		StatusMessage = TEXT("Enter an asset name or path first.");
		return FReply::Handled();
	}

	int32 MatchCount = 0;
	if (!TryFindBlueprintAsset(Query, FoundAssetData, MatchCount))
	{
		StatusMessage = FString::Printf(TEXT("No Blueprint asset found under /Game for: %s"), *Query);
		return FReply::Handled();
	}

	bHasFoundAsset = true;
	AnalyzeUsedStructs();

	const UBlueprint* Blueprint = GetFoundBlueprint();
	const FString UnsavedChanges = GetUnsavedChangesText(Blueprint);
	StatusMessage = MatchCount > 1
		? FString::Printf(TEXT("Found %s. Multiple matches existed (%d); using the best match. Unsaved changes: %s. User-defined structs: %d."), *FoundAssetData.PackageName.ToString(), MatchCount, *UnsavedChanges, StructRows.Num())
		: FString::Printf(TEXT("Found %s. Unsaved changes: %s. User-defined structs: %d."), *FoundAssetData.PackageName.ToString(), *UnsavedChanges, StructRows.Num());

	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::OpenAsset()
{
	if (!bHasFoundAsset)
	{
		StatusMessage = TEXT("Find an asset first.");
		return FReply::Handled();
	}

	UObject* Asset = FoundAssetData.GetAsset();
	if (!Asset)
	{
		bHasFoundAsset = false;
		ClearStructRows();
		StatusMessage = FString::Printf(TEXT("Could not load asset: %s"), *FoundAssetData.PackageName.ToString());
		return FReply::Handled();
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor ? GEditor->GetEditorSubsystem<UAssetEditorSubsystem>() : nullptr;
	if (!AssetEditorSubsystem)
	{
		StatusMessage = TEXT("Asset editor subsystem is not available.");
		return FReply::Handled();
	}

	const bool bOpened = AssetEditorSubsystem->OpenEditorForAsset(Asset);
	const UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
	StatusMessage = FString::Printf(
		TEXT("%s %s. Unsaved changes: %s."),
		bOpened ? TEXT("Opened") : TEXT("Could not open"),
		*FoundAssetData.PackageName.ToString(),
		*GetUnsavedChangesText(Blueprint));
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::RefreshNodes()
{
	if (!bHasFoundAsset)
	{
		StatusMessage = TEXT("Find an asset first.");
		return FReply::Handled();
	}

	UBlueprint* Blueprint = GetFoundBlueprint();
	if (!Blueprint)
	{
		bHasFoundAsset = false;
		ClearStructRows();
		StatusMessage = FString::Printf(TEXT("Loaded asset is not a Blueprint: %s"), *FoundAssetData.PackageName.ToString());
		return FReply::Handled();
	}

	FString RefreshMessage;
	RefreshAndCompileBlueprint(Blueprint, RefreshMessage);
	AnalyzeUsedStructs();
	StatusMessage = FString::Printf(
		TEXT("%s for %s. Unsaved changes: %s. User-defined structs: %d."),
		*RefreshMessage,
		*FoundAssetData.PackageName.ToString(),
		*GetUnsavedChangesText(Blueprint),
		StructRows.Num());
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::ShowStructInFolder(TSharedPtr<FStructUsageInfo> StructInfo)
{
	UUserDefinedStruct* Struct = StructInfo.IsValid() ? StructInfo->Struct.Get() : nullptr;
	if (!Struct || !GEditor)
	{
		StatusMessage = TEXT("Could not sync struct in Content Browser.");
		return FReply::Handled();
	}

	TArray<UObject*> ObjectsToSync;
	ObjectsToSync.Add(Struct);
	GEditor->SyncBrowserToObjects(ObjectsToSync, true);
	StatusMessage = FString::Printf(TEXT("Synced Content Browser to %s."), *StructInfo->PackageName);
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::ParseBuildLog()
{
	ErrorAssetRows.Reset();
	BatchStructRows.Reset();
	ErrorStructPackageNames.Reset();
	SelectedErrorAssetPackageName.Reset();

	const FString RequestedLogPath = LogPathTextBox.IsValid()
		? LogPathTextBox->GetText().ToString()
		: GetDefaultLogPath();

	TArray<FString> Lines;
	FString ReadLogPath;
	FString ReadDiagnostic;
	if (!LoadLogLines(RequestedLogPath, Lines, ReadLogPath, ReadDiagnostic))
	{
		RebuildErrorAssetRows();
		RebuildBatchStructRows();
		StatusMessage = FString::Printf(TEXT("Could not read log file: %s. %s"), *RequestedLogPath, *ReadDiagnostic);
		return FReply::Handled();
	}

	TMap<FString, FString> PackageToFirstErrorLine;
	TSet<FString> ReferencedLogPaths;
	TSet<FString> ParsedLogPaths;
	ParsedLogPaths.Add(ReadLogPath);

	int32 ParsedFileCount = 1;
	int32 TotalLineCount = Lines.Num();
	int32 RelevantLineCount = 0;
	int32 RawPackageCandidateCount = 0;
	ParseLogLines(Lines, PackageToFirstErrorLine, ReferencedLogPaths, RelevantLineCount, RawPackageCandidateCount);

	TArray<FString> PendingReferencedLogPaths = ReferencedLogPaths.Array();
	for (int32 PendingIndex = 0; PendingIndex < PendingReferencedLogPaths.Num(); ++PendingIndex)
	{
		const FString ReferencedLogPath = PendingReferencedLogPaths[PendingIndex];
		if (ParsedLogPaths.Contains(ReferencedLogPath))
		{
			continue;
		}

		TArray<FString> ReferencedLines;
		FString ReferencedReadPath;
		FString ReferencedDiagnostic;
		if (!LoadLogLines(ReferencedLogPath, ReferencedLines, ReferencedReadPath, ReferencedDiagnostic))
		{
			continue;
		}

		if (ParsedLogPaths.Contains(ReferencedReadPath))
		{
			continue;
		}

		ParsedLogPaths.Add(ReferencedReadPath);
		++ParsedFileCount;
		TotalLineCount += ReferencedLines.Num();
		const int32 ReferencedPathCountBeforeParse = ReferencedLogPaths.Num();
		ParseLogLines(ReferencedLines, PackageToFirstErrorLine, ReferencedLogPaths, RelevantLineCount, RawPackageCandidateCount);
		if (ReferencedLogPaths.Num() != ReferencedPathCountBeforeParse)
		{
			for (const FString& NewReferencedLogPath : ReferencedLogPaths)
			{
				if (!ParsedLogPaths.Contains(NewReferencedLogPath))
				{
					PendingReferencedLogPaths.AddUnique(NewReferencedLogPath);
				}
			}
		}
	}

	TArray<FString> SortedPackageNames;
	PackageToFirstErrorLine.GetKeys(SortedPackageNames);
	SortedPackageNames.Sort();

	TMap<FString, UUserDefinedStruct*> UniqueStructsByPackage;
	for (const FString& PackageName : SortedPackageNames)
	{
		FAssetData AssetData;
		if (ResolveBlueprintAsset(PackageName, AssetData))
		{
			TSharedPtr<FErrorAssetInfo> Info = MakeShared<FErrorAssetInfo>();
			Info->AssetData = AssetData;
			Info->PackageName = AssetData.PackageName.ToString();
			Info->FirstErrorLine = PackageToFirstErrorLine[PackageName];
			ErrorAssetRows.Add(Info);
		}

		UUserDefinedStruct* Struct = nullptr;
		if (ResolveUserDefinedStructAsset(PackageName, Struct) && Struct)
		{
			const FString StructPackageName = Struct->GetOutermost()->GetName();
			UniqueStructsByPackage.FindOrAdd(StructPackageName, Struct);
			ErrorStructPackageNames.Add(StructPackageName);
		}
	}

	TArray<FString> SortedStructPackages;
	UniqueStructsByPackage.GetKeys(SortedStructPackages);
	SortedStructPackages.Sort();
	for (const FString& StructPackage : SortedStructPackages)
	{
		if (TSharedPtr<FStructUsageInfo> Info = MakeStructInfo(UniqueStructsByPackage[StructPackage]))
		{
			BatchStructRows.Add(Info);
		}
	}

	RebuildErrorAssetRows();
	RebuildBatchStructRows();
	AnalyzeUsedStructs();
	StatusMessage = FString::Printf(
		TEXT("Parsed %d log file(s), %d lines. Relevant lines: %d. Package candidates: %d. Blueprint assets: %d. Struct assets: %d. Read: %s"),
		ParsedFileCount,
		TotalLineCount,
		RelevantLineCount,
		RawPackageCandidateCount,
		ErrorAssetRows.Num(),
		BatchStructRows.Num(),
		*ReadLogPath);
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::SelectErrorAsset(TSharedPtr<FErrorAssetInfo> ErrorAsset)
{
	if (!ErrorAsset.IsValid())
	{
		StatusMessage = TEXT("Could not select error asset.");
		return FReply::Handled();
	}

	FoundAssetData = ErrorAsset->AssetData;
	bHasFoundAsset = true;
	SelectedErrorAssetPackageName = ErrorAsset->PackageName;

	if (SearchTextBox.IsValid())
	{
		SearchTextBox->SetText(FText::FromString(ErrorAsset->PackageName));
	}

	AnalyzeUsedStructs();
	RebuildErrorAssetRows();

	const UBlueprint* Blueprint = GetFoundBlueprint();
	StatusMessage = FString::Printf(
		TEXT("Selected %s from error log. Unsaved changes: %s. User-defined structs: %d."),
		*ErrorAsset->PackageName,
		*GetUnsavedChangesText(Blueprint),
		StructRows.Num());
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::RefreshSelectedErrorAsset()
{
	if (SelectedErrorAssetPackageName.IsEmpty())
	{
		StatusMessage = TEXT("Select an error asset first.");
		return FReply::Handled();
	}

	TSharedPtr<FErrorAssetInfo> SelectedErrorAsset;
	for (const TSharedPtr<FErrorAssetInfo>& ErrorAsset : ErrorAssetRows)
	{
		if (ErrorAsset.IsValid() && ErrorAsset->PackageName.Equals(SelectedErrorAssetPackageName, ESearchCase::IgnoreCase))
		{
			SelectedErrorAsset = ErrorAsset;
			break;
		}
	}

	if (!SelectedErrorAsset.IsValid())
	{
		SelectedErrorAssetPackageName.Reset();
		StatusMessage = TEXT("Selected error asset is no longer in the parsed list.");
		return FReply::Handled();
	}

	FoundAssetData = SelectedErrorAsset->AssetData;
	bHasFoundAsset = true;
	UBlueprint* Blueprint = GetFoundBlueprint();
	if (!Blueprint)
	{
		StatusMessage = FString::Printf(TEXT("Selected asset is not a loaded Blueprint: %s"), *SelectedErrorAsset->PackageName);
		return FReply::Handled();
	}

	FString RefreshMessage;
	int32 CompileErrorCount = 0;
	const bool bCompileSucceeded = RefreshAndCompileBlueprint(Blueprint, RefreshMessage, &CompileErrorCount);
	ApplyRefreshCompileResult(SelectedErrorAsset, RefreshMessage, bCompileSucceeded, CompileErrorCount);
	AnalyzeUsedStructs();
	RebuildErrorAssetRows();
	StatusMessage = FString::Printf(
		TEXT("%s for selected asset %s. Unsaved changes: %s. User-defined structs: %d."),
		*RefreshMessage,
		*SelectedErrorAsset->PackageName,
		*GetUnsavedChangesText(Blueprint),
		StructRows.Num());
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::RefreshErrorAssets()
{
	if (ErrorAssetRows.Num() == 0)
	{
		StatusMessage = TEXT("Parse a build log first.");
		return FReply::Handled();
	}

	int32 RefreshedCount = 0;
	int32 FailedCompileCount = 0;
	FScopedSlowTask SlowTask(static_cast<float>(ErrorAssetRows.Num()), LOCTEXT("RefreshErrorAssetsTask", "Refreshing nodes for error assets"));
	SlowTask.MakeDialog(false);

	TSet<FString> ProcessedPackages;
	for (const TSharedPtr<FErrorAssetInfo>& ErrorAsset : ErrorAssetRows)
	{
		if (!ErrorAsset.IsValid() || ProcessedPackages.Contains(ErrorAsset->PackageName))
		{
			continue;
		}

		ProcessedPackages.Add(ErrorAsset->PackageName);
		SlowTask.EnterProgressFrame(1.0f, FText::FromString(ErrorAsset->PackageName));

		UBlueprint* Blueprint = Cast<UBlueprint>(ErrorAsset->AssetData.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		FString RefreshMessage;
		int32 CompileErrorCount = 0;
		const bool bCompileSucceeded = RefreshAndCompileBlueprint(Blueprint, RefreshMessage, &CompileErrorCount);
		ApplyRefreshCompileResult(ErrorAsset, RefreshMessage, bCompileSucceeded, CompileErrorCount);
		if (!bCompileSucceeded)
		{
			++FailedCompileCount;
		}
		++RefreshedCount;
	}

	RebuildErrorAssetRows();
	StatusMessage = FailedCompileCount == 0
		? FString::Printf(TEXT("Refresh All Nodes + Compile completed for %d unique Blueprint assets. Compile errors: 0. Safe to review and then save manually."), RefreshedCount)
		: FString::Printf(TEXT("Refresh All Nodes + Compile completed for %d unique Blueprint assets. Assets with compile errors: %d. Do not Save All as a repair step."), RefreshedCount, FailedCompileCount);
	return FReply::Handled();
}

FReply SBlueprintStructRepairWidget::CollectStructsFromErrorAssets()
{
	TMap<FString, UUserDefinedStruct*> UniqueStructsByPackage;
	for (const TSharedPtr<FStructUsageInfo>& ExistingStructInfo : BatchStructRows)
	{
		UUserDefinedStruct* Struct = ExistingStructInfo.IsValid() ? ExistingStructInfo->Struct.Get() : nullptr;
		if (Struct)
		{
			const FString StructPackageName = Struct->GetOutermost()->GetName();
			UniqueStructsByPackage.FindOrAdd(StructPackageName, Struct);
			ErrorStructPackageNames.Add(StructPackageName);
		}
	}

	BatchStructRows.Reset();
	for (const TSharedPtr<FErrorAssetInfo>& ErrorAsset : ErrorAssetRows)
	{
		if (!ErrorAsset.IsValid())
		{
			continue;
		}

		UBlueprint* Blueprint = Cast<UBlueprint>(ErrorAsset->AssetData.GetAsset());
		if (!Blueprint)
		{
			continue;
		}

		TSet<UUserDefinedStruct*> StructsForAsset;
		CollectUsedStructs(Blueprint, StructsForAsset);
		for (UUserDefinedStruct* Struct : StructsForAsset)
		{
			if (!Struct)
			{
				continue;
			}

			const FString StructPackageName = Struct->GetOutermost()->GetName();
			UniqueStructsByPackage.FindOrAdd(StructPackageName, Struct);
			ErrorStructPackageNames.Add(StructPackageName);
		}
	}

	TArray<FString> SortedStructPackages;
	UniqueStructsByPackage.GetKeys(SortedStructPackages);
	SortedStructPackages.Sort();
	for (const FString& StructPackage : SortedStructPackages)
	{
		if (TSharedPtr<FStructUsageInfo> Info = MakeStructInfo(UniqueStructsByPackage[StructPackage]))
		{
			BatchStructRows.Add(Info);
		}
	}

	RebuildBatchStructRows();
	RebuildStructRows();
	StatusMessage = FString::Printf(TEXT("Collected %d unique user-defined structs from %d error assets."), BatchStructRows.Num(), ErrorAssetRows.Num());
	return FReply::Handled();
}

bool SBlueprintStructRepairWidget::TryFindBlueprintAsset(const FString& Query, FAssetData& OutAssetData, int32& OutMatchCount) const
{
	TArray<FAssetData> Assets;
	FindBlueprintAssets(Assets);

	TArray<FAssetData> Matches;
	for (const FAssetData& Asset : Assets)
	{
		if (DoesAssetMatch(Asset, Query))
		{
			Matches.Add(Asset);
		}
	}

	OutMatchCount = Matches.Num();
	if (Matches.Num() == 0)
	{
		return false;
	}

	Matches.Sort([&Query](const FAssetData& A, const FAssetData& B)
	{
		const FString AName = A.AssetName.ToString();
		const FString BName = B.AssetName.ToString();
		const FString APackage = A.PackageName.ToString();
		const FString BPackage = B.PackageName.ToString();

		const int32 AScore =
			(AName.Equals(Query, ESearchCase::IgnoreCase) ? 100 : 0) +
			(APackage.Equals(Query, ESearchCase::IgnoreCase) ? 80 : 0) +
			(APackage.EndsWith(Query, ESearchCase::IgnoreCase) ? 60 : 0);
		const int32 BScore =
			(BName.Equals(Query, ESearchCase::IgnoreCase) ? 100 : 0) +
			(BPackage.Equals(Query, ESearchCase::IgnoreCase) ? 80 : 0) +
			(BPackage.EndsWith(Query, ESearchCase::IgnoreCase) ? 60 : 0);

		if (AScore != BScore)
		{
			return AScore > BScore;
		}
		return APackage < BPackage;
	});

	OutAssetData = Matches[0];
	return true;
}

void SBlueprintStructRepairWidget::FindBlueprintAssets(TArray<FAssetData>& OutAssets) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().WaitForCompletion();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	AssetRegistryModule.Get().GetAssets(Filter, OutAssets);
}

FString SBlueprintStructRepairWidget::NormalizeSearchText(FString SearchText) const
{
	SearchText.TrimStartAndEndInline();
	SearchText.TrimQuotesInline();
	SearchText.ReplaceInline(TEXT("\\"), TEXT("/"));

	const FString ContentMarker = TEXT("/Content/");
	const int32 ContentIndex = SearchText.Find(ContentMarker, ESearchCase::IgnoreCase);
	if (ContentIndex != INDEX_NONE)
	{
		SearchText = TEXT("/Game/") + SearchText.RightChop(ContentIndex + ContentMarker.Len());
	}

	if (SearchText.EndsWith(TEXT(".uasset"), ESearchCase::IgnoreCase))
	{
		SearchText.LeftChopInline(7);
	}

	if (SearchText.StartsWith(TEXT("Content/"), ESearchCase::IgnoreCase))
	{
		SearchText = TEXT("/Game/") + SearchText.RightChop(8);
	}

	return SearchText;
}

bool SBlueprintStructRepairWidget::DoesAssetMatch(const FAssetData& AssetData, const FString& SearchText) const
{
	const FString AssetName = AssetData.AssetName.ToString();
	const FString PackageName = AssetData.PackageName.ToString();
	const FString ObjectPath = AssetData.GetSoftObjectPath().ToString();

	if (AssetName.Equals(SearchText, ESearchCase::IgnoreCase)
		|| PackageName.Equals(SearchText, ESearchCase::IgnoreCase)
		|| ObjectPath.Equals(SearchText, ESearchCase::IgnoreCase)
		|| PackageName.EndsWith(SearchText, ESearchCase::IgnoreCase)
		|| ObjectPath.EndsWith(SearchText, ESearchCase::IgnoreCase))
	{
		return true;
	}

	return AssetName.Contains(SearchText, ESearchCase::IgnoreCase)
		|| PackageName.Contains(SearchText, ESearchCase::IgnoreCase);
}

UBlueprint* SBlueprintStructRepairWidget::GetFoundBlueprint() const
{
	return Cast<UBlueprint>(FoundAssetData.GetAsset());
}

FString SBlueprintStructRepairWidget::GetUnsavedChangesText(const UBlueprint* Blueprint) const
{
	const UPackage* Package = Blueprint ? Blueprint->GetOutermost() : nullptr;
	if (!Package)
	{
		return TEXT("Unknown");
	}

	return Package->IsDirty() ? TEXT("Yes") : TEXT("No");
}

bool SBlueprintStructRepairWidget::RefreshAndCompileBlueprint(UBlueprint* Blueprint, FString& OutMessage, int32* OutErrorCount) const
{
	if (!Blueprint)
	{
		OutMessage = TEXT("Blueprint is not loaded");
		if (OutErrorCount)
		{
			*OutErrorCount = 1;
		}
		return false;
	}

	FBlueprintEditorUtils::RefreshAllNodes(Blueprint);

	FCompilerResultsLog CompileResults;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::SkipGarbageCollection, &CompileResults);

	const bool bCompiled = Blueprint->Status != BS_Error;
	if (OutErrorCount)
	{
		*OutErrorCount = CompileResults.NumErrors;
	}
	OutMessage = bCompiled
		? TEXT("Refresh All Nodes + Compile completed")
		: FString::Printf(TEXT("Refresh All Nodes completed, but compile has errors (%d error(s))"), CompileResults.NumErrors);
	return bCompiled;
}

void SBlueprintStructRepairWidget::ApplyRefreshCompileResult(TSharedPtr<FErrorAssetInfo> ErrorAsset, const FString& Message, bool bSucceeded, int32 ErrorCount) const
{
	if (!ErrorAsset.IsValid())
	{
		return;
	}

	ErrorAsset->bRefreshCompileAttempted = true;
	ErrorAsset->bCompileSucceeded = bSucceeded;
	ErrorAsset->CompileErrorCount = ErrorCount;
	ErrorAsset->CompileStatusText = bSucceeded
		? TEXT("Compile OK after refresh.")
		: FString::Printf(TEXT("Compile failed after refresh. Errors: %d. Do not save as repaired."), ErrorCount);
	if (!Message.IsEmpty())
	{
		ErrorAsset->CompileStatusText += FString::Printf(TEXT(" %s"), *Message);
	}
}

void SBlueprintStructRepairWidget::AnalyzeUsedStructs()
{
	StructRows.Reset();

	UBlueprint* Blueprint = GetFoundBlueprint();
	if (!Blueprint)
	{
		RebuildStructRows();
		return;
	}

	TSet<UUserDefinedStruct*> UsedStructs;
	CollectUsedStructs(Blueprint, UsedStructs);

	TArray<UUserDefinedStruct*> SortedStructs = UsedStructs.Array();
	SortedStructs.Sort([](const UUserDefinedStruct& A, const UUserDefinedStruct& B)
	{
		return A.GetOutermost()->GetName() < B.GetOutermost()->GetName();
	});

	for (UUserDefinedStruct* Struct : SortedStructs)
	{
		if (TSharedPtr<FStructUsageInfo> Info = MakeStructInfo(Struct))
		{
			StructRows.Add(Info);
		}
	}

	RebuildStructRows();
}

void SBlueprintStructRepairWidget::CollectUsedStructs(UBlueprint* Blueprint, TSet<UUserDefinedStruct*>& OutStructs) const
{
	if (!Blueprint)
	{
		return;
	}

	for (const FBPVariableDescription& Variable : Blueprint->NewVariables)
	{
		AddStructFromPinType(Variable.VarType, OutStructs);
	}

	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);

	TFunction<void(const UEdGraphPin*)> VisitPin = [this, &OutStructs, &VisitPin](const UEdGraphPin* Pin)
	{
		if (!Pin)
		{
			return;
		}

		AddStructFromPinType(Pin->PinType, OutStructs);
		for (const UEdGraphPin* SubPin : Pin->SubPins)
		{
			VisitPin(SubPin);
		}
	};

	for (const UEdGraph* Graph : Graphs)
	{
		if (!Graph)
		{
			continue;
		}

		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				continue;
			}

			for (const UEdGraphPin* Pin : Node->Pins)
			{
				VisitPin(Pin);
			}
		}
	}
}

void SBlueprintStructRepairWidget::AddStructFromPinType(const FEdGraphPinType& PinType, TSet<UUserDefinedStruct*>& OutStructs) const
{
	auto AddIfUserStruct = [&OutStructs](UObject* Object)
	{
		UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Object);
		if (Struct && Struct->GetOutermost()->GetName().StartsWith(TEXT("/Game/")))
		{
			OutStructs.Add(Struct);
		}
	};

	AddIfUserStruct(PinType.PinSubCategoryObject.Get());
	AddIfUserStruct(PinType.PinValueType.TerminalSubCategoryObject.Get());
}

TSharedPtr<FStructUsageInfo> SBlueprintStructRepairWidget::MakeStructInfo(UUserDefinedStruct* Struct) const
{
	if (!Struct)
	{
		return nullptr;
	}

	TSharedPtr<FStructUsageInfo> Info = MakeShared<FStructUsageInfo>();
	Info->Struct = Struct;
	Info->Name = Struct->GetName();
	Info->PackageName = Struct->GetOutermost()->GetName();
	Info->ModifiedText = GetPackageModifiedText(Info->PackageName);
	Info->TablesText = GetTablesUsingStructText(Struct);
	Info->bHasErrors = HasStructErrors(Info->PackageName);
	return Info;
}

FString SBlueprintStructRepairWidget::GetPackageModifiedText(const FString& PackageName) const
{
	FString Filename;
	if (!FPackageName::TryConvertLongPackageNameToFilename(PackageName, Filename, FPackageName::GetAssetPackageExtension()))
	{
		return TEXT("Unknown");
	}

	const FDateTime Timestamp = IFileManager::Get().GetTimeStamp(*Filename);
	if (Timestamp.GetTicks() <= 0)
	{
		return TEXT("Unknown");
	}

	return Timestamp.ToString(TEXT("%Y-%m-%d %H:%M"));
}

FString SBlueprintStructRepairWidget::GetTablesUsingStructText(const UUserDefinedStruct* Struct) const
{
	if (!Struct)
	{
		return TEXT("Unknown");
	}

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().WaitForCompletion();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(TEXT("/Game")));
	Filter.bRecursivePaths = true;
	Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;

	TArray<FAssetData> DataTableAssets;
	AssetRegistryModule.Get().GetAssets(Filter, DataTableAssets);

	const FString StructTopLevelPath = Struct->GetStructPathName().ToString();
	const FString StructObjectPath = Struct->GetPathName();
	static const FName RowStructureTagName(TEXT("RowStructure"));

	TArray<FString> TableNames;
	for (const FAssetData& DataTableAsset : DataTableAssets)
	{
		FString RowStructure;
		if (!DataTableAsset.GetTagValue<FString>(RowStructureTagName, RowStructure))
		{
			continue;
		}

		if (RowStructure.Equals(StructTopLevelPath, ESearchCase::IgnoreCase)
			|| RowStructure.Equals(StructObjectPath, ESearchCase::IgnoreCase))
		{
			TableNames.Add(DataTableAsset.PackageName.ToString());
		}
	}

	TableNames.Sort();
	return TableNames.Num() > 0 ? FString::Join(TableNames, TEXT(", ")) : TEXT("None");
}

FString SBlueprintStructRepairWidget::GetDefaultLogPath() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectLogDir(),
		FString::Printf(TEXT("%s.log"), FApp::GetProjectName())));
}

bool SBlueprintStructRepairWidget::LoadLogLines(const FString& RequestedPath, TArray<FString>& OutLines, FString& OutReadPath, FString& OutDiagnostic) const
{
	OutLines.Reset();
	OutReadPath.Reset();
	OutDiagnostic.Reset();

	TArray<FString> CandidatePaths;
	auto AddCandidatePath = [&CandidatePaths](FString Path)
	{
		Path.TrimStartAndEndInline();
		Path.TrimQuotesInline();
		if (Path.IsEmpty())
		{
			return;
		}

		Path = FPaths::ConvertRelativePathToFull(Path);
		FPaths::NormalizeFilename(Path);
		CandidatePaths.AddUnique(Path);
	};

	AddCandidatePath(RequestedPath);
	AddCandidatePath(GetDefaultLogPath());

	const FString ProjectLogDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectLogDir());
	TArray<FString> LogFilenames;
	IFileManager::Get().FindFiles(LogFilenames, *FPaths::Combine(ProjectLogDir, TEXT("*.log")), true, false);
	LogFilenames.Sort([ProjectLogDir](const FString& A, const FString& B)
	{
		const FString APath = FPaths::IsRelative(A) ? FPaths::Combine(ProjectLogDir, A) : A;
		const FString BPath = FPaths::IsRelative(B) ? FPaths::Combine(ProjectLogDir, B) : B;
		return IFileManager::Get().GetTimeStamp(*APath) > IFileManager::Get().GetTimeStamp(*BPath);
	});

	for (const FString& LogFilename : LogFilenames)
	{
		AddCandidatePath(FPaths::IsRelative(LogFilename) ? FPaths::Combine(ProjectLogDir, LogFilename) : LogFilename);
	}

	TArray<FString> FailedPaths;
	for (const FString& CandidatePath : CandidatePaths)
	{
		if (!IFileManager::Get().FileExists(*CandidatePath))
		{
			FailedPaths.Add(FString::Printf(TEXT("missing: %s"), *CandidatePath));
			continue;
		}

		FString FileText;
		if (!ReadFileTextWithSharedAccess(CandidatePath, FileText))
		{
			FailedPaths.Add(FString::Printf(TEXT("read failed: %s"), *CandidatePath));
			continue;
		}

		FileText.ParseIntoArrayLines(OutLines, false);
		OutReadPath = CandidatePath;
		if (!CandidatePath.Equals(RequestedPath, ESearchCase::IgnoreCase))
		{
			OutDiagnostic = FString::Printf(TEXT("Used fallback log: %s"), *CandidatePath);
		}
		return true;
	}

	OutDiagnostic = FailedPaths.Num() > 0
		? FString::Printf(TEXT("Tried %d path(s). First result: %s"), FailedPaths.Num(), *FailedPaths[0])
		: TEXT("No log candidates were available.");
	return false;
}

bool SBlueprintStructRepairWidget::ReadFileTextWithSharedAccess(const FString& FilePath, FString& OutText) const
{
	OutText.Reset();

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenRead(*FilePath, true));
	if (!FileHandle)
	{
		return FFileHelper::LoadFileToString(OutText, *FilePath);
	}

	const int64 FileSize = FileHandle->Size();
	if (FileSize < 0 || FileSize > MAX_int32)
	{
		return false;
	}

	if (FileSize == 0)
	{
		return true;
	}

	TArray<uint8> Buffer;
	Buffer.SetNumUninitialized(static_cast<int32>(FileSize));
	if (!FileHandle->Read(Buffer.GetData(), Buffer.Num()))
	{
		return false;
	}

	FFileHelper::BufferToString(OutText, Buffer.GetData(), Buffer.Num());
	return true;
}

bool SBlueprintStructRepairWidget::IsErrorLine(const FString& Line) const
{
	return Line.Contains(TEXT("Error:"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT(": Error"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("Compiler Error"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("PackagingResults: Error"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("LogBlueprint: Error"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("Ensure condition failed"), ESearchCase::IgnoreCase);
}

bool SBlueprintStructRepairWidget::IsRelevantLogLine(const FString& Line) const
{
	if (IsErrorLine(Line))
	{
		return true;
	}

	if (Line.Contains(TEXT("Unknown structure"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("FStructProperty::Serialize"), ESearchCase::IgnoreCase)
		|| Line.Contains(TEXT("StructProperty::Serialize"), ESearchCase::IgnoreCase))
	{
		return true;
	}

	if (Line.Contains(TEXT("[AssetLog]"), ESearchCase::IgnoreCase)
		&& (Line.Contains(TEXT("UserDefinedStruct"), ESearchCase::IgnoreCase)
			|| Line.Contains(TEXT("K2Node_MakeStruct"), ESearchCase::IgnoreCase)
			|| Line.Contains(TEXT("StructProperty"), ESearchCase::IgnoreCase)
			|| Line.Contains(TEXT("Invalid GameplayTag TagName"), ESearchCase::IgnoreCase)))
	{
		return true;
	}

	return false;
}

void SBlueprintStructRepairWidget::ParseLogLines(
	const TArray<FString>& Lines,
	TMap<FString, FString>& OutPackageToFirstRelevantLine,
	TSet<FString>& OutReferencedLogPaths,
	int32& OutRelevantLineCount,
	int32& OutRawPackageCandidateCount) const
{
	for (int32 LineIndex = 0; LineIndex < Lines.Num(); ++LineIndex)
	{
		ExtractReferencedLogPathsFromLine(Lines[LineIndex], OutReferencedLogPaths);

		if (!IsRelevantLogLine(Lines[LineIndex]))
		{
			continue;
		}

		++OutRelevantLineCount;

		const int32 ContextRadius = IsErrorLine(Lines[LineIndex]) ? 8 : 0;
		const int32 FirstContextLine = FMath::Max(0, LineIndex - ContextRadius);
		const int32 LastContextLine = FMath::Min(Lines.Num() - 1, LineIndex + ContextRadius);
		for (int32 ContextLineIndex = FirstContextLine; ContextLineIndex <= LastContextLine; ++ContextLineIndex)
		{
			TArray<FString> PackageCandidates;
			ExtractPackageCandidatesFromLine(Lines[ContextLineIndex], PackageCandidates);
			OutRawPackageCandidateCount += PackageCandidates.Num();

			for (const FString& PackageName : PackageCandidates)
			{
				if (!OutPackageToFirstRelevantLine.Contains(PackageName))
				{
					OutPackageToFirstRelevantLine.Add(PackageName, Lines[LineIndex]);
				}
			}
		}
	}
}

void SBlueprintStructRepairWidget::ExtractPackageCandidatesFromLine(const FString& Line, TArray<FString>& OutPackageNames) const
{
	auto IsDelimiter = [](TCHAR Character)
	{
		return FChar::IsWhitespace(Character)
			|| Character == TEXT('"')
			|| Character == TEXT('\'')
			|| Character == TEXT('[')
			|| Character == TEXT(']')
			|| Character == TEXT('(')
			|| Character == TEXT(')')
			|| Character == TEXT('{')
			|| Character == TEXT('}')
			|| Character == TEXT(',')
			|| Character == TEXT(';');
	};

	auto AddCandidate = [this, &OutPackageNames](const FString& Candidate)
	{
		FString PackageName;
		if (NormalizePackageCandidate(Candidate, PackageName))
		{
			OutPackageNames.AddUnique(PackageName);
		}
	};

	int32 SearchIndex = 0;
	while (true)
	{
		const int32 GameIndex = Line.Find(TEXT("/Game/"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchIndex);
		if (GameIndex == INDEX_NONE)
		{
			break;
		}

		int32 EndIndex = GameIndex;
		while (EndIndex < Line.Len() && !IsDelimiter(Line[EndIndex]))
		{
			++EndIndex;
		}

		AddCandidate(Line.Mid(GameIndex, EndIndex - GameIndex));
		SearchIndex = EndIndex + 1;
	}

	FString NormalizedLine = Line;
	NormalizedLine.ReplaceInline(TEXT("\\"), TEXT("/"));

	SearchIndex = 0;
	while (true)
	{
		const int32 ExtensionIndex = NormalizedLine.Find(TEXT(".uasset"), ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchIndex);
		if (ExtensionIndex == INDEX_NONE)
		{
			break;
		}

		int32 ContentIndex = NormalizedLine.Find(TEXT("/Content/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd, ExtensionIndex);
		if (ContentIndex == INDEX_NONE)
		{
			ContentIndex = NormalizedLine.Find(TEXT("Content/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd, ExtensionIndex);
		}

		if (ContentIndex != INDEX_NONE)
		{
			AddCandidate(NormalizedLine.Mid(ContentIndex, ExtensionIndex + 7 - ContentIndex));
		}

		SearchIndex = ExtensionIndex + 7;
	}
}

void SBlueprintStructRepairWidget::ExtractReferencedLogPathsFromLine(const FString& Line, TSet<FString>& OutLogPaths) const
{
	FString NormalizedLine = Line;
	NormalizedLine.ReplaceInline(TEXT("\\"), TEXT("/"));

	auto ExtractPathsWithExtension = [&NormalizedLine, &OutLogPaths](const FString& Extension)
	{
		int32 SearchIndex = 0;
		while (true)
		{
			const int32 ExtensionIndex = NormalizedLine.Find(Extension, ESearchCase::IgnoreCase, ESearchDir::FromStart, SearchIndex);
			if (ExtensionIndex == INDEX_NONE)
			{
				break;
			}

			int32 StartIndex = INDEX_NONE;
			for (int32 CandidateStart = ExtensionIndex; CandidateStart >= 0; --CandidateStart)
			{
				if (CandidateStart + 2 < NormalizedLine.Len()
					&& FChar::IsAlpha(NormalizedLine[CandidateStart])
					&& NormalizedLine[CandidateStart + 1] == TEXT(':')
					&& NormalizedLine[CandidateStart + 2] == TEXT('/'))
				{
					StartIndex = CandidateStart;
					break;
				}
			}

			if (StartIndex != INDEX_NONE)
			{
				FString Path = NormalizedLine.Mid(StartIndex, ExtensionIndex + Extension.Len() - StartIndex);
				Path.TrimStartAndEndInline();
				Path.TrimQuotesInline();
				FPaths::NormalizeFilename(Path);
				OutLogPaths.Add(Path);
			}

			SearchIndex = ExtensionIndex + Extension.Len();
		}
	};

	ExtractPathsWithExtension(TEXT(".log"));
	ExtractPathsWithExtension(TEXT(".txt"));
}

bool SBlueprintStructRepairWidget::NormalizePackageCandidate(FString Candidate, FString& OutPackageName) const
{
	Candidate.TrimStartAndEndInline();
	Candidate.TrimQuotesInline();
	Candidate.ReplaceInline(TEXT("\\"), TEXT("/"));

	const int32 UAssetIndex = Candidate.Find(TEXT(".uasset"), ESearchCase::IgnoreCase);
	if (UAssetIndex != INDEX_NONE)
	{
		Candidate.LeftInline(UAssetIndex);
	}

	const int32 ContentMarkerIndex = Candidate.Find(TEXT("/Content/"), ESearchCase::IgnoreCase);
	if (ContentMarkerIndex != INDEX_NONE)
	{
		Candidate = TEXT("/Game/") + Candidate.RightChop(ContentMarkerIndex + 9);
	}
	else if (Candidate.StartsWith(TEXT("Content/"), ESearchCase::IgnoreCase))
	{
		Candidate = TEXT("/Game/") + Candidate.RightChop(8);
	}

	const int32 GameIndex = Candidate.Find(TEXT("/Game/"), ESearchCase::IgnoreCase);
	if (GameIndex != INDEX_NONE)
	{
		Candidate = Candidate.RightChop(GameIndex);
	}

	if (!Candidate.StartsWith(TEXT("/Game/"), ESearchCase::IgnoreCase))
	{
		return false;
	}

	const int32 ObjectSeparatorIndex = Candidate.Find(TEXT("."));
	if (ObjectSeparatorIndex != INDEX_NONE)
	{
		Candidate.LeftInline(ObjectSeparatorIndex);
	}

	const int32 SuffixSeparatorIndex = Candidate.Find(TEXT(":"));
	if (SuffixSeparatorIndex != INDEX_NONE)
	{
		Candidate.LeftInline(SuffixSeparatorIndex);
	}

	while (!Candidate.IsEmpty())
	{
		const TCHAR LastChar = Candidate[Candidate.Len() - 1];
		if (LastChar == TEXT('.') || LastChar == TEXT(':') || LastChar == TEXT(';') || LastChar == TEXT(',') || LastChar == TEXT(')') || LastChar == TEXT(']') || LastChar == TEXT('"') || LastChar == TEXT('\''))
		{
			Candidate.LeftChopInline(1);
			continue;
		}
		break;
	}

	if (Candidate.Len() <= 6 || Candidate.Contains(TEXT(" ")))
	{
		return false;
	}

	OutPackageName = Candidate;
	return true;
}

bool SBlueprintStructRepairWidget::ResolveBlueprintAsset(const FString& PackageName, FAssetData& OutAssetData) const
{
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().WaitForCompletion();

	TArray<FAssetData> AssetsInPackage;
	if (!AssetRegistryModule.Get().GetAssetsByPackageName(FName(*PackageName), AssetsInPackage))
	{
		return false;
	}

	for (const FAssetData& AssetData : AssetsInPackage)
	{
		UObject* Asset = AssetData.GetAsset();
		if (Cast<UBlueprint>(Asset))
		{
			OutAssetData = AssetData;
			return true;
		}
	}

	return false;
}

bool SBlueprintStructRepairWidget::ResolveUserDefinedStructAsset(const FString& PackageName, UUserDefinedStruct*& OutStruct) const
{
	OutStruct = nullptr;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().WaitForCompletion();

	TArray<FAssetData> AssetsInPackage;
	if (!AssetRegistryModule.Get().GetAssetsByPackageName(FName(*PackageName), AssetsInPackage))
	{
		return false;
	}

	for (const FAssetData& AssetData : AssetsInPackage)
	{
		UObject* Asset = AssetData.GetAsset();
		if (UUserDefinedStruct* Struct = Cast<UUserDefinedStruct>(Asset))
		{
			OutStruct = Struct;
			return true;
		}
	}

	return false;
}

bool SBlueprintStructRepairWidget::HasStructErrors(const FString& StructPackageName) const
{
	if (ErrorStructPackageNames.Contains(StructPackageName))
	{
		return true;
	}

	for (const TSharedPtr<FStructUsageInfo>& BatchStructInfo : BatchStructRows)
	{
		if (BatchStructInfo.IsValid() && BatchStructInfo->PackageName.Equals(StructPackageName, ESearchCase::IgnoreCase))
		{
			return true;
		}
	}

	return false;
}

void SBlueprintStructRepairWidget::RebuildStructRows()
{
	if (!StructRowsBox.IsValid())
	{
		return;
	}

	StructRowsBox->ClearChildren();

	StructRowsBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 2.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(0.27f)
		[
			SNew(STextBlock).Text(LOCTEXT("StructureColumn", "Structure"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.16f)
		[
			SNew(STextBlock).Text(LOCTEXT("ModifiedColumn", "Modified"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.08f)
		[
			SNew(STextBlock).Text(LOCTEXT("HasErrorsColumn", "Has Errors"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.33f)
		[
			SNew(STextBlock).Text(LOCTEXT("TablesColumn", "DataTables"))
		]
		+ SHorizontalBox::Slot()
		.FillWidth(0.18f)
		[
			SNew(STextBlock).Text(LOCTEXT("ActionsColumn", "Actions"))
		]
	];

	if (StructRows.Num() == 0)
	{
		StructRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("NoStructsFound", "No user-defined structs found in the selected Blueprint."))
		];
		return;
	}

	for (TSharedPtr<FStructUsageInfo> StructInfo : StructRows)
	{
		StructRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SBorder)
			.Padding(4.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.27f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(StructInfo->PackageName))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.16f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StructInfo->ModifiedText))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.08f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(StructInfo->bHasErrors ? TEXT("Yes") : TEXT("No")))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.33f)
				.VAlign(VAlign_Center)
				.Padding(4.0f, 0.0f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(StructInfo->TablesText))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(0.18f)
				.VAlign(VAlign_Center)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("ShowInFolderButton", "Show in Folder"))
						.OnClicked_Lambda([this, StructInfo]()
						{
							return ShowStructInFolder(StructInfo);
						})
					]
				]
			]
		];
	}
}

void SBlueprintStructRepairWidget::RebuildErrorAssetRows()
{
	if (!ErrorAssetRowsBox.IsValid())
	{
		return;
	}

	ErrorAssetRowsBox->ClearChildren();

	ErrorAssetRowsBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 2.0f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("ErrorAssetsHeader", "Unique Blueprint assets from error log"))
	];

	if (ErrorAssetRows.Num() == 0)
	{
		ErrorAssetRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("NoErrorAssets", "No parsed Blueprint assets yet."))
		];
		return;
	}

	for (const TSharedPtr<FErrorAssetInfo>& ErrorAsset : ErrorAssetRows)
	{
		if (!ErrorAsset.IsValid())
		{
			continue;
		}

		const bool bIsSelected = ErrorAsset->PackageName.Equals(SelectedErrorAssetPackageName, ESearchCase::IgnoreCase);
		const FString CompileStatusText = ErrorAsset->bRefreshCompileAttempted
			? ErrorAsset->CompileStatusText
			: TEXT("Refresh/Compile not run yet.");
		ErrorAssetRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SButton)
			.ContentPadding(0.0f)
			.ButtonColorAndOpacity(bIsSelected
				? FSlateColor(FLinearColor(0.16f, 0.28f, 0.48f, 1.0f))
				: FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)))
			.OnClicked_Lambda([this, ErrorAsset]()
			{
				return SelectErrorAsset(ErrorAsset);
			})
			[
				SNew(SBorder)
				.Padding(4.0f)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(FText::FromString(ErrorAsset->PackageName))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(FText::FromString(CompileStatusText))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock)
						.AutoWrapText(true)
						.Text(FText::FromString(ErrorAsset->FirstErrorLine.Left(320)))
					]
				]
			]
		];
	}
}

void SBlueprintStructRepairWidget::RebuildBatchStructRows()
{
	if (!BatchStructRowsBox.IsValid())
	{
		return;
	}

	BatchStructRowsBox->ClearChildren();

	BatchStructRowsBox->AddSlot()
	.AutoHeight()
	.Padding(0.0f, 0.0f, 0.0f, 2.0f)
	[
		SNew(STextBlock)
		.Text(LOCTEXT("BatchStructsHeader", "Unique structs from error log/assets"))
	];

	if (BatchStructRows.Num() == 0)
	{
		BatchStructRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 4.0f, 0.0f, 0.0f)
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("NoBatchStructs", "No collected structs yet."))
		];
		return;
	}

	for (TSharedPtr<FStructUsageInfo> StructInfo : BatchStructRows)
	{
		if (!StructInfo.IsValid())
		{
			continue;
		}

		BatchStructRowsBox->AddSlot()
		.AutoHeight()
		.Padding(0.0f, 2.0f)
		[
			SNew(SBorder)
			.Padding(4.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(StructInfo->PackageName))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 2.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.AutoWrapText(true)
					.Text(FText::FromString(FString::Printf(TEXT("Modified: %s | DataTables: %s"), *StructInfo->ModifiedText, *StructInfo->TablesText)))
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0.0f, 4.0f, 0.0f, 0.0f)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SButton)
						.Text(LOCTEXT("BatchShowInFolderButton", "Show in Folder"))
						.OnClicked_Lambda([this, StructInfo]()
						{
							return ShowStructInFolder(StructInfo);
						})
					]
				]
			]
		];
	}
}

void SBlueprintStructRepairWidget::ClearStructRows()
{
	StructRows.Reset();
	RebuildStructRows();
}

void SBlueprintStructRepairWidget::ClearBatchRows()
{
	ErrorAssetRows.Reset();
	BatchStructRows.Reset();
	ErrorStructPackageNames.Reset();
	SelectedErrorAssetPackageName.Reset();
	RebuildErrorAssetRows();
	RebuildBatchStructRows();
}

FText SBlueprintStructRepairWidget::GetStatusText() const
{
	return FText::FromString(StatusMessage);
}

bool SBlueprintStructRepairWidget::CanUseFoundAsset() const
{
	return bHasFoundAsset;
}

bool SBlueprintStructRepairWidget::CanUseSelectedErrorAsset() const
{
	return !SelectedErrorAssetPackageName.IsEmpty();
}

bool SBlueprintStructRepairWidget::CanUseErrorAssets() const
{
	return ErrorAssetRows.Num() > 0;
}

#undef LOCTEXT_NAMESPACE
