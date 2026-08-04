/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "HistoryItem.h"
#include "Constants.h"
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

class BMenuField;
class BPopUpMenu;
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

	static BString _UrlEncode(const BString& value);
	status_t _SaveSettings();
	status_t	_LoadSettings(BMessage& settings);
	void _RestoreValues(BMessage& settings);
	void _LoadHistoryItem(HistoryItem* item);
	void _UpdateHistoryButtons();
	void _UpdatePreview();

};


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


class PreviewTabView : public BTabView {
public:
    PreviewTabView(const char* name) : BTabView(name) {}

    void Select(int32 index) override
    {
        BTabView::Select(index);
        if (Window())
            Window()->PostMessage(M_UPDATE_PREVIEW);
    }
};

#endif