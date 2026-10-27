#include "CXChatWidget.h"
#include "CXPlayerController.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

void UCXChatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ChatInput)
	{
		ChatInput->OnTextCommitted.AddUniqueDynamic(this, &UCXChatWidget::HandleTextCommitted);
	}
}

void UCXChatWidget::FocusChatInput()
{
	if (ChatInput)
	{
		ChatInput->SetUserFocus(GetOwningPlayer());
		ChatInput->SetKeyboardFocus();
	}
}

void UCXChatWidget::AddChatMessage(const FString& Message)
{
	if (!ChatHistory)
	{
		return;
	}

	UTextBlock* MessageText = NewObject<UTextBlock>(this);
	MessageText->SetText(FText::FromString(Message));
	MessageText->SetAutoWrapText(true);
	ChatHistory->AddChild(MessageText);

	while (ChatHistory->GetChildrenCount() > 100)
	{
		ChatHistory->RemoveChildAt(0);
	}
	ChatHistory->ScrollToEnd();
}

void UCXChatWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	const FString Message = Text.ToString();
	ChatInput->SetText(FText::GetEmpty());

	if (ACXPlayerController* Controller = Cast<ACXPlayerController>(GetOwningPlayer()))
	{
		Controller->SubmitChatMessage(Message);
		Controller->CloseChat();
	}
}

FReply UCXChatWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (ACXPlayerController* Controller = Cast<ACXPlayerController>(GetOwningPlayer()))
		{
			Controller->CloseChat();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

