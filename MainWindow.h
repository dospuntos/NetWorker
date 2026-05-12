/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Window.h>
#include <HttpFields.h>
#include <HttpSession.h>
#include <HttpResult.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <MenuBar.h>

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
};

#endif