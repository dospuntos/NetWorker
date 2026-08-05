/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MainWindow.h"
#include "Constants.h"
#include "IconMenuItem.h"

#include <RadioButton.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <DataIO.h>
#include <Directory.h>
#include <ErrorsExt.h>
#include <File.h>
#include <FindDirectory.h>
#include <HttpFields.h>
#include <HttpRequest.h>
#include <HttpResult.h>
#include <HttpSession.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <NetServicesDefs.h>
#include <PopUpMenu.h>
#include <ScrollView.h>
#include <SplitView.h>
#include <StringView.h>
#include <SupportDefs.h>
#include <TextControl.h>
#include <Url.h>
#include <string>

using BPrivate::Network::BHttpFields;
using BPrivate::Network::BHttpMethod;
using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpSession;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainView"

static const char* kMethods[]
	= {"GET", "HEAD", "POST", "PUT", "PATCH", "DELETE", "OPTIONS", "QUERY", nullptr};

namespace {

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


BString
Base64Encode(const BString& input)
{
	static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	BString output;
	int32 len = input.Length();
	const unsigned char* data = (const unsigned char*)input.String();

	for (int32 i = 0; i < len; i += 3) {
		int32 chunkLen = std::min((int32)3, len - i);
		uint32 chunk = data[i] << 16;
		if (chunkLen > 1)
			chunk |= data[i + 1] << 8;
		if (chunkLen > 2)
			chunk |= data[i + 2];

		output << table[(chunk >> 18) & 0x3F];
		output << table[(chunk >> 12) & 0x3F];
		output << (chunkLen > 1 ? table[(chunk >> 6) & 0x3F] : '=');
		output << (chunkLen > 2 ? table[chunk & 0x3F] : '=');
	}

	return output;
}

} // namespace


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 900, 660), kApplicationName, B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fSession(BHttpSession())
{
	fMenuBar = _BuildMenu();
	_BuildLayout();

	// Load and restore settings
	BMessage settings;
	_LoadSettings(settings);
	_RestoreValues(settings);
}


MainWindow::~MainWindow()
{
	if (fCurrentResult.has_value())
		fSession.Cancel(fCurrentResult.value());

	_SaveSettings();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	using namespace BPrivate::Network::UrlEvent;
	using namespace BPrivate::Network::UrlEventData;

	switch (message->what) {

		case M_SEND_REQUEST:
			if (fCurrentResult.has_value()) {
				fSession.Cancel(fCurrentResult.value());
				fCurrentResult.reset();
				fSendButton->SetLabel("Send");
				fStatusLabel->SetText("Cancelled");
				break;
			}
			_SendRequest();
			break;

		case M_CLEAR_RESPONSE:
			_ClearResponse();
			break;

		case RequestCompleted:
		{
			int id = message->GetInt32(Id, -1);
			bool ok = message->GetBool(Success, false);

			if (!fCurrentResult.has_value() || id != fCurrentResult->Identity())
				break;

			if (!ok) {
				fStatusLabel->SetText("Request failed");
				fSendButton->SetLabel("Send");
				fCurrentResult.reset();
				break;
			}

			auto& status = fCurrentResult->Status();
			BString statusText;
			statusText << status.code << " " << status.text;
			fStatusLabel->SetText(statusText.String());

			fResponseHeadersList->Clear();

			if (fCurrentResult->HasFields()) {
				const BHttpFields& fields = fCurrentResult->Fields();

				for (const BHttpFields::Field& field : fields) {
					const BHttpFields::FieldName& name = field.Name();

					std::string_view value = field.Value();
					std::string_view nameView = name;

					BRow* row = new BRow();

					row->SetField(
						new BStringField(BString(nameView.data(), nameView.size()).String()), 0);

					row->SetField(new BStringField(BString(value.data(), value.size()).String()),
						1);

					fResponseHeadersList->AddRow(row);
				}
			}

			auto& body = fCurrentResult->Body();
			if (body.text.has_value())
				fResponseBodyView->SetText(body.text->String());
			else
				fResponseBodyView->SetText("(no body)");

			fSendButton->SetLabel("Send");
			fCurrentResult.reset();
			break;
		}

		case HttpStatus:
		{
			using namespace BPrivate::Network::UrlEventData;
			int id = message->GetInt32(Id, -1);
			if (!fCurrentResult.has_value() || id != fCurrentResult->Identity())
				break;

			int statusCode = message->GetInt16(HttpStatusCode, 0);
			BString label;
			label << statusCode;
			fStatusLabel->SetText(label.String());
			break;
		}

		case M_ADD_PARAMETER:
		{
			BString key = fParamKeyField->Text();
			BString value = fParamValueField->Text();
			if (key.Length() == 0)
				break;

			BRow* row = new BRow();
			row->SetField(new BStringField(key.String()), 0);
			row->SetField(new BStringField(value.String()), 1);
			fParamsList->AddRow(row);

			fParamKeyField->SetText("");
			fParamValueField->SetText("");

			_UpdatePreview();
			break;
		}

		case M_REMOVE_PARAMETER:
		{
			BRow* selected = fParamsList->CurrentSelection();
			if (selected != nullptr)
				fParamsList->RemoveRow(selected);

			_UpdatePreview();
			break;
		}

		case M_SELECT_PARAMETER:
		{
			BRow* selected = fParamsList->CurrentSelection();
			if (selected != nullptr) {
				auto* keyField = static_cast<BStringField*>(selected->GetField(0));
				auto* valField = static_cast<BStringField*>(selected->GetField(1));
				fParamKeyField->SetText(keyField->String());
				fParamValueField->SetText(valField->String());
			}

			_UpdatePreview();
			break;
		}

		case M_AUTH_TYPE_CHANGED:
		{
			int32 index = 0;
			if (fAuthBasicRadio->Value() == B_CONTROL_ON)
				index = 1;
			else if (fAuthBearerRadio->Value() == B_CONTROL_ON)
				index = 2;
			else if (fAuthApiKeyRadio->Value() == B_CONTROL_ON)
				index = 3;

			fAuthCardLayout->SetVisibleItem(index);
			break;
		}

		case M_UPDATE_PREVIEW:
		{
			BMenuItem* marked = fMethodMenu->FindMarked();
			BString method(marked ? marked->Label() : "GET");
			bool bodyAllowed = (method != "GET" && method != "HEAD");

			fBodyTabView->TabAt(0)->SetEnabled(bodyAllowed); // Raw
			fBodyTabView->TabAt(1)->SetEnabled(bodyAllowed); // Form

			if (!bodyAllowed && fBodyTabView->Selection() < 2)
				fBodyTabView->Select(2); // jump to Authorization tab

			_UpdatePreview();
			break;
		}

		case M_SELECT_HISTORY:
		{
			int32 index = fHistoryPanel->CurrentSelection();
			if (index >= 0) {
				HistoryItem* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(index));

				if (item != nullptr)
					_LoadHistoryItem(item);
			}
			break;
		}

		case M_DELETE_HISTORY_ITEM:
		{
			BList selectedItems;
			int32 index;
			for (int32 i = 0; (index = fHistoryPanel->CurrentSelection(i)) >= 0; i++)
				selectedItems.AddItem((void*)(addr_t)index);

			if (selectedItems.CountItems() == 0)
				break;

			for (int32 i = selectedItems.CountItems() - 1; i >= 0; i--) {
				int32 idx = (int32)(addr_t)selectedItems.ItemAt(i);
				BListItem* item = fHistoryPanel->RemoveItem(idx);
				delete item;
			}

			_UpdateHistoryButtons();
			_SaveSettings(); // Avoid stale history on disk
			break;
		}

		case M_HISTORY_SELECTION_CHANGED:
			_UpdateHistoryButtons();
			break;

		case M_CLEAR_HISTORY:
		{
			BAlert* alert = new BAlert("Clear history",
				"Remove all items from the request history? This cannot be undone.", "Clear",
				nullptr, "Cancel", B_WIDTH_FROM_WIDEST, B_OFFSET_SPACING, B_WARNING_ALERT);

			alert->SetShortcut(2, B_ESCAPE);
			if (alert->Go() == 0) {
				fHistoryPanel->MakeEmpty();
				_SaveSettings(); // Avoid stale history on disk
			}
			_UpdateHistoryButtons();

			break;
		}

		case M_NOT_IMPLEMENTED:
		{
			BAlert* alert = new BAlert("Coming Soon",
				"This feature is planned but has not been implemented yet.", "OK", nullptr, nullptr,
				B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_INFO_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go();
			break;
		}

		case M_TOGGLE_PREVIEW:
		{
			bool visible = !fRequestAreaSplit->IsItemCollapsed(1);

			fRequestAreaSplit->SetItemCollapsed(1, visible);

			if (BMenuItem* item = fMenuBar->FindItem(M_TOGGLE_PREVIEW))
				item->SetMarked(!visible);
			break;
		}

		case B_ABOUT_REQUESTED:
			be_app->AboutRequested();
			break;
		case M_REPORT_A_BUG:
		{
			std::string uri = "https://github.com/dospuntos/NetWorker/issues/";
			BUrl url = uri.c_str();
			url.OpenWithPreferredApplication();
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


bool
MainWindow::QuitRequested()
{
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


BMenuBar*
MainWindow::_BuildMenu()
{
	BMenuBar* menuBar = new BMenuBar("menubar");
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
	menuBar->AddItem(iconMenu);

	// File menu
	menu = new BMenu(B_TRANSLATE("File"));

	menu->AddItem(new BMenuItem(B_TRANSLATE("New request"), new BMessage(M_CLEAR_RESPONSE), 'N'));
	menu->AddSeparatorItem();
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Import" B_UTF8_ELLIPSIS), new BMessage(M_NOT_IMPLEMENTED)));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Export" B_UTF8_ELLIPSIS), new BMessage(M_NOT_IMPLEMENTED)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Close"), new BMessage(B_QUIT_REQUESTED)));

	menuBar->AddItem(menu);

	// View menu
	menu = new BMenu(B_TRANSLATE("View"));

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Request preview"), new BMessage(M_TOGGLE_PREVIEW), 'P'));

	menuBar->AddItem(menu);

	return menuBar;
}


BView*
MainWindow::_BuildAuthPanel()
{
	fAuthNoneRadio = new BRadioButton("authNone", "None", new BMessage(M_AUTH_TYPE_CHANGED));
	fAuthBasicRadio = new BRadioButton("authBasic", "Basic", new BMessage(M_AUTH_TYPE_CHANGED));
	fAuthBearerRadio
		= new BRadioButton("authBearer", "Bearer token", new BMessage(M_AUTH_TYPE_CHANGED));
	fAuthApiKeyRadio = new BRadioButton("authApiKey", "API key", new BMessage(M_AUTH_TYPE_CHANGED));
	fAuthNoneRadio->SetValue(B_CONTROL_ON);

	fAuthNoneRadio->SetTarget(this);
	fAuthBasicRadio->SetTarget(this);
	fAuthBearerRadio->SetTarget(this);
	fAuthApiKeyRadio->SetTarget(this);

	BView* authTypeRow = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
							 .Add(fAuthNoneRadio)
							 .Add(fAuthBasicRadio)
							 .Add(fAuthBearerRadio)
							 .Add(fAuthApiKeyRadio)
							 .AddGlue()
							 .View();

	// Card 0: None
	BView* authNoneCard = new BView("authNoneCard", B_WILL_DRAW);

	// Card 1: Basic
	fAuthUsernameField = new BTextControl("authUsername", "Username", "", nullptr);
	fAuthPasswordField = new BTextControl("authPassword", "Password", "", nullptr);
	fAuthPasswordField->TextView()->HideTyping(true);
	BView* authBasicCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							   .Add(fAuthUsernameField)
							   .Add(fAuthPasswordField)
							   .AddGlue()
							   .View();

	// Card 2: Bearer token
	fAuthTokenField = new BTextControl("authToken", "Token", "", nullptr);
	BView* authBearerCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
								.Add(fAuthTokenField)
								.AddGlue()
								.View();

	// Card 3: API key
	fAuthApiKeyNameField = new BTextControl("authApiKeyName", "Header name", "", nullptr);
	fAuthApiKeyValueField = new BTextControl("authApiKeyValue", "Value", "", nullptr);
	BView* authApiKeyCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
								.Add(fAuthApiKeyNameField)
								.Add(fAuthApiKeyValueField)
								.AddGlue()
								.View();

	BView* authCardsView = new BView("authCards", 0);
	fAuthCardLayout = new BCardLayout();
	authCardsView->SetLayout(fAuthCardLayout);
	fAuthCardLayout->AddView(authNoneCard);
	fAuthCardLayout->AddView(authBasicCard);
	fAuthCardLayout->AddView(authBearerCard);
	fAuthCardLayout->AddView(authApiKeyCard);
	fAuthCardLayout->SetVisibleItem((int32)0);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(authTypeRow)
		.Add(authCardsView)
		.View();
}


BView*
MainWindow::_BuildParamsPanel()
{
	fParamsList = new BColumnListView("paramsList", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_FANCY_BORDER);
	fParamsList->AddColumn(new BStringColumn("Key", 180, 80, 400, 0), 0);
	fParamsList->AddColumn(new BStringColumn("Value", 300, 80, 2000, 0), 1);
	fParamsList->SetInvocationMessage(
		new BMessage(M_SELECT_PARAMETER)); // double-click to load into fields for editing

	fParamKeyField = new BTextControl("paramKey", "Key", "", nullptr);
	fParamValueField = new BTextControl("paramValue", "Value", "", nullptr);
	fParamAddButton = new BButton("paramAdd", "Add", new BMessage(M_ADD_PARAMETER));
	fParamRemoveButton = new BButton("paramRemove", "Remove", new BMessage(M_REMOVE_PARAMETER));

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.Add(fParamsList)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fParamKeyField)
			.Add(fParamValueField)
			.Add(fParamAddButton)
			.Add(fParamRemoveButton)
			.End()
		.View();
}


BView*
MainWindow::_BuildPreviewPanel()
{
	fPreviewPanel = new BTextView("previewPanel");
	fPreviewPanel->MakeEditable(false);

	BStringView* previewLabel = new BStringView("previewLabel", "Request preview");
	previewLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fPreviewPanelScroll = new BScrollView("previewScroll", fPreviewPanel,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(previewLabel)
		.Add(fPreviewPanelScroll)
		.View();
}


BView*
MainWindow::_BuildRequestPanel()
{
	// Method menu
	fMethodMenu = new BPopUpMenu("GET");
	for (int i = 0; kMethods[i] != nullptr; ++i)
		fMethodMenu->AddItem(new BMenuItem(kMethods[i], new BMessage(M_UPDATE_PREVIEW)));
	fMethodMenu->ItemAt(0)->SetMarked(true);

	fMethodField = new BMenuField("method", nullptr, fMethodMenu);
	fMethodField->SetExplicitMinSize(BSize(120, B_SIZE_UNSET));
	fMethodField->SetExplicitMaxSize(BSize(120, B_SIZE_UNSET));

	// URL bar
	fUrlField = new BTextControl("url", nullptr, "", nullptr);
	fUrlField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	fUrlField->SetText("https://httpbin.org/get");

	// Send button
	fSendButton = new BButton("send", "Send", new BMessage(M_SEND_REQUEST));
	fSendButton->MakeDefault(true);

	BView* requestTopBar = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
							   .SetInsets(B_USE_WINDOW_INSETS)
							   .Add(fMethodField)
							   .Add(fUrlField)
							   .Add(fSendButton)
							   .View();

	// Request body (Raw tab)
	fRequestBodyView = new PreviewTextView("requestBody");
	fRequestBodyScroll = new BScrollView("requestBodyScroll", fRequestBodyView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	BView* paramsPanel = _BuildParamsPanel();
	BView* authPanel = _BuildAuthPanel();

	fBodyTabView = new PreviewTabView("bodyTabs");
	fBodyTabView->AddTab(fRequestBodyScroll);
	fBodyTabView->TabAt(0)->SetLabel("Raw");
	fBodyTabView->AddTab(paramsPanel);
	fBodyTabView->TabAt(1)->SetLabel("Form");
	fBodyTabView->AddTab(authPanel);
	fBodyTabView->TabAt(2)->SetLabel("Authorization");

	BView* bodyPanel = BLayoutBuilder::Group<>(B_VERTICAL)
						   .SetInsets(B_USE_WINDOW_INSETS)
						   .Add(fBodyTabView)
						   .View();

	BView* previewPanel = _BuildPreviewPanel();

	fRequestAreaSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	fRequestAreaSplit->AddChild(bodyPanel, 0.7f);
	fRequestAreaSplit->AddChild(previewPanel, 0.3f);
	fRequestAreaSplit->SetItemCollapsed(1, true);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.Add(requestTopBar)
		.Add(fRequestAreaSplit)
		.View();
}


BView*
MainWindow::_BuildResponsePanel()
{
	fStatusLabel = new BStringView("status", "(no response yet)");
	BButton* clearButton = new BButton("clear", "Clear", new BMessage(M_CLEAR_RESPONSE));

	fResponseHeadersList = new BColumnListView("responseHeaders",
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_FANCY_BORDER);
	fResponseHeadersList->AddColumn(new BStringColumn("Header", 180, 80, 400, 0), 0);
	fResponseHeadersList->AddColumn(new BStringColumn("Value", 500, 100, 2000, 0), 1);
	fResponseHeadersList->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));

	fResponseBodyView = new BTextView("responseBody");
	fResponseBodyView->MakeEditable(false);
	fResponseBodyScroll = new BScrollView("responseBodyScroll", fResponseBodyView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	fResponseBodyScroll->SetExplicitMaxSize(BSize(B_SIZE_UNSET, B_SIZE_UNLIMITED));

	fResponseTabView = new BTabView("responseTabs");
	fResponseTabView->AddTab(fResponseHeadersList);
	fResponseTabView->TabAt(0)->SetLabel("Headers");
	fResponseTabView->AddTab(fResponseBodyScroll);
	fResponseTabView->TabAt(1)->SetLabel("Body");

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
			.Add(fStatusLabel)
			.AddGlue()
			.Add(clearButton)
			.End()
		.Add(fResponseTabView)
		.View();
}


BView*
MainWindow::_BuildHistoryPanel()
{
	fHistoryPanel = new BListView("historyPanel", B_MULTIPLE_SELECTION_LIST);
	fHistoryPanel->SetInvocationMessage(new BMessage(M_SELECT_HISTORY));
	fHistoryPanel->SetSelectionMessage(new BMessage(M_HISTORY_SELECTION_CHANGED));

	BScrollView* historyScroll = new BScrollView("historyScroll", fHistoryPanel,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	BStringView* historyLabel = new BStringView("historyLabel", "History");
	historyLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fClearHistoryBtn = new BButton("clear", "Clear history", new BMessage(M_CLEAR_HISTORY));
	fClearHistoryBtn->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fRemoveItemBtn
		= new BButton("deleteItem", "Delete selected", new BMessage(M_DELETE_HISTORY_ITEM));
	fRemoveItemBtn->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fRemoveItemBtn->SetEnabled(false);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(historyLabel)
		.Add(historyScroll)
		.Add(fRemoveItemBtn)
		.Add(fClearHistoryBtn)
		.View();
}


void
MainWindow::_BuildLayout()
{
	BView* requestArea = _BuildRequestPanel();
	BView* responsePanel = _BuildResponsePanel();
	BView* historyPanel = _BuildHistoryPanel();

	fSplitView = new BSplitView(B_VERTICAL, B_USE_SMALL_SPACING);
	fSplitView->AddChild(requestArea, 0.5f);
	fSplitView->AddChild(responsePanel, 0.5f);

	BSplitView* outerSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	outerSplit->AddChild(fSplitView, 0.8f);
	outerSplit->AddChild(historyPanel, 0.2f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(fMenuBar).Add(outerSplit).End();
	_UpdateHistoryButtons();
	_UpdatePreview();
}


void
MainWindow::_SendRequest()
{
	if (fCurrentResult.has_value()) {
		fSession.Cancel(fCurrentResult.value());
		fCurrentResult.reset();
	}

	BUrl url(fUrlField->Text(), false);
	if (!url.IsValid()) {
		fStatusLabel->SetText("Invalid URL");
		return;
	}

	BMenuItem* marked = fMethodMenu->FindMarked();
	BString method(marked ? marked->Label() : "GET");

	BHttpRequest request(url);
	request.SetMethod(BHttpMethod(method.String()));

	if (fBodyTabView->Selection() == 1) { // Form
		fPendingRequestBody = "";
		for (int32 i = 0; i < fParamsList->CountRows(); ++i) {
			BRow* row = fParamsList->RowAt(i);
			auto* keyField = static_cast<BStringField*>(row->GetField(0));
			auto* valField = static_cast<BStringField*>(row->GetField(1));

			if (i > 0)
				fPendingRequestBody << "&";

			BString key = keyField->String();
			BString value = valField->String();
			fPendingRequestBody << BUrl::UrlEncode(key) << "=" << BUrl::UrlEncode(value);
		}

		if (fPendingRequestBody.Length() > 0 && method != "GET" && method != "HEAD") {
			request.SetRequestBody(std::make_unique<BMemoryIO>(fPendingRequestBody.String(),
									   fPendingRequestBody.Length()),
				"application/x-www-form-urlencoded", fPendingRequestBody.Length());
		}
	} else { // Raw text (expect JSON for now)
		fPendingRequestBody = fRequestBodyView->Text();
		if (fPendingRequestBody.Length() > 0 && method != "GET" && method != "HEAD") {
			request.SetRequestBody(std::make_unique<BMemoryIO>(fPendingRequestBody.String(),
									   fPendingRequestBody.Length()),
				"application/json", fPendingRequestBody.Length());
		}
	}

	// Add to history
	BMessage params;
	for (int32 i = 0; i < fParamsList->CountRows(); ++i) {
		BRow* row = fParamsList->RowAt(i);
		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		BMessage param;
		param.AddString("key", keyField->String());
		param.AddString("value", valField->String());
		params.AddMessage("param", &param);
	}

	BHttpFields requestFields;
	_ApplyAuth(requestFields);
	if (requestFields.CountFields() > 0)
		request.SetFields(requestFields);

	HistoryItem* item = new HistoryItem(method, url.UrlString(), fPendingRequestBody, params,
		_CurrentAuthType(), _CurrentAuthValues());
	fHistoryPanel->AddItem(item, 0); // newest on top
	_UpdateHistoryButtons();

	// Send request
	fCurrentResult = fSession.Execute(std::move(request), nullptr, BMessenger(this));

	fSendButton->SetLabel("Cancel");
	fStatusLabel->SetText("Sending" B_UTF8_ELLIPSIS);
	fResponseHeadersList->Clear();
	fResponseBodyView->SetText("");
}


void
MainWindow::_ClearResponse()
{
	fStatusLabel->SetText("(no response yet)");
	fResponseHeadersList->Clear();
	fResponseBodyView->SetText("");
	if (fCurrentResult.has_value())
		fSession.Cancel(fCurrentResult.value());
}


status_t
MainWindow::_LoadSettings(BMessage& settings)
{
	BPath path;
	status_t status;
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
MainWindow::_SaveSettings()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	status = create_directory(path.Path(), 0755);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings;
	settings.AddRect("main_window_rect", Frame());
	settings.AddString("urlField", fUrlField->Text());

	BMenuItem* marked = fMethodMenu->FindMarked();
	settings.AddString("method", marked ? marked->Label() : "GET");

	// save history
	for (int32 i = 0; i < fHistoryPanel->CountItems(); ++i) {
		auto* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(i));
		BMessage itemArchive;
		item->Archive(itemArchive);
		settings.AddMessage("historyItem", &itemArchive);
	}

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
MainWindow::_RestoreValues(BMessage& settings)
{
	BRect frame;
	if (settings.FindRect("main_window_rect", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		MoveOnScreen();
	}

	BString text;

	// Restore values
	if (settings.FindString("urlField", &text) == B_OK)
		fUrlField->SetText(text.String());

	if (settings.FindString("method", &text) == B_OK) {
		for (int32 i = 0; i < fMethodMenu->CountItems(); ++i) {
			BMenuItem* menuItem = fMethodMenu->ItemAt(i);
			if (text == menuItem->Label()) {
				menuItem->SetMarked(true);
				break;
			}
		}
	}

	// Restore history
	fHistoryPanel->MakeEmpty(); // just in case

	BMessage itemArchive;
	for (int32 i = 0; settings.FindMessage("historyItem", i, &itemArchive) == B_OK; ++i) {
		fHistoryPanel->AddItem(new HistoryItem(itemArchive));
		itemArchive.MakeEmpty();
	}
	_UpdateHistoryButtons();
}


void
MainWindow::_LoadHistoryItem(HistoryItem* item)
{
	fUrlField->SetText(item->fUrl.String());
	fRequestBodyView->SetText(item->fBody.String());

	for (int32 i = 0; i < fMethodMenu->CountItems(); i++) {
		BMenuItem* mi = fMethodMenu->ItemAt(i);
		if (item->fMethod == mi->Label()) {
			mi->SetMarked(true);
			break;
		}
	}

	fParamsList->Clear();
	BMessage param;
	for (int32 i = 0; item->fParams.FindMessage("param", i, &param) == B_OK; i++) {
		BString key, value;
		param.FindString("key", &key);
		param.FindString("value", &value);

		BRow* row = new BRow();
		row->SetField(new BStringField(key.String()), 0);
		row->SetField(new BStringField(value.String()), 1);
		fParamsList->AddRow(row);

		param.MakeEmpty();
	}

	BString username, password, token, headerName, headerValue;
	item->fAuthValues.FindString("username", &username);
	item->fAuthValues.FindString("password", &password);
	item->fAuthValues.FindString("token", &token);
	item->fAuthValues.FindString("headerName", &headerName);
	item->fAuthValues.FindString("headerValue", &headerValue);

	fAuthUsernameField->SetText(username.String());
	fAuthPasswordField->SetText(password.String());
	fAuthTokenField->SetText(token.String());
	fAuthApiKeyNameField->SetText(headerName.String());
	fAuthApiKeyValueField->SetText(headerValue.String());

	if (item->fAuthType == "basic") {
		fAuthBasicRadio->SetValue(B_CONTROL_ON);
		fAuthCardLayout->SetVisibleItem((int32)1);
	} else if (item->fAuthType == "bearer") {
		fAuthBearerRadio->SetValue(B_CONTROL_ON);
		fAuthCardLayout->SetVisibleItem((int32)2);
	} else if (item->fAuthType == "apikey") {
		fAuthApiKeyRadio->SetValue(B_CONTROL_ON);
		fAuthCardLayout->SetVisibleItem((int32)3);
	} else {
		fAuthNoneRadio->SetValue(B_CONTROL_ON);
		fAuthCardLayout->SetVisibleItem((int32)0);
	}
}


void
MainWindow::_UpdateHistoryButtons()
{
	bool hasItems = fHistoryPanel->CountItems() > 0;
	fClearHistoryBtn->SetEnabled(hasItems);

	bool hasSelection = fHistoryPanel->CurrentSelection() >= 0;
	fRemoveItemBtn->SetEnabled(hasSelection);
}


void
MainWindow::_UpdatePreview()
{
	BMenuItem* marked = fMethodMenu->FindMarked();
	BString method(marked ? marked->Label() : "GET");
	BString urlText(fUrlField->Text());

	BUrl url(urlText, false);

	BString preview;
	preview << method << " " << urlText << " HTTP/1.1\n";
	if (url.IsValid())
		preview << "Host: " << url.Host() << "\n";

	BHttpFields previewFields;
	_ApplyAuth(previewFields);
	for (const BHttpFields::Field& field : previewFields) {
		std::string_view name = field.Name();
		std::string_view value = field.Value();
		preview << BString(name.data(), name.size()) << ": " << BString(value.data(), value.size())
				<< "\n";
	}

	if (fBodyTabView->Selection() == 1) {
		// Form mode
		BString encoded;
		for (int32 i = 0; i < fParamsList->CountRows(); i++) {
			BRow* row = fParamsList->RowAt(i);
			auto* keyField = static_cast<BStringField*>(row->GetField(0));
			auto* valField = static_cast<BStringField*>(row->GetField(1));

			if (i > 0)
				encoded << "&";

			BString key = keyField->String();
			BString value = valField->String();
			encoded << BUrl::UrlEncode(key) << "=" << BUrl::UrlEncode(value);
		}

		if (encoded.Length() > 0 && method != "GET" && method != "HEAD") {
			preview << "Content-Type: application/x-www-form-urlencoded\n";
			preview << "Content-Length: " << encoded.Length() << "\n\n";
			preview << encoded;
		} else {
			preview << "\n";
		}
	} else {
		BString bodyText(fRequestBodyView->Text());
		if (bodyText.Length() > 0 && method != "GET" && method != "HEAD") {
			preview << "Content-Type: application/json\n";
			preview << "Content-Length: " << bodyText.Length() << "\n\n";
			preview << bodyText;
		} else {
			preview << "\n";
		}
	}

	fPreviewPanel->SetText(preview.String());
}


void
MainWindow::_ApplyAuth(BHttpFields& fields)
{
	if (fAuthBasicRadio->Value() == B_CONTROL_ON) {
		BString credentials;
		credentials << fAuthUsernameField->Text() << ":" << fAuthPasswordField->Text();

		BString header;
		header << "Basic " << Base64Encode(credentials);
		fields.AddField("Authorization", header.String());

	} else if (fAuthBearerRadio->Value() == B_CONTROL_ON) {
		BString token(fAuthTokenField->Text());
		if (token.Length() > 0) {
			BString header;
			header << "Bearer " << token;
			fields.AddField("Authorization", header.String());
		}

	} else if (fAuthApiKeyRadio->Value() == B_CONTROL_ON) {
		BString name(fAuthApiKeyNameField->Text());
		BString value(fAuthApiKeyValueField->Text());
		if (name.Length() > 0)
			fields.AddField(name.String(), value.String());
	}
}


BString
MainWindow::_CurrentAuthType() const
{
	if (fAuthBasicRadio->Value() == B_CONTROL_ON)
		return "basic";
	if (fAuthBearerRadio->Value() == B_CONTROL_ON)
		return "bearer";
	if (fAuthApiKeyRadio->Value() == B_CONTROL_ON)
		return "apikey";
	return "none";
}


BMessage
MainWindow::_CurrentAuthValues() const
{
	BMessage values;
	BString type = _CurrentAuthType();

	if (type == "basic") {
		values.AddString("username", fAuthUsernameField->Text());
		values.AddString("password", fAuthPasswordField->Text());
	} else if (type == "bearer") {
		values.AddString("token", fAuthTokenField->Text());
	} else if (type == "apikey") {
		values.AddString("headerName", fAuthApiKeyNameField->Text());
		values.AddString("headerValue", fAuthApiKeyValueField->Text());
	}

	return values;
}
