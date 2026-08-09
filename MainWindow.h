/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "Collection.h"
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

using BPrivate::Network::BHttpFields;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpSession;

class BCardLayout;
class BMenuField;
class BPopUpMenu;
class BRadioButton;
class BTextControl;
class BScrollView;
class BStringView;
class BSplitView;

static const uint32 M_DEBUG_TEST_COLLECTIONS = 'dtco';

class MainWindow : public BWindow {
public:
                        MainWindow();
    virtual             ~MainWindow();

    virtual void        MessageReceived(BMessage* message);
    virtual bool        QuitRequested();

private:
	BMenuBar*			_BuildMenu();
	void        		_BuildLayout();
	BView* 				_BuildRequestPanel();
	BView* 				_BuildAuthPanel();
	BView* 				_BuildParamsPanel();
	BView* 				_BuildPreviewPanel();
	BView* 				_BuildResponsePanel();
	BView* 				_BuildHistoryPanel();
	BView*				_BuildCollectionPanel();
	void        		_SendRequest();
	void        		_ClearResponse();

	BMenuBar*			fMenuBar;
    BPopUpMenu*         fMethodMenu;
    BMenuField*         fMethodField;
    BTextControl*       fUrlField;
    BButton*            fSendButton;
	BButton*			fSaveButton;
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

	BTabView*        	fBodyTabView;
	BTabView*			fResponseTabView;
	BColumnListView*  	fParamsList;
	BTextControl*     	fParamKeyField;
	BTextControl*     	fParamValueField;
	BButton*          	fParamAddButton;
	BButton*          	fParamRemoveButton;

	BRadioButton*   	fAuthNoneRadio;
	BRadioButton*   	fAuthBasicRadio;
	BRadioButton*   	fAuthBearerRadio;
	BRadioButton*   	fAuthApiKeyRadio;

	BCardLayout*    	fAuthCardLayout;

	BTextControl*   	fAuthUsernameField;
	BTextControl*   	fAuthPasswordField;
	BTextControl*   	fAuthTokenField;
	BTextControl*   	fAuthApiKeyNameField;
	BTextControl*   	fAuthApiKeyValueField;

	BTabView*        fSidebarTabs;

	BMenuField*      fCollectionMenuField;
	BPopUpMenu*      fCollectionMenu;
	BListView*       fCollectionListView;
	BButton*         fNewCollectionButton;
	BButton*         fDeleteCollectionButton;

	BObjectList<Collection, true> fCollections;
	int32            fActiveCollectionIndex;

	status_t _SaveSettings();
	status_t	_LoadSettings(BMessage& settings);
	void _RestoreValues(BMessage& settings);
	void _LoadRequestData(const RequestData& data);
	void _UpdateHistoryButtons();
	void _UpdatePreview();
	void _ApplyAuth(BHttpFields& fields);
	BString _CurrentAuthType() const;
	BMessage _CurrentAuthValues() const;

	status_t _CollectionsDirectory(BPath& path);
	status_t _SaveCollection(Collection* collection);
	status_t _LoadCollection(const BString& fileName, Collection*& outCollection);
	status_t _SaveCollectionsIndex();
	status_t _LoadCollectionsIndex();

	void _RefreshCollectionMenu();
	void _RefreshCollectionItemList();
};

#endif
