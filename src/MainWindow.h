/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "AppSettings.h"
#include "AuthPanel.h"
#include "BodyPanel.h"
#include "Collection.h"
#include "HeadersPanel.h"
#include "KeyValueEditor.h"
#include "MenuBar.h"
#include "SettingsWindow.h"
#include <optional>

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <DataIO.h>
#include <ExclusiveBorrow.h>
#include <HttpFields.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <ListView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TabView.h>
#include <TextView.h>
#include <Window.h>

using BPrivate::Network::BExclusiveBorrow;
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
class BMessageRunner;

// Status colors for response messages
enum class StatusType { None, Success, Warning, Error, Notice };

class MainWindow : public BWindow {
public:
						MainWindow();
	virtual				~MainWindow();

	virtual void		MessageReceived(BMessage* message);
	virtual bool		QuitRequested();

private:
	// Layout
	void				_BuildLayout();
	BView*				_BuildRequestPanel();
	BView*				_BuildPreviewPanel();
	BView*				_BuildResponsePanel();
	BView*				_BuildHistoryPanel();
	BView*				_BuildCollectionPanel();
	BView*              _BuildStatusBar();
    void                _SetStatus(BString text);
	void				_SetResponseStatus(const char* text, StatusType type = StatusType::None);

	// Request handling
	void				_SendRequest();
	void				_HandleRequestCompleted(BMessage* message);
	void				_CancelCurrentRequest();
	void				_StopRequestTimeout();
	void				_LoadRequestData(const RequestData& data);
	BString				_BuildFullUrl() const;
	BMessage			_ComputedHeaders() const;
	void				_ApplyDefaultUserAgent();

	// Response handling
	void				_ClearResponse();
	void				_DisplayResponseBody();
	void				_UpdateResponseActions();
	bool				_IsTextResponse() const;
	bool				_IsJsonResponse() const;
	BString				_FormatFileSize(int64 bytes);

	// Preview
	void				_UpdatePreview();

	// History
	void                _HandleHistorySelection();
	void                _DeleteSelectedHistoryItems();
	void                _UpdateHistoryButtons();
	void                _CopyHistoryUrl();
	void                _ShowRenameHistoryDialog();
	void                _RenameHistoryItem(BMessage* message);
	void                _ClearHistory();
	void                _TrimHistory();

	// Collections
	void                _SelectCollection(BMessage* message);
	void                _ShowNewCollectionDialog();
	void                _CreateCollection(BMessage* message);
	void                _DeleteCollection();
	void                _ConfirmDeleteCollection(BMessage* message);
	void                _SaveToCollection();
	void                _LoadCollectionItem();
	void                _CopyCollectionUrl();
	void                _ShowRenameCollectionItemDialog();
	void                _RenameCollectionItem(BMessage* message);
	void                _DeleteCollectionItem();

	void                _ShowExportCollectionPanel();
	void                _ExportCollection(BMessage* message);
	void                _ShowImportCollectionPanel();
	void                _ImportCollection(BMessage* message);

	status_t            _CollectionsDirectory(BPath& path);
	status_t            _SaveCollection(Collection* collection);
	status_t            _LoadCollection(const BString& fileName,
							Collection*& outCollection);
	status_t            _SaveCollectionsIndex();
	status_t            _LoadCollectionsIndex();
	void                _RefreshCollectionMenu();
	void                _RefreshCollectionItemList();

	// Clipboard
	status_t            _CopyTextToClipboard(const BString& text);

	// Settings
	status_t			_SaveSettings();
	status_t			_LoadSettings(BMessage& settings);
	void				_RestoreValues(BMessage& settings);
	void				_ApplySettings();


private:
	// Settings
	AppSettings*		fAppSettings;
	SettingsWindow*		fSettingsWindow;

	// Menu
	MenuBar*			fMenuBar;
	BPopUpMenu*			fMethodMenu;
	BMenuField*			fMethodField;

	// Request controls
	BTextControl*		fUrlField;
	BButton*			fSendButton;
	BButton*			fSaveButton;
	BStringView*		fResponseStatusLabel;

	// Request panels
	BodyPanel*			fBodyPanel;
	AuthPanel*			fAuthPanel;
	HeadersPanel*		fHeadersPanel;
	KeyValueEditor*		fQueryParamsEditor;
	BTabView*			fBodyTabView;

	// Request state
	BHttpSession		fSession;
	std::optional<BHttpResult> fCurrentResult;
	BString				fPendingRequestBody;
	BMessageRunner*		fRequestTimeoutRunner = nullptr;

	// Preview
	BTextView*			fPreviewPanel;
	BScrollView*		fPreviewPanelScroll;

	// Response controls
	BTabView*			fResponseTabView;
	BColumnListView*	fResponseHeadersList;
	BTextView*			fResponseBodyView;
	BScrollView*		fResponseBodyScroll;
	BCheckBox*			fPrettifyCheck;
	BButton*			fOpenResponseButton;
	BButton*			fCopyResponseButton;
	BButton*			fSaveResponseButton;
	BFilePanel*			fSaveResponseFilePanel;

	// Response state
	BExclusiveBorrow<BMallocIO> fResponseBuffer;
	BString				fRawResponseBody;
	BString				fContentType;
	int64				fContentLength = -1;
	bool				fHasCompletedResponse = false;

	// History
	BListView*			fHistoryPanel;
	BButton*			fClearHistoryBtn;

	// Collections
	BMenuField*			fCollectionMenuField;
	BPopUpMenu*			fCollectionMenu;
	BListView*			fCollectionListView;
	BButton*			fNewCollectionButton;
	BButton*			fDeleteCollectionButton;
	BFilePanel*			fExportFilePanel;
	BFilePanel*			fImportFilePanel;

	BObjectList<Collection, true> fCollections;
	Collection*			fPendingExportCollection;
	int32				fActiveCollectionIndex;

	// Layout
	BSplitView*			fSplitView;
	BSplitView*			fRequestAreaSplit;
	BSplitView*			fOuterSplit;
	BTabView*			fSidebarTabs;
	BStringView*        fStatusLabel;
    BStringView*        fStatusDetailsLabel;
};

#endif
