/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "RenameWindow.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <TextView.h>

enum { M_RENAME_OK = 'rnok', M_RENAME_CANCEL = 'rncn' };


RenameWindow::RenameWindow(BRect frame, const BString& initialText, int32 itemIndex,
	const BMessenger& target, uint32 resultWhat)
	:
	BWindow(frame, "Rename", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fItemIndex(itemIndex),
	fTarget(target),
	fResultWhat(resultWhat)
{
	fTextField = new BTextControl("renameField", nullptr, initialText.String(), nullptr);

	BButton* okButton = new BButton("ok", "OK", new BMessage(M_RENAME_OK));
	okButton->MakeDefault(true);
	BButton* cancelButton = new BButton("cancel", "Cancel", new BMessage(M_RENAME_CANCEL));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fTextField)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(cancelButton)
			.Add(okButton)
			.End()
		.End();

	AddShortcut(B_ESCAPE, 0, new BMessage(M_RENAME_CANCEL));

	fTextField->MakeFocus(true);
	fTextField->TextView()->SelectAll();
}


void
RenameWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_RENAME_OK:
		{
			BMessage result(fResultWhat);
			result.AddInt32("index", fItemIndex);
			result.AddString("label", fTextField->Text());
			fTarget.SendMessage(&result);
			PostMessage(B_QUIT_REQUESTED);
			break;
		}
		case M_RENAME_CANCEL:
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			BWindow::MessageReceived(message);
			break;
	}
}
