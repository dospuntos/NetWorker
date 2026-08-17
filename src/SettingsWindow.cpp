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
	BWindow(BRect(120, 120, 560, 420), "Settings", B_TITLED_WINDOW,
		B_NOT_ZOOMABLE | B_NOT_RESIZABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fSettings(settings),
	fClosedTarget(closedTarget)
{
	fCategoryList = new BListView("categoryList", B_SINGLE_SELECTION_LIST);
	fCategoryList->AddItem(new BStringItem("General"));
	fCategoryList->Select(0);
	fCategoryList->AddItem(new BStringItem("Variables"));
	fCategoryList->SetExplicitMinSize(BSize(120, B_SIZE_UNSET));

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

	AddShortcut(B_ESCAPE, 0, new BMessage(B_QUIT_REQUESTED));
}


BView*
SettingsWindow::_BuildGeneralPage()
{
	fSaveOnExitCheck = new BCheckBox("saveOnExit", "Save fields and history on exit",
		new BMessage(M_SETTINGS_CHANGED));
	fSaveOnExitCheck->SetValue(fSettings->fSaveFieldsOnExit ? B_CONTROL_ON : B_CONTROL_OFF);

	BStringView* warning = new BStringView("saveOnExitWarning",
		"Warning: this saves request fields, including passwords, tokens, "
		"and API keys, in a readable file on disk.\n"
		"Collections are always saved in readable files on disk.");

	fUserAgentField = new BTextControl("userAgent", "Default User-Agent",
		fSettings->fDefaultUserAgent.String(), new BMessage(M_SETTINGS_CHANGED));

	BString timeoutText;
	timeoutText << fSettings->fTimeoutSeconds;
	fTimeoutField = new BTextControl("timeout", "Timeout (seconds)", timeoutText.String(),
		new BMessage(M_SETTINGS_CHANGED));

	BString maxHistoryText;
	maxHistoryText << fSettings->fMaxHistoryItems;
	fMaxHistoryField = new BTextControl("maxHistory", "Max history items", maxHistoryText.String(),
		new BMessage(M_SETTINGS_CHANGED));

	fVerifySSLCheck
		= new BCheckBox("verifySSL", "Verify SSL certificates", new BMessage(M_SETTINGS_CHANGED));
	fVerifySSLCheck->SetValue(fSettings->fVerifySSL ? B_CONTROL_ON : B_CONTROL_OFF);

	fFollowRedirectsCheck
		= new BCheckBox("followRedirects", "Follow redirects", new BMessage(M_SETTINGS_CHANGED));
	fFollowRedirectsCheck->SetValue(fSettings->fFollowRedirects ? B_CONTROL_ON : B_CONTROL_OFF);

	BString maxSizeText;
	maxSizeText << (fSettings->fMaxResponseSize / (1024 * 1024));
	fMaxResponseSizeField = new BTextControl("maxResponseSize", "Max response size (MB)",
		maxSizeText.String(), new BMessage(M_SETTINGS_CHANGED));

	fWordWrapCheck
		= new BCheckBox("wordWrap", "Word wrap response/preview", new BMessage(M_SETTINGS_CHANGED));
	fWordWrapCheck->SetValue(fSettings->fWordWrap ? B_CONTROL_ON : B_CONTROL_OFF);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(fSaveOnExitCheck)
		.Add(warning)
		.Add(fUserAgentField)
		.Add(fTimeoutField)
		.Add(fMaxHistoryField)
		.Add(fVerifySSLCheck)
		.Add(fFollowRedirectsCheck)
		.Add(fMaxResponseSizeField)
		.Add(fWordWrapCheck)
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
	fSettings->fVerifySSL = (fVerifySSLCheck->Value() == B_CONTROL_ON);
	fSettings->fFollowRedirects = (fFollowRedirectsCheck->Value() == B_CONTROL_ON);
	fSettings->fWordWrap = (fWordWrapCheck->Value() == B_CONTROL_ON);

	int32 timeout = atoi(fTimeoutField->Text());
	fSettings->fTimeoutSeconds = timeout > 0 ? timeout : 1;

	int32 maxHistory = atoi(fMaxHistoryField->Text());
	fSettings->fMaxHistoryItems = maxHistory > 0 ? maxHistory : 1;

	int64 maxSizeMB = atoll(fMaxResponseSizeField->Text());
	fSettings->fMaxResponseSize = (maxSizeMB > 0 ? maxSizeMB : 1) * 1024 * 1024;

	fSettings->Save();
}


bool
SettingsWindow::QuitRequested()
{
	fClosedTarget.SendMessage(M_SETTINGS_WINDOW_CLOSED);
	return true;
}
