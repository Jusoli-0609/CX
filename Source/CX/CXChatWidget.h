#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/EditableTextBox.h"
#include "CXChatWidget.generated.h"

class UScrollBox;

UCLASS()
class CX_API UCXChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void FocusChatInput();
	void AddChatMessage(const FString& Message);

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UScrollBox> ChatHistory;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInput;

private:
	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);
};

