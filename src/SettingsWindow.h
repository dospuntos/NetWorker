/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Messenger.h>
#include <Spinner.h>
#include <Window.h>

class AppSettings;
class BListView;
class BCardLayout;
class BCheckBox;
class BTextControl;

class SettingsWindow : public BWindow {
public:
    explicit SettingsWindow(AppSettings* settings, const BMessenger& closedTarget);

    void MessageReceived(BMessage* message) override;
    bool QuitRequested() override;

private:
    BView* _BuildGeneralPage();
	BView* _BuildVariablesPage();
    void _ApplyAndSave();

    AppSettings*   fSettings;
	BMessenger     fClosedTarget;
    BListView*     fCategoryList;
    BCardLayout*   fCardLayout;

    BCheckBox*     fSaveOnExitCheck;
    BTextControl*  fUserAgentField;
    BSpinner*	   fTimeoutField;
    BSpinner*  	   fMaxHistoryField;
    BCheckBox*     fVerifySSLCheck;
    BCheckBox*     fFollowRedirectsCheck;
    BSpinner*	   fMaxResponseSizeField;
    BCheckBox*     fWordWrapCheck;
};

#endif // SETTINGS_WINDOW_H
