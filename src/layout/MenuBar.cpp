/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MenuBar.h"

#include <Application.h>
#include <Catalog.h>
#include <Menu.h>
#include <MenuItem.h>

#include "Constants.h"
#include "IconMenuItem.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainView"


MenuBar::MenuBar()
	:
	BMenuBar("menubar")
{
	BMenu* menu;
	BMenuItem* item;

	// App menu
	menu = new BMenu("");
	item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Help" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_HELP), 'H'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Report a bug" B_UTF8_ELLIPSIS), new BMessage(M_REPORT_A_BUG)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(M_SHOW_SETTINGS), ',', B_COMMAND_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	IconMenuItem* iconMenu = new IconMenuItem(menu, NULL, kApplicationSignature, B_MINI_ICON);
	AddItem(iconMenu);


	// Request menu
	menu = new BMenu(B_TRANSLATE("Request"));

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Send request"), new BMessage(M_SEND_REQUEST), B_ENTER));

	menu->AddItem(new BMenuItem(B_TRANSLATE("New request"), new BMessage(M_NEW_REQUEST), 'N'));
	menu->AddSeparatorItem();

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Clear response"), new BMessage(M_CLEAR_RESPONSE), 'D'));

	AddItem(menu);


	// Collections menu
	menu = new BMenu(B_TRANSLATE("Collections"));

	menu->AddItem(new BMenuItem(B_TRANSLATE("New collection" B_UTF8_ELLIPSIS),
		new BMessage(M_NEW_COLLECTION), 'N', B_OPTION_KEY));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Delete collection" B_UTF8_ELLIPSIS),
		new BMessage(M_DELETE_COLLECTION)));

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Import collection" B_UTF8_ELLIPSIS),
		new BMessage(M_IMPORT_COLLECTION)));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Export collection" B_UTF8_ELLIPSIS),
		new BMessage(M_EXPORT_COLLECTION)));

	AddItem(menu);

	// View menu
	menu = new BMenu(B_TRANSLATE("View"));

	fTogglePreview
		= new BMenuItem(B_TRANSLATE("Show preview"), new BMessage(M_TOGGLE_PREVIEW), 'P');
	menu->AddItem(fTogglePreview);

	fToggleSidebar
		= new BMenuItem(B_TRANSLATE("Show sidebar"), new BMessage(M_TOGGLE_SIDEBAR), 'B');
	fToggleSidebar->SetMarked(true);
	menu->AddItem(fToggleSidebar);

	AddItem(menu);
}
