/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MainWindow.h"

#include <cctype>

#include "Constants.h"
#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <DataIO.h>
#include <ErrorsExt.h>
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
#include <TextView.h>
#include <Url.h>
#include <string>

using BPrivate::Network::BHttpFields;
using BPrivate::Network::BHttpMethod;
using BPrivate::Network::BHttpRequest;
using BPrivate::Network::BHttpResult;
using BPrivate::Network::BHttpSession;

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "MainView"


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 900, 660), kApplicationName, B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fSession(BHttpSession())
{
	fMenuBar = _BuildMenu();
	_BuildLayout();
	fUrlField->SetText("https://httpbin.org/get");
}


MainWindow::~MainWindow()
{
	if (fCurrentResult.has_value())
		fSession.Cancel(fCurrentResult.value());
}


void
MainWindow::MessageReceived(BMessage* message)
{
	using namespace BPrivate::Network::UrlEvent;
	using namespace BPrivate::Network::UrlEventData;

	switch (message->what) {

		case M_SEND_REQUEST:
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
				fSendButton->SetEnabled(true);
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

			fSendButton->SetEnabled(true);
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
			break;
		}

		case M_REMOVE_PARAMETER:
		{
			BRow* selected = fParamsList->CurrentSelection();
			if (selected != nullptr)
				fParamsList->RemoveRow(selected);
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
			break;
		}

		case M_SELECT_HISTORY:
		{
			BAlert* alert = new BAlert("Confirm", "Selected history item.", "OK", nullptr, nullptr,
				B_WIDTH_FROM_WIDEST, B_OFFSET_SPACING, B_INFO_ALERT);
			alert->Go();
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

	menu = new BMenu(B_TRANSLATE("File"));

	menu->AddItem(
		new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED)));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Help" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_HELP), 'H'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Report a bug" B_UTF8_ELLIPSIS), new BMessage(M_REPORT_A_BUG)));
	menu->AddSeparatorItem();
	// menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
	// new BMessage(M_SHOW_SETTINGS), ',', B_COMMAND_KEY));
	// menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	menuBar->AddItem(menu);

	return menuBar;
}


void
MainWindow::_BuildLayout()
{
	// Method menu
	fMethodMenu = new BPopUpMenu("GET");
	const char* methods[] = {"GET", "QUERY", "POST", "PUT", "PATCH", "DELETE", nullptr};
	for (int i = 0; methods[i] != nullptr; i++)
		fMethodMenu->AddItem(new BMenuItem(methods[i], nullptr));
	fMethodMenu->ItemAt(0)->SetMarked(true);

	fMethodField = new BMenuField("method", nullptr, fMethodMenu);
	fMethodField->SetExplicitMinSize(BSize(90, B_SIZE_UNSET));
	fMethodField->SetExplicitMaxSize(BSize(90, B_SIZE_UNSET));

	// URL bar
	fUrlField = new BTextControl("url", nullptr, "", nullptr);

	// Send button
	fSendButton = new BButton("send", "Send", new BMessage(M_SEND_REQUEST));
	fSendButton->MakeDefault(true);

	// Request body
	fRequestBodyView = new BTextView("requestBody");
	fRequestBodyScroll = new BScrollView("requestBodyScroll", fRequestBodyView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	// Status label
	fStatusLabel = new BStringView("status", "(no response yet)");

	// Response headers
	fResponseHeadersList = new BColumnListView("responseHeaders",
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_FANCY_BORDER);
	fResponseHeadersList->AddColumn(new BStringColumn("Header", 180, 80, 400, 0), 0);
	fResponseHeadersList->AddColumn(new BStringColumn("Value", 500, 100, 2000, 0), 1);
	fResponseHeadersList->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 140));

	// Response body
	fResponseBodyView = new BTextView("responseBody");
	fResponseBodyView->MakeEditable(false);
	fResponseBodyScroll = new BScrollView("responseBodyScroll", fResponseBodyView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	BButton* clearButton = new BButton("clear", "Clear", new BMessage(M_CLEAR_RESPONSE));

	// Params tab
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

	// Top bar: method + URL + send
	BView* requestTopBar = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
							   .SetInsets(B_USE_WINDOW_INSETS)
							   .Add(fMethodField)
							   .Add(fUrlField)
							   .Add(fSendButton)
							   .View();


	BView* paramsPanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							 .Add(fParamsList)
							 .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
								 .Add(fParamKeyField)
								 .Add(fParamValueField)
								 .Add(fParamAddButton)
								 .Add(fParamRemoveButton)
								 .End()
							 .View();

	fBodyTabView = new BTabView("bodyTabs");
	fBodyTabView->AddTab(fRequestBodyScroll);
	fBodyTabView->TabAt(0)->SetLabel("Raw");
	fBodyTabView->AddTab(paramsPanel);
	fBodyTabView->TabAt(1)->SetLabel("Form");

	// Response panel
	BView* responsePanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							   .SetInsets(B_USE_WINDOW_INSETS)
							   .AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
								   .Add(fStatusLabel)
								   .AddGlue()
								   .Add(clearButton)
								   .End()
							   .Add(fResponseHeadersList)
							   .Add(fResponseBodyScroll)
							   .View();

	// Preview panel
	fPreviewPanel = new BTextView("previewPanel");
	fPreviewPanel->MakeEditable(false);

	BStringView* previewLabel = new BStringView("previewLabel", "Request preview");
	previewLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fPreviewPanelScroll = new BScrollView("responseBodyScroll", fPreviewPanel,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);
	BView* previewPanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							  .SetInsets(B_USE_WINDOW_INSETS)
							  .Add(previewLabel)
							  .Add(fPreviewPanelScroll)
							  .View();

	// History panel
	fHistoryPanel = new BListView("historyPanel");
	fHistoryPanel->SetInvocationMessage(new BMessage(M_SELECT_HISTORY));

	BScrollView* historyScroll = new BScrollView("historyScroll", fHistoryPanel,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	BStringView* historyLabel = new BStringView("historyLabel", "History");
	historyLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	BView* historyPanel = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							  .SetInsets(B_USE_WINDOW_INSETS)
							  .Add(historyLabel)
							  .Add(historyScroll)
							  .View();
	// Body tabs and preview split beneath it
	BSplitView* requestAreaSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	requestAreaSplit->AddChild(fBodyTabView, 0.5f);
	requestAreaSplit->AddChild(previewPanel, 0.5f);

	// Outer container view
	/* BSplitView* requestAreaSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	requestAreaSplit->AddChild(requestPanel, 0.5f);
	requestAreaSplit->AddChild(previewPanel, 0.5f); */

	// Combine top bar + split into one vertical group representing the whole request area
	BView* requestArea = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							 .Add(requestTopBar)
							 .Add(requestAreaSplit)
							 .View();

	fSplitView = new BSplitView(B_VERTICAL, B_USE_SMALL_SPACING);
	fSplitView->AddChild(requestArea, 0.5f);
	fSplitView->AddChild(responsePanel, 0.5f);

	BSplitView* outerSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	outerSplit->AddChild(historyPanel, 0.2f);
	outerSplit->AddChild(fSplitView, 0.8f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(fMenuBar).Add(outerSplit).End();
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

	// BHttpRequest only takes a BUrl — set method separately via SetMethod().
	BHttpRequest request(url);
	request.SetMethod(BHttpMethod(method.String()));

	if (fBodyTabView->Selection() == 1) {
		fPendingRequestBody = "";
		for (int32 i = 0; i < fParamsList->CountRows(); i++) {
			BRow* row = fParamsList->RowAt(i);
			auto* keyField = static_cast<BStringField*>(row->GetField(0));
			auto* valField = static_cast<BStringField*>(row->GetField(1));

			if (i > 0)
				fPendingRequestBody << "&";
			fPendingRequestBody << _UrlEncode(keyField->String()) << "="
								<< _UrlEncode(valField->String());
		}

		if (fPendingRequestBody.Length() > 0 && method != "GET" && method != "HEAD") {
			request.SetRequestBody(std::make_unique<BMemoryIO>(fPendingRequestBody.String(),
									   fPendingRequestBody.Length()),
				"application/x-www-form-urlencoded", fPendingRequestBody.Length());
		}
	} else {
		fPendingRequestBody = fRequestBodyView->Text();
		if (fPendingRequestBody.Length() > 0 && method != "GET" && method != "HEAD") {
			request.SetRequestBody(std::make_unique<BMemoryIO>(fPendingRequestBody.String(),
									   fPendingRequestBody.Length()),
				"application/json", fPendingRequestBody.Length());
		}
	}

	// Add to history
	BString label;
	label << method << " " << url.UrlString();
	HistoryItem* item = new HistoryItem(label);
	item->fMethod = method;
	item->fUrl = url.UrlString();
	item->fBody = fPendingRequestBody;
	fHistoryPanel->AddItem(item, 0);

	// Send request
	fCurrentResult = fSession.Execute(std::move(request), nullptr, BMessenger(this));

	fSendButton->SetEnabled(false);
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
}


BString
MainWindow::_UrlEncode(const BString& value)
{
	BString result;
	const char* str = value.String();

	for (int32 i = 0; str[i] != '\0'; i++) {
		unsigned char c = str[i];

		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			result << (char)c;
		} else if (c == ' ') {
			result << '+';
		} else {
			char buf[4];
			snprintf(buf, sizeof(buf), "%%%02X", c);
			result << buf;
		}
	}

	return result;
}
