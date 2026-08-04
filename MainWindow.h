/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "HistoryItem.h"
#include <optional>

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <HttpFields.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <ListView.h>
#include <MenuBar.h>
#include <TabView.h>
#include <TextView.h>
#include <Window.h>

using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpFields;

class BCardLayout;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BTextControl;
class BScrollView;
class BStringView;
class BSplitView;


class MainWindow : public BWindow {
public:
                        MainWindow();
    virtual             ~MainWindow();

    virtual void        MessageReceived(BMessage* message);
    virtual bool        QuitRequested();

private:
			BMenuBar*	_BuildMenu();
            void        _BuildLayout();
            void        _SendRequest();
            void        _ClearResponse();

	BMenuBar*			fMenuBar;
    BPopUpMenu*         fMethodMenu;
    BMenuField*         fMethodField;
    BTextControl*       fUrlField;
    BButton*            fSendButton;
    BTextView*          fRequestBodyView;
    BScrollView*        fRequestBodyScroll;
	BTextView*			fPreviewPanel;
	BScrollView*		fPreviewPanelScroll;
    BStringView*        fStatusLabel;
    BColumnListView*    fResponseHeadersList;
    BTextView*          fResponseBodyView;
    BScrollView*        fResponseBodyScroll;
    BSplitView*         fSplitView;
	BSplitView*			fRequestAreaSplit;
    BHttpSession                fSession;
    std::optional<BHttpResult>  fCurrentResult;
	BString fPendingRequestBody;
	BListView* 			fHistoryPanel;
	BButton*			fClearHistoryBtn;
	BButton*			fRemoveItemBtn;

	BTabView*        	fBodyTabView;
	BTabView*			fResponseTabView;
	BColumnListView*  	fParamsList;
	BTextControl*     	fParamKeyField;
	BTextControl*     	fParamValueField;
	BButton*          	fParamAddButton;
	BButton*          	fParamRemoveButton;

	BRadioButton*   fAuthNoneRadio;
	BRadioButton*   fAuthBasicRadio;
	BRadioButton*   fAuthBearerRadio;
	BRadioButton*   fAuthApiKeyRadio;

	BCardLayout*    fAuthCardLayout;

	BTextControl*   fAuthUsernameField;
	BTextControl*   fAuthPasswordField;
	BTextControl*   fAuthTokenField;
	BTextControl*   fAuthApiKeyNameField;
	BTextControl*   fAuthApiKeyValueField;

	static BString _UrlEncode(const BString& value);
	status_t _SaveSettings();
	status_t	_LoadSettings(BMessage& settings);
	void _RestoreValues(BMessage& settings);
	void _LoadHistoryItem(HistoryItem* item);
	void _UpdateHistoryButtons();
	void _UpdatePreview();
	void _ApplyAuth(BHttpFields& fields);
	BString _CurrentAuthType() const;
	BMessage _CurrentAuthValues() const;
};

#endif