#pragma once

#include "AssetRegistry/AssetData.h"
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SVerticalBox;
class UBlueprint;
class UUserDefinedStruct;
struct FEdGraphPinType;

struct FStructUsageInfo
{
	TWeakObjectPtr<UUserDefinedStruct> Struct;
	FString Name;
	FString PackageName;
	FString ModifiedText;
	FString TablesText;
	bool bHasErrors = false;
};

struct FErrorAssetInfo
{
	FAssetData AssetData;
	FString PackageName;
	FString FirstErrorLine;
	FString CompileStatusText;
	bool bRefreshCompileAttempted = false;
	bool bCompileSucceeded = false;
	int32 CompileErrorCount = 0;
};

class SBlueprintStructRepairWidget final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBlueprintStructRepairWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FReply FindAsset();
	FReply OpenAsset();
	FReply RefreshNodes();
	FReply ShowStructInFolder(TSharedPtr<FStructUsageInfo> StructInfo);
	FReply ParseBuildLog();
	FReply SelectErrorAsset(TSharedPtr<FErrorAssetInfo> ErrorAsset);
	FReply RefreshSelectedErrorAsset();
	FReply RefreshErrorAssets();
	FReply CollectStructsFromErrorAssets();

	bool TryFindBlueprintAsset(const FString& Query, FAssetData& OutAssetData, int32& OutMatchCount) const;
	void FindBlueprintAssets(TArray<FAssetData>& OutAssets) const;
	FString NormalizeSearchText(FString SearchText) const;
	bool DoesAssetMatch(const FAssetData& AssetData, const FString& SearchText) const;
	UBlueprint* GetFoundBlueprint() const;
	FString GetUnsavedChangesText(const UBlueprint* Blueprint) const;
	bool RefreshAndCompileBlueprint(UBlueprint* Blueprint, FString& OutMessage, int32* OutErrorCount = nullptr) const;
	void ApplyRefreshCompileResult(TSharedPtr<FErrorAssetInfo> ErrorAsset, const FString& Message, bool bSucceeded, int32 ErrorCount) const;

	void AnalyzeUsedStructs();
	void CollectUsedStructs(UBlueprint* Blueprint, TSet<UUserDefinedStruct*>& OutStructs) const;
	void AddStructFromPinType(const FEdGraphPinType& PinType, TSet<UUserDefinedStruct*>& OutStructs) const;
	TSharedPtr<FStructUsageInfo> MakeStructInfo(UUserDefinedStruct* Struct) const;
	FString GetPackageModifiedText(const FString& PackageName) const;
	FString GetTablesUsingStructText(const UUserDefinedStruct* Struct) const;
	FString GetDefaultLogPath() const;
	bool LoadLogLines(const FString& RequestedPath, TArray<FString>& OutLines, FString& OutReadPath, FString& OutDiagnostic) const;
	bool ReadFileTextWithSharedAccess(const FString& FilePath, FString& OutText) const;
	bool IsErrorLine(const FString& Line) const;
	bool IsRelevantLogLine(const FString& Line) const;
	void ParseLogLines(const TArray<FString>& Lines, TMap<FString, FString>& OutPackageToFirstRelevantLine, TSet<FString>& OutReferencedLogPaths, int32& OutRelevantLineCount, int32& OutRawPackageCandidateCount) const;
	void ExtractPackageCandidatesFromLine(const FString& Line, TArray<FString>& OutPackageNames) const;
	void ExtractReferencedLogPathsFromLine(const FString& Line, TSet<FString>& OutLogPaths) const;
	bool NormalizePackageCandidate(FString Candidate, FString& OutPackageName) const;
	bool ResolveBlueprintAsset(const FString& PackageName, FAssetData& OutAssetData) const;
	bool ResolveUserDefinedStructAsset(const FString& PackageName, UUserDefinedStruct*& OutStruct) const;
	bool HasStructErrors(const FString& StructPackageName) const;
	void RebuildStructRows();
	void RebuildErrorAssetRows();
	void RebuildBatchStructRows();
	void ClearStructRows();
	void ClearBatchRows();

	FText GetStatusText() const;
	bool CanUseFoundAsset() const;
	bool CanUseSelectedErrorAsset() const;
	bool CanUseErrorAssets() const;

private:
	TSharedPtr<SEditableTextBox> SearchTextBox;
	TSharedPtr<SEditableTextBox> LogPathTextBox;
	TSharedPtr<SVerticalBox> StructRowsBox;
	TSharedPtr<SVerticalBox> ErrorAssetRowsBox;
	TSharedPtr<SVerticalBox> BatchStructRowsBox;
	TArray<TSharedPtr<FStructUsageInfo>> StructRows;
	TArray<TSharedPtr<FErrorAssetInfo>> ErrorAssetRows;
	TArray<TSharedPtr<FStructUsageInfo>> BatchStructRows;
	TSet<FString> ErrorStructPackageNames;
	FAssetData FoundAssetData;
	bool bHasFoundAsset = false;
	FString SelectedErrorAssetPackageName;
	FString StatusMessage;
};
