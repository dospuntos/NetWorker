/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <optional>

#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <HttpFields.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <MenuBar.h>
#include <TabView.h>
#include <Window.h>

using BPrivate::Network::BHttpSession;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpFields;

class BButton;
class BMenuField;
class BPopUpMenu;
class BTextControl;
class BTextView;
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
    BStringView*        fStatusLabel;
    BColumnListView*    fResponseHeadersList;
    BTextView*          fResponseBodyView;
    BScrollView*        fResponseBodyScroll;
    BSplitView*         fSplitView;
    BHttpSession                fSession;
    std::optional<BHttpResult>  fCurrentResult;
	BString fPendingRequestBody;

	BTabView*        	fBodyTabView;
	BColumnListView*  	fParamsList;
	BTextControl*     	fParamKeyField;
	BTextControl*     	fParamValueField;
	BButton*          	fParamAddButton;
	BButton*          	fParamRemoveButton;

	static BString _UrlEncode(const BString& value);
};

#endif