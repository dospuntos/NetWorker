/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SettingsWindow.h"

#include <Alert.h>
#include <Box.h>
#include <Button.h>
#include <CardLayout.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>
#include <ListView.h>
#include <ScrollView.h>
#include <StringItem.h>
#include <StringView.h>
#include <TextControl.h>
#include <TextView.h>

#include "AppSettings.h"
#include "Constants.h"


SettingsWindow::SettingsWindow(AppSettings* settings, const BMessenger& closedTarget)
	:
	BWindow(BRect(120, 120, 460, 420), "Settings", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE),
	fSettings(settings),
	fClosedTarget(closedTarget)
{
	fCategoryList = new BListView("categoryList", B_SINGLE_SELECTION_LIST);
	fCategoryList->AddItem(new BStringItem("General"));
	fCategoryList->AddItem(new BStringItem("Variables"));
	fCategoryList->Select(0);
	fCategoryList->SetExplicitMinSize(BSize(150, B_SIZE_UNSET));
	fCategoryList->SetSelectionMessage(new BMessage(M_SETTINGS_CATEGORY_SELECTED));

	BScrollView* categoryScroll = new BScrollView("categoryScroll", fCategoryList,
		B_WILL_DRAW | B_FRAME_EVENTS, false, false);

	BView* cardsView = new BView("settingsCards", 0);
	fCardLayout = new BCardLayout();
	cardsView->SetLayout(fCardLayout);
	fCardLayout->AddView(_BuildGeneralPage());
	fCardLayout->AddView(_BuildVariablesPage());

	BButton* closeButton = new BButton("close", "Close", new BMessage(B_QUIT_REQUESTED));
	closeButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(categoryScroll)
			.Add(cardsView)
			.End()
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.AddGlue()
			.Add(closeButton)
			.End();
}


BView*
SettingsWindow::_BuildGeneralPage()
{
	fSaveOnExitCheck = new BCheckBox("saveOnExit", "Save fields and history on exit",
		new BMessage(M_SETTINGS_CHANGED));
	fSaveOnExitCheck->SetValue(fSettings->fSaveFieldsOnExit ? B_CONTROL_ON : B_CONTROL_OFF);

	BStringView* warning = new BStringView("saveOnExitWarning",
		"Warning: this saves request fields, including passwords,\n"
		"tokens, and API keys, in a readable file on disk.\n"
		"Collections are always saved in readable files on disk.");

	fWordWrapCheck
		= new BCheckBox("wordWrap", "Word wrap response/preview", new BMessage(M_SETTINGS_CHANGED));
	fWordWrapCheck->SetValue(fSettings->fWordWrap ? B_CONTROL_ON : B_CONTROL_OFF);

	fUserAgentField = new BTextControl("userAgent", "Default User-Agent",
		fSettings->fDefaultUserAgent.String(), new BMessage(M_SETTINGS_CHANGED));

	fTimeoutField = new BSpinner("timeout", "Timeout (seconds)", new BMessage(M_SETTINGS_CHANGED));
	fTimeoutField->SetMinValue(0);
	fTimeoutField->SetValue(fSettings->fTimeoutSeconds);

	fMaxHistoryField
		= new BSpinner("maxHistory", "Max history items", new BMessage(M_SETTINGS_CHANGED));
	fMaxHistoryField->SetMinValue(0);
	fMaxHistoryField->SetValue(fSettings->fMaxHistoryItems);

	fMaxResponseSizeField = new BSpinner("maxResponseSize", "Max response size (MB)",
		new BMessage(M_SETTINGS_CHANGED));
	fMaxResponseSizeField->SetMinValue(0);
	fMaxResponseSizeField->SetValue(fSettings->fMaxResponseSize / (1024 * 1024));

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fSaveOnExitCheck)
		.Add(warning)
		.Add(fWordWrapCheck)
		.AddGrid(B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
			.Add(fUserAgentField->CreateLabelLayoutItem(), 0, 0)
			.Add(fUserAgentField->CreateTextViewLayoutItem(), 1, 0)
			.Add(fTimeoutField->CreateLabelLayoutItem(), 0, 1)
			.Add(fTimeoutField->CreateTextViewLayoutItem(), 1, 1)
			.Add(fMaxHistoryField->CreateLabelLayoutItem(), 0, 2)
			.Add(fMaxHistoryField->CreateTextViewLayoutItem(), 1, 2)
			.Add(fMaxResponseSizeField->CreateLabelLayoutItem(), 0, 3)
			.Add(fMaxResponseSizeField->CreateTextViewLayoutItem(), 1, 3)
			.End()
		.AddGlue()
		.View();
}


BView*
SettingsWindow::_BuildVariablesPage()
{

	BStringView* warning = new BStringView("variablesNote",
		"Variables is a planned feature, but has not been implemented yet.");

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(warning)
		.AddGlue()
		.View();
}


void
SettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case M_SETTINGS_CHANGED:
			_ApplyAndSave();
			break;
		case M_SETTINGS_CATEGORY_SELECTED:
		{
			int32 index = fCategoryList->CurrentSelection();
			if (index >= 0)
				fCardLayout->SetVisibleItem(index);
			break;
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
SettingsWindow::_ApplyAndSave()
{
	fSettings->fSaveFieldsOnExit = (fSaveOnExitCheck->Value() == B_CONTROL_ON);
	fSettings->fDefaultUserAgent = fUserAgentField->Text();
	// fSettings->fVerifySSL = (fVerifySSLCheck->Value() == B_CONTROL_ON);
	// fSettings->fFollowRedirects = (fFollowRedirectsCheck->Value() == B_CONTROL_ON);
	fSettings->fWordWrap = (fWordWrapCheck->Value() == B_CONTROL_ON);

	int32 timeout = fTimeoutField->Value();
	fSettings->fTimeoutSeconds = timeout >= 0 ? timeout : 1;

	int32 maxHistory = fMaxHistoryField->Value();
	fSettings->fMaxHistoryItems = maxHistory > 0 ? maxHistory : 1;

	int64 maxSizeMB = fMaxResponseSizeField->Value();
	fSettings->fMaxResponseSize = maxSizeMB > 0 ? maxSizeMB * 1024 * 1024 : 0;

	fSettings->Save();
	fClosedTarget.SendMessage(M_SETTINGS_APPLIED);
}


bool
SettingsWindow::QuitRequested()
{
	fClosedTarget.SendMessage(M_SETTINGS_WINDOW_CLOSED);
	return true;
}
