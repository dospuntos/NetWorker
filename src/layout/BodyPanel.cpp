/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "BodyPanel.h"
#include "Constants.h"

#include <Button.h>
#include <File.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <Node.h>
#include <NodeInfo.h>
#include <ScrollView.h>
#include <TextControl.h>
#include <TextView.h>
#include <Url.h>
#include <View.h>
#include <Window.h>

namespace {

class PreviewTextView : public BTextView {
public:
    PreviewTextView(const char* name) : BTextView(name) {}

    void InsertText(const char* text, int32 length, int32 offset,
        const text_run_array* runs) override
    {
        BTextView::InsertText(text, length, offset, runs);
        if (Window())
            Window()->PostMessage(M_UPDATE_PREVIEW);
    }

    void DeleteText(int32 start, int32 finish) override
    {
        BTextView::DeleteText(start, finish);
        if (Window())
            Window()->PostMessage(M_UPDATE_PREVIEW);
    }
};

} // namespace


BodyPanel::BodyPanel()
	:
	fFilePanel(nullptr)
{
	fRadioGroup = new RadioCardGroup();

	// None
	BView* noneCard = new BView("bodyNoneCard", B_WILL_DRAW);
	fRadioGroup->AddOption("None", noneCard); // index 0

	// Raw
	fRawBodyView = new PreviewTextView("requestBody");
	BScrollView* rawScroll = new BScrollView("requestBodyScroll", fRawBodyView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	fRadioGroup->AddOption("Raw", rawScroll); // index 1

	// Form
	fFormEditor = new KeyValueEditor("Key", "Value");
	fRadioGroup->AddOption("Form", fFormEditor->View()); // index 2

	// File
	fFilePathField = new BTextControl("bodyFilePath", "File", "", nullptr);
	fFilePathField->SetEnabled(false); // path is set via Browse, not typed
	fBrowseButton
		= new BButton("bodyBrowse", "Browse" B_UTF8_ELLIPSIS, new BMessage(M_NOT_IMPLEMENTED));
	BView* fileCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
						  .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
							  .Add(fFilePathField)
							  .Add(fBrowseButton)
							  .End()
						  .AddGlue()
						  .View();
	fRadioGroup->AddOption("File", fileCard); // index 3
}


BodyPanel::~BodyPanel()
{
	delete fRadioGroup;
	delete fFormEditor;
	delete fFilePanel;
}


void
BodyPanel::SetTarget(BHandler* target, uint32 modeChangedWhat)
{
	fRadioGroup->SetTarget(target, modeChangedWhat);
}


void
BodyPanel::UpdateVisibleCard()
{
	fRadioGroup->UpdateVisibleCard();
}


void
BodyPanel::ShowFilePanel(BHandler* target, uint32 refsReceivedWhat)
{
	if (fFilePanel == nullptr) {
		BMessenger messenger(target);
		fFilePanel = new BFilePanel(B_OPEN_PANEL, &messenger, nullptr, 0, false);
	}
	fFilePanel->Show();
}


void
BodyPanel::SetFilePath(const BString& path)
{
	fFilePath = path;
	fFilePathField->SetText(path.String());
}


BString
BodyPanel::CurrentMode() const
{
	switch (fRadioGroup->SelectedIndex()) {
		case 1:
			return "raw";
		case 2:
			return "form";
		case 3:
			return "file";
		default:
			return "none";
	}
}


BString
BodyPanel::CurrentBody() const
{
	BString mode = CurrentMode();

	if (mode == "raw") {
		return BString(fRawBodyView->Text());

	} else if (mode == "form") {
		return fFormEditor->FormEncodedValues();

	} else if (mode == "file") {
		if (fFilePath.Length() == 0)
			return "";
		BFile file(fFilePath.String(), B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			return "";
		off_t size = 0;
		file.GetSize(&size);
		BString content;
		char* buffer = content.LockBuffer((int32)size);
		file.Read(buffer, size);
		content.UnlockBuffer((int32)size);
		return content;
	}

	return ""; // none
}


BString
BodyPanel::CurrentContentType() const
{
	BString mode = CurrentMode();

	if (mode == "raw")
		return "application/json";
	if (mode == "form")
		return "application/x-www-form-urlencoded";
	if (mode == "file") {
		if (fFilePath.Length() == 0)
			return "application/octet-stream";
		BNode node(fFilePath.String());
		BNodeInfo info(&node);
		char mimeType[B_MIME_TYPE_LENGTH];
		if (info.GetType(mimeType) == B_OK)
			return BString(mimeType);
		return "application/octet-stream";
	}

	return ""; // none
}


void
BodyPanel::LoadFrom(const BString& mode, const BString& rawBody, const BMessage& formParams,
	const BString& filePath)
{
	fRawBodyView->SetText(rawBody.String());
	fFormEditor->LoadFrom(formParams);
	SetFilePath(filePath);

	int32 index = 0;
	if (mode == "raw")
		index = 1;
	else if (mode == "form")
		index = 2;
	else if (mode == "file")
		index = 3;

	fRadioGroup->SetSelectedIndex(index);
}
