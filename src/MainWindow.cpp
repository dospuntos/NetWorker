/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MainWindow.h"
#include "Collection.h"
#include "Constants.h"
#include "HistoryItem.h"
#include "RenameWindow.h"

#include <RadioButton.h>

#include <Alert.h>
#include <Application.h>
#include <Button.h>
#include <CardLayout.h>
#include <Catalog.h>
#include <Clipboard.h>
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
#include <MessageRunner.h>
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


class HistoryListView : public BListView {
public:
    HistoryListView(const char* name)
        : BListView(name, B_MULTIPLE_SELECTION_LIST) {}

    void MouseDown(BPoint where) override
    {
        BMessage* msg = Window()->CurrentMessage();
        int32 buttons = msg->GetInt32("buttons", 0);

        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
            int32 index = IndexOf(where);
            if (index >= 0) {
                if (!IsItemSelected(index)) {
                    DeselectAll();
                    Select(index);
                }

                BPopUpMenu* menu = new BPopUpMenu("historyContext", false, false);
                menu->AddItem(new BMenuItem("Load", new BMessage(M_SELECT_HISTORY)));
				menu->AddItem(new BMenuItem("Rename" B_UTF8_ELLIPSIS, new BMessage(M_SHOW_RENAME_DIALOG)));
				menu->AddItem(new BMenuItem("Copy URL", new BMessage(M_COPY_HISTORY_URL)));
				menu->AddSeparatorItem();
				menu->AddItem(new BMenuItem("Save to collection" B_UTF8_ELLIPSIS, new BMessage(M_SAVE_TO_COLLECTION)));
				menu->AddSeparatorItem();
                menu->AddItem(new BMenuItem("Delete", new BMessage(M_DELETE_HISTORY_ITEM)));
                menu->SetTargetForItems(Window());

                ConvertToScreen(&where);
                menu->Go(where, true, true, true);
            }
            return;
        }

        BListView::MouseDown(where);
    }
};


class CollectionListView : public BListView {
public:
    CollectionListView(const char* name)
        : BListView(name, B_SINGLE_SELECTION_LIST) {}

    void MouseDown(BPoint where) override
    {
        BMessage* msg = Window()->CurrentMessage();
        int32 buttons = msg->GetInt32("buttons", 0);

        if (buttons & B_SECONDARY_MOUSE_BUTTON) {
            int32 index = IndexOf(where);
            if (index >= 0) {
                Select(index);

                BPopUpMenu* menu = new BPopUpMenu("collectionItemContext", false, false);
                menu->AddItem(new BMenuItem("Load", new BMessage(M_LOAD_COLLECTION_ITEM)));
                menu->AddItem(new BMenuItem("Rename" B_UTF8_ELLIPSIS,
                    new BMessage(M_SHOW_RENAME_COLLECTION_ITEM)));
				menu->AddItem(new BMenuItem("Copy URL", new BMessage(M_COPY_COLLECTION_URL)));
                menu->AddSeparatorItem();
                menu->AddItem(new BMenuItem("Delete", new BMessage(M_DELETE_COLLECTION_ITEM)));
                menu->SetTargetForItems(Window());

                ConvertToScreen(&where);
                menu->Go(where, true, true, true);
            }
            return;
        }

        BListView::MouseDown(where);
    }
};

} // namespace


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 900, 660), kApplicationName, B_TITLED_WINDOW,
		B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE),
	fSession(BHttpSession()),
	fActiveCollectionIndex(-1)
{
	fAppSettings = new AppSettings();
	fAppSettings->Load();

	fMenuBar = new MenuBar();
	_BuildLayout();

	// Load and restore settings
	BMessage settings;
	if (_LoadSettings(settings) == B_OK)
		_RestoreValues(settings);

	_LoadCollectionsIndex();
	_RefreshCollectionMenu();
	_RefreshCollectionItemList();
	_ApplyDefaultUserAgent();
}


MainWindow::~MainWindow()
{
	if (fCurrentResult.has_value())
		fSession.Cancel(fCurrentResult.value());

	_SaveSettings();

	if (fSettingsWindow != nullptr)
		fSettingsWindow->PostMessage(B_QUIT_REQUESTED);

	delete fAuthPanel;
	delete fBodyPanel;
	delete fQueryParamsEditor;
	delete fHeadersPanel;
	delete fAppSettings;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	using namespace BPrivate::Network::UrlEvent;
	using namespace BPrivate::Network::UrlEventData;

	switch (message->what) {

		case M_NEW_REQUEST:
		{
			_ClearResponse();
			RequestData data;
			_LoadRequestData(data);
			_UpdatePreview();
			_ApplyDefaultUserAgent();
			break;
		}
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

		case M_REQUEST_TIMEOUT:
		{
			if (!fCurrentResult.has_value())
				break;

			_CancelCurrentRequest();

			fStatusLabel->SetText("Request timed out");
			fSendButton->SetLabel("Send");
			break;
		}

		case M_CLEAR_RESPONSE:
			_ClearResponse();
			break;

		case RequestCompleted:
		{
			_StopRequestTimeout();

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
			if (body.text.has_value()) {
				BString bodyText(body.text->String());
				int32 length = bodyText.Length();

				if (fAppSettings->fMaxResponseSize > 0 && length > fAppSettings->fMaxResponseSize) {
					bodyText.Truncate(fAppSettings->fMaxResponseSize);
					bodyText << "\n\n[Response length truncated, adjust the limit in Settings.]";
				}
				fResponseBodyView->SetText(bodyText);
			} else {
				fResponseBodyView->SetText("(no body)");
			}

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

		case M_HEADER_ADD:
			fHeadersPanel->AddCurrentFields();
			_UpdatePreview();
			break;

		case M_HEADER_REMOVE:
			fHeadersPanel->RemoveSelected();
			_UpdatePreview();
			break;

		case M_HEADER_SELECT:
			fHeadersPanel->LoadSelectedIntoFields();
			break;

		case M_FORM_PARAM_ADD:
			fBodyPanel->FormEditor()->AddCurrentFields();
			_UpdatePreview();
			break;

		case M_FORM_PARAM_REMOVE:
			fBodyPanel->FormEditor()->RemoveSelected();
			_UpdatePreview();
			break;

		case M_FORM_PARAM_SELECT:
			fBodyPanel->FormEditor()->LoadSelectedIntoFields();
			break;

		case M_QUERY_PARAM_ADD:
			fQueryParamsEditor->AddCurrentFields();
			_UpdatePreview();
			break;

		case M_QUERY_PARAM_REMOVE:
			fQueryParamsEditor->RemoveSelected();
			_UpdatePreview();
			break;

		case M_QUERY_PARAM_SELECT:
			fQueryParamsEditor->LoadSelectedIntoFields();
			break;

		case M_AUTH_TYPE_CHANGED:
		{
			fAuthPanel->UpdateVisibleCard();
			_UpdatePreview();
			break;
		}

		case M_UPDATE_PREVIEW:
		{
			BMenuItem* marked = fMethodMenu->FindMarked();
			BString method(marked ? marked->Label() : "GET");
			bool bodyAllowed = (method != "GET" && method != "HEAD");

			fBodyTabView->TabAt(2)->SetEnabled(bodyAllowed); // Body tab

			if (!bodyAllowed && fBodyTabView->Selection() == 2)
				fBodyTabView->Select(0); // jump to Params tab

			_UpdatePreview();
			break;
		}

		case M_SELECT_HISTORY:
		{
			int32 index = fHistoryPanel->CurrentSelection();
			if (index >= 0) {
				HistoryItem* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(index));

				if (item != nullptr)
					_LoadRequestData(item->fData);
			}
			break;
		}

		case M_DELETE_HISTORY_ITEM:
		{
			BList selectedItems;
			int32 index;
			for (int32 i = 0; (index = fHistoryPanel->CurrentSelection(i)) >= 0; ++i)
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

		case M_COPY_HISTORY_URL:
		{
			int32 index = fHistoryPanel->CurrentSelection();
			if (index < 0)
				break;

			auto* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(index));

			if (be_clipboard->Lock()) {
				be_clipboard->Clear();

				BMessage* clip = be_clipboard->Data();
				if (clip != nullptr) {
					clip->AddData("text/plain", B_MIME_TYPE, item->fData.fUrl.String(),
						item->fData.fUrl.Length());
					be_clipboard->Commit();
				}

				be_clipboard->Unlock();
			}
			break;
		}

		case M_COPY_COLLECTION_URL:
		{
			int32 index = fCollectionListView->CurrentSelection();
			if (index < 0 || fActiveCollectionIndex < 0)
				break;

			Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
			if (collection == nullptr)
				break;

			CollectionItem* item = collection->ItemAt(index);
			if (item == nullptr)
				break;

			if (be_clipboard->Lock()) {
				be_clipboard->Clear();

				BMessage* clip = be_clipboard->Data();
				if (clip != nullptr) {
					clip->AddData("text/plain", B_MIME_TYPE, item->fData.fUrl.String(),
						item->fData.fUrl.Length());
					be_clipboard->Commit();
				}

				be_clipboard->Unlock();
			}
			break;
		}

		case M_SHOW_RENAME_DIALOG:
		{
			int32 index = fHistoryPanel->CurrentSelection();
			if (index < 0)
				break;

			auto* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(index));

			BRect frame(0, 0, 280, 60);
			frame.OffsetTo(Frame().left + 60, Frame().top + 60);

			RenameWindow* win = new RenameWindow(frame, item->Text(), index, BMessenger(this),
				M_RENAME_HISTORY_ITEM);
			win->Show();
			break;
		}

		case M_RENAME_HISTORY_ITEM:
		{
			int32 index;
			BString label;
			if (message->FindInt32("index", &index) != B_OK
				|| message->FindString("label", &label) != B_OK) {
				break;
			}

			if (index >= 0 && index < fHistoryPanel->CountItems()) {
				auto* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(index));
				item->SetCustomLabel(label);
				fHistoryPanel->InvalidateItem(index);
				_SaveSettings();
			}
			break;
		}

		case M_CLEAR_HISTORY:
		{
			BAlert* alert = new BAlert("Clear history",
				"Remove all items from the request history? This cannot be undone.", "Cancel",
				"Clear", nullptr, B_WIDTH_AS_USUAL, B_WARNING_ALERT);

			alert->SetShortcut(0, B_ESCAPE);
			if (alert->Go() == 1) {
				fHistoryPanel->MakeEmpty();
				_SaveSettings(); // Avoid stale history on disk
			}
			_UpdateHistoryButtons();

			break;
		}

		case M_SELECT_COLLECTION:
		{
			int32 index;
			if (message->FindInt32("index", &index) == B_OK) {
				fActiveCollectionIndex = index;
				_RefreshCollectionMenu();
				_RefreshCollectionItemList();
			}
			break;
		}

		case M_NEW_COLLECTION:
		{
			BRect frame(0, 0, 280, 60);
			frame.OffsetTo(Frame().left + 60, Frame().top + 60);
			RenameWindow* win
				= new RenameWindow(frame, "", -1, BMessenger(this), M_CREATE_COLLECTION);
			win->SetTitle("New collection");
			win->Show();
			break;
		}

		case M_CREATE_COLLECTION:
		{
			BString label;
			if (message->FindString("label", &label) != B_OK || label.Length() == 0)
				break;

			Collection* collection = new Collection(label);
			fCollections.AddItem(collection);
			fActiveCollectionIndex = fCollections.CountItems() - 1;

			_SaveCollection(collection);
			_SaveCollectionsIndex();
			_RefreshCollectionMenu();
			_RefreshCollectionItemList();
			break;
		}

		case M_DELETE_COLLECTION:
		{
			if (fActiveCollectionIndex < 0)
				break;

			BString collectionName = fCollections.ItemAt(fActiveCollectionIndex)->Name().String();
			BString alertMsg;
			alertMsg.SetToFormat("Delete the collection '%s' permanently?\nThis cannot be undone.",
				collectionName.String());

			BAlert* alert = new BAlert("Delete collection", alertMsg, "Cancel", "Delete", nullptr,
				B_WIDTH_AS_USUAL, B_WARNING_ALERT);
			alert->SetShortcut(0, B_ESCAPE);
			alert->Go(new BInvoker(new BMessage(M_CONFIRM_DELETE_COLLECTION), this));
			break;
		}

		case M_CONFIRM_DELETE_COLLECTION:
		{
			int32 which = message->GetInt32("which", -1);
			if (which == 1 && fActiveCollectionIndex >= 0) {
				Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
				if (collection == nullptr)
					break;

				BPath dirPath;
				_CollectionsDirectory(dirPath);
				BPath filePath(dirPath);
				filePath.Append(collection->FileName());
				BEntry(filePath.Path()).Remove();

				fCollections.RemoveItemAt(fActiveCollectionIndex);
				delete collection;

				fActiveCollectionIndex = fCollections.CountItems() > 0 ? 0 : -1;
				_SaveCollectionsIndex();
				_RefreshCollectionMenu();
				_RefreshCollectionItemList();
			}
			break;
		}

		case M_SAVE_TO_COLLECTION:
		{
			if (fActiveCollectionIndex < 0) {
				(new BAlert("save", "No active collections - create one first", "OK"))->Go();
				break;
			}

			Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
			if (collection == nullptr)
				break;

			BMenuItem* marked = fMethodMenu->FindMarked();
			BString method(marked ? marked->Label() : "GET");
			BString urlText(fUrlField->Text());
			BString bodyText(fBodyPanel->CurrentBody());

			BMessage params = fBodyPanel->FormEditor()->CurrentValues();
			BMessage queryParams = fQueryParamsEditor->CurrentValues();
			BMessage customHeaders = fHeadersPanel->CurrentHeaders();

			RequestData data(method, urlText, customHeaders, bodyText, params, queryParams,
				fBodyPanel->CurrentMode(), fBodyPanel->CurrentFilePath(), fAuthPanel->CurrentType(),
				fAuthPanel->CurrentValues());

			collection->AddItem(new CollectionItem(data));
			_SaveCollection(collection);
			_RefreshCollectionItemList();
			fStatusLabel->SetText("Saved to collection");
			break;
		}

		case M_LOAD_COLLECTION_ITEM:
		{
			int32 index = fCollectionListView->CurrentSelection();
			if (index < 0 || fActiveCollectionIndex < 0)
				break;

			Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
			if (collection == nullptr)
				break;
			CollectionItem* item = collection->ItemAt(index);
			if (item != nullptr)
				_LoadRequestData(item->fData);
			break;
		}

		case M_SHOW_RENAME_COLLECTION_ITEM:
		{
			int32 index = fCollectionListView->CurrentSelection();
			if (index < 0 || fActiveCollectionIndex < 0)
				break;

			CollectionItem* item = fCollections.ItemAt(fActiveCollectionIndex)->ItemAt(index);
			if (item == nullptr)
				break;

			BRect frame(0, 0, 280, 60);
			frame.OffsetTo(Frame().left + 60, Frame().top + 60);
			RenameWindow* win = new RenameWindow(frame, item->Text(), index, BMessenger(this),
				M_RENAME_COLLECTION_ITEM);
			win->Show();
			break;
		}

		case M_RENAME_COLLECTION_ITEM:
		{
			int32 index;
			BString label;
			if (message->FindInt32("index", &index) != B_OK
				|| message->FindString("label", &label) != B_OK) {
				break;
			}
			if (fActiveCollectionIndex < 0)
				break;

			Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
			if (collection == nullptr)
				break;
			CollectionItem* item = collection->ItemAt(index);
			if (item != nullptr) {
				item->SetCustomLabel(label);
				_SaveCollection(collection);
				_RefreshCollectionItemList();
			}
			break;
		}

		case M_DELETE_COLLECTION_ITEM:
		{
			int32 index = fCollectionListView->CurrentSelection();
			if (index < 0 || fActiveCollectionIndex < 0)
				break;

			Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
			if (collection == nullptr)
				break;
			CollectionItem* removed = collection->RemoveItem(index);
			delete removed;
			_SaveCollection(collection);
			_RefreshCollectionItemList();
			break;
		}

		case M_BODY_MODE_CHANGED:
			fBodyPanel->UpdateVisibleCard();
			_UpdatePreview();
			break;

		case M_SHOW_BODY_FILE_PANEL:
			fBodyPanel->ShowFilePanel(this, M_BODY_FILE_SELECTED);
			break;

		case M_BODY_FILE_SELECTED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) == B_OK) {
				BPath path(&ref);
				fBodyPanel->SetFilePath(path.Path());
				_UpdatePreview();
			}
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

		case M_TOGGLE_SIDEBAR:
		{
			bool visible = !fOuterSplit->IsItemCollapsed(1);

			fOuterSplit->SetItemCollapsed(1, visible);

			if (BMenuItem* item = fMenuBar->FindItem(M_TOGGLE_SIDEBAR))
				item->SetMarked(!visible);
			break;
		}

		case M_SHOW_SETTINGS:
		{
			if (fSettingsWindow == nullptr) {
				fSettingsWindow = new SettingsWindow(fAppSettings, BMessenger(this));
				fSettingsWindow->CenterIn(Frame());
			}
			fSettingsWindow->Show();
			if (fSettingsWindow->IsHidden() == false)
				fSettingsWindow->Activate();
			break;
		}

		case M_SETTINGS_APPLIED:
			_ApplySettings();
			break;

		case M_SETTINGS_WINDOW_CLOSED:
			fSettingsWindow = nullptr;
			break;

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


BView*
MainWindow::_BuildPreviewPanel()
{
	fPreviewPanel = new BTextView("previewPanel");
	fPreviewPanel->MakeEditable(false);

	BStringView* previewLabel = new BStringView("previewLabel", "Request preview");
	previewLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fPreviewPanelScroll
		= new BScrollView("previewScroll", fPreviewPanel, B_WILL_DRAW | B_FRAME_EVENTS, true, true);

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

	// Save button
	fSaveButton = new BButton("saveToCollection", "Save", new BMessage(M_SAVE_TO_COLLECTION));

	BView* requestTopBar = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
							   .SetInsets(B_USE_WINDOW_INSETS)
							   .Add(fMethodField)
							   .Add(fUrlField)
							   .Add(fSendButton)
							   .Add(fSaveButton)
							   .View();

	// Request tabs
	fHeadersPanel = new HeadersPanel();
	fHeadersPanel->SetTarget(this, M_HEADER_ADD, M_HEADER_REMOVE, M_HEADER_SELECT);

	fQueryParamsEditor = new KeyValueEditor("Key", "Value");
	fQueryParamsEditor->SetTarget(this, M_QUERY_PARAM_ADD, M_QUERY_PARAM_REMOVE,
		M_QUERY_PARAM_SELECT);

	fBodyPanel = new BodyPanel();
	fBodyPanel->SetTarget(this, M_BODY_MODE_CHANGED);
	fBodyPanel->FormEditor()->SetTarget(this, M_FORM_PARAM_ADD, M_FORM_PARAM_REMOVE,
		M_FORM_PARAM_SELECT);

	fAuthPanel = new AuthPanel();
	fAuthPanel->SetTarget(this, M_AUTH_TYPE_CHANGED);

	fBodyTabView = new PreviewTabView("bodyTabs");
	fBodyTabView->AddTab(fHeadersPanel->View());
	fBodyTabView->TabAt(0)->SetLabel("Headers");
	fBodyTabView->AddTab(fQueryParamsEditor->View());
	fBodyTabView->TabAt(1)->SetLabel("Params");
	fBodyTabView->AddTab(fBodyPanel->View());
	fBodyTabView->TabAt(2)->SetLabel("Body");
	fBodyTabView->AddTab(fAuthPanel->View());
	fBodyTabView->TabAt(3)->SetLabel("Authorization");

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
	fHistoryPanel = new HistoryListView("historyPanel");
	fHistoryPanel->SetInvocationMessage(new BMessage(M_SELECT_HISTORY));
	fHistoryPanel->SetSelectionMessage(new BMessage(M_HISTORY_SELECTION_CHANGED));

	BScrollView* historyScroll = new BScrollView("historyScroll", fHistoryPanel,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	BStringView* historyLabel = new BStringView("historyLabel", "History");
	historyLabel->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	fClearHistoryBtn = new BButton("clear", "Clear history", new BMessage(M_CLEAR_HISTORY));
	fClearHistoryBtn->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(historyLabel)
		.Add(historyScroll)
		.Add(fClearHistoryBtn)
		.View();
}


BView*
MainWindow::_BuildCollectionPanel()
{
	// Collection dropdown
	fCollectionMenu = new BPopUpMenu("(no collections)");
	fCollectionMenuField = new BMenuField("collectionSelect", nullptr, fCollectionMenu);

	fNewCollectionButton = new BButton("newCollection", "New", new BMessage(M_NEW_COLLECTION));
	fDeleteCollectionButton
		= new BButton("deleteCollection", "Delete", new BMessage(M_DELETE_COLLECTION));
	fDeleteCollectionButton->SetEnabled(false);

	BView* collectionHeader = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
								  .Add(fCollectionMenuField)
								  .AddGlue()
								  .Add(fNewCollectionButton)
								  .Add(fDeleteCollectionButton)
								  .View();

	// Collection items list
	fCollectionListView = new CollectionListView("collectionItems");
	fCollectionListView->SetInvocationMessage(new BMessage(M_LOAD_COLLECTION_ITEM));
	BScrollView* collectionScroll = new BScrollView("collectionItemsScroll", fCollectionListView,
		B_WILL_DRAW | B_FRAME_EVENTS, false, true);

	return BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(collectionHeader)
		.Add(collectionScroll)
		.View();
}


void
MainWindow::_BuildLayout()
{
	BView* requestArea = _BuildRequestPanel();
	BView* responsePanel = _BuildResponsePanel();
	BView* historyPanel = _BuildHistoryPanel();
	BView* collectionsPanel = _BuildCollectionPanel();

	fSplitView = new BSplitView(B_VERTICAL, B_USE_SMALL_SPACING);
	fSplitView->AddChild(requestArea, 0.5f);
	fSplitView->AddChild(responsePanel, 0.5f);

	// Sidebar: History / Collections tabs
	fSidebarTabs = new BTabView("sidebarTabs");
	fSidebarTabs->AddTab(historyPanel);
	fSidebarTabs->TabAt(0)->SetLabel("History");
	fSidebarTabs->AddTab(collectionsPanel);
	fSidebarTabs->TabAt(1)->SetLabel("Collections");

	fOuterSplit = new BSplitView(B_HORIZONTAL, B_USE_SMALL_SPACING);
	fOuterSplit->AddChild(fSplitView, 0.8f);
	fOuterSplit->AddChild(fSidebarTabs, 0.2f);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0).Add(fMenuBar).Add(fOuterSplit).End();
	_UpdateHistoryButtons();
	_UpdatePreview();
	_RefreshCollectionMenu();
}


void
MainWindow::_SendRequest()
{
	if (fCurrentResult.has_value()) {
		fSession.Cancel(fCurrentResult.value());
		fCurrentResult.reset();
	}

	BUrl url(_BuildFullUrl(), false);
	if (!url.IsValid()) {
		fStatusLabel->SetText("Invalid URL");
		return;
	}

	BMenuItem* marked = fMethodMenu->FindMarked();
	BString method(marked ? marked->Label() : "GET");

	BHttpRequest request(url);
	request.SetMethod(BHttpMethod(method.String()));

	BString mode = fBodyPanel->CurrentMode();
	bool bodyAllowed = (method != "GET" && method != "HEAD" && mode != "none");

	if (bodyAllowed) {
		BString body = fBodyPanel->CurrentBody();
		if (body.Length() > 0) {
			request.SetRequestBody(std::make_unique<BMemoryIO>(body.String(), body.Length()),
				fBodyPanel->CurrentContentType().String(), body.Length());
		}
	}

	// Add to history
	BMessage params = fBodyPanel->FormEditor()->CurrentValues();
	BMessage queryParams = fQueryParamsEditor->CurrentValues();

	BHttpFields requestFields;
	fAuthPanel->ApplyTo(requestFields);

	BMessage customHeaders = fHeadersPanel->CurrentHeaders();
	BMessage h;
	for (int32 i = 0; customHeaders.FindMessage("header", i, &h) == B_OK; i++) {
		BString key, value;
		h.FindString("key", &key);
		h.FindString("value", &value);
		if (key.Length() > 0)
			requestFields.AddField(key.String(), value.String());
		h.MakeEmpty();
	}

	if (requestFields.CountFields() > 0)
		request.SetFields(requestFields);

	RequestData data(method, url.UrlString(), fHeadersPanel->CurrentHeaders(), fPendingRequestBody,
		params, queryParams, fBodyPanel->CurrentMode(), fBodyPanel->CurrentFilePath(),
		fAuthPanel->CurrentType(), fAuthPanel->CurrentValues());
	HistoryItem* newItem = new HistoryItem(data);

	for (int32 i = 0; i < fHistoryPanel->CountItems(); ++i) {
		auto* existing = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(i));
		if (existing->Equals(*newItem)) {
			newItem->SetCustomLabel(existing->fCustomLabel);
			BListItem* removed = fHistoryPanel->RemoveItem(i);
			delete removed;
			break;
		}
	}
	fHistoryPanel->AddItem(newItem, 0); // newest on top
	_TrimHistory();
	_UpdateHistoryButtons();

	// Send request
	fCurrentResult = fSession.Execute(std::move(request), nullptr, BMessenger(this));

	fSendButton->SetLabel("Cancel");
	fStatusLabel->SetText("Sending" B_UTF8_ELLIPSIS);
	fResponseHeadersList->Clear();
	fResponseBodyView->SetText("");

	// Manual timeout
	if (fAppSettings->fTimeoutSeconds > 0) {
		bigtime_t interval = static_cast<bigtime_t>(fAppSettings->fTimeoutSeconds) * 1000000;

		fRequestTimeoutRunner
			= new BMessageRunner(BMessenger(this), new BMessage(M_REQUEST_TIMEOUT), interval, 1);
	}
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

	settings.AddBool("showPreview", fMenuBar->TogglePreview()->IsMarked());
	settings.AddBool("showSidebar", fMenuBar->ToggleSidebar()->IsMarked());

	if (fAppSettings->fSaveFieldsOnExit) {
		// save current values
		BMenuItem* marked = fMethodMenu->FindMarked();
		BString method = marked ? marked->Label() : "GET";

		RequestData current(method, fUrlField->Text(), fHeadersPanel->CurrentHeaders(),
			fBodyPanel->CurrentBody(), fBodyPanel->FormEditor()->CurrentValues(),
			fQueryParamsEditor->CurrentValues(), fBodyPanel->CurrentMode(),
			fBodyPanel->CurrentFilePath(), fAuthPanel->CurrentType(), fAuthPanel->CurrentValues());

		BMessage currentArchive;
		current.Archive(currentArchive);
		settings.AddMessage("currentRequest", &currentArchive);

		// save history
		for (int32 i = 0; i < fHistoryPanel->CountItems(); ++i) {
			auto* item = static_cast<HistoryItem*>(fHistoryPanel->ItemAt(i));
			BMessage itemArchive;
			item->Archive(itemArchive);
			settings.AddMessage("historyItem", &itemArchive);
		}
	} else {
		// Remove existing data if any, and erase history
		fHistoryPanel->MakeEmpty();
		PostMessage(M_NEW_REQUEST);
		_SaveSettings();
	}

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
MainWindow::_RestoreValues(BMessage& settings)
{
	// Restore window size/position
	BRect frame;
	if (settings.FindRect("main_window_rect", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
		MoveOnScreen();
	}

	bool showPreview;
	if (settings.FindBool("showPreview", &showPreview) == B_OK) {
		fRequestAreaSplit->SetItemCollapsed(1, !showPreview);
		fMenuBar->TogglePreview()->SetMarked(showPreview);
	}

	bool showSidebar;
	if (settings.FindBool("showSidebar", &showSidebar) == B_OK) {
		fOuterSplit->SetItemCollapsed(1, !showSidebar);
		fMenuBar->ToggleSidebar()->SetMarked(showSidebar);
	}

	if (fAppSettings->fSaveFieldsOnExit) {
		// Restore fields
		BMessage currentArchive;
		if (settings.FindMessage("currentRequest", &currentArchive) == B_OK) {
			RequestData current(currentArchive);
			_LoadRequestData(current);
		}

		// Restore history
		fHistoryPanel->MakeEmpty(); // just in case

		BMessage itemArchive;
		for (int32 i = 0; settings.FindMessage("historyItem", i, &itemArchive) == B_OK; ++i) {
			fHistoryPanel->AddItem(new HistoryItem(itemArchive));
			itemArchive.MakeEmpty();
		}
	}
	_UpdateHistoryButtons();
}


void
MainWindow::_LoadRequestData(const RequestData& data)
{
	fUrlField->SetText(data.fUrl.String());
	fBodyPanel->LoadFrom(data.fBodyMode, data.fBody, data.fParams, data.fFilePath);

	for (int32 i = 0; i < fMethodMenu->CountItems(); ++i) {
		BMenuItem* mi = fMethodMenu->ItemAt(i);
		if (data.fMethod == mi->Label()) {
			mi->SetMarked(true);
			break;
		}
	}

	BString username, password, token, headerName, headerValue;
	data.fAuthValues.FindString("username", &username);
	data.fAuthValues.FindString("password", &password);
	data.fAuthValues.FindString("token", &token);
	data.fAuthValues.FindString("headerName", &headerName);
	data.fAuthValues.FindString("headerValue", &headerValue);

	fAuthPanel->LoadFrom(data.fAuthType, data.fAuthValues);
	fQueryParamsEditor->LoadFrom(data.fQueryParams);
	fHeadersPanel->LoadFrom(data.fCustomHeaders);
}


void
MainWindow::_UpdateHistoryButtons()
{
	bool hasItems = fHistoryPanel->CountItems() > 0;
	fClearHistoryBtn->SetEnabled(hasItems);
}


void
MainWindow::_UpdatePreview()
{
	BMenuItem* marked = fMethodMenu->FindMarked();
	BString method(marked ? marked->Label() : "GET");
	BString urlText = _BuildFullUrl();

	BUrl url(urlText, false);

	BString preview;
	preview << method << " " << urlText << " HTTP/1.1\n";
	if (url.IsValid())
		preview << "Host: " << url.Host() << "\n";

	fHeadersPanel->SetComputedHeaders(_ComputedHeaders());
	BHttpFields previewFields;
	fAuthPanel->ApplyTo(previewFields);
	for (const BHttpFields::Field& field : previewFields) {
		std::string_view name = field.Name();
		std::string_view value = field.Value();
		preview << BString(name.data(), name.size()) << ": " << BString(value.data(), value.size())
				<< "\n";
	}

	// Custom headers
	BMessage customHeaders = fHeadersPanel->CurrentHeaders();
	BMessage h;
	for (int32 i = 0; customHeaders.FindMessage("header", i, &h) == B_OK; i++) {
		BString key, value;
		h.FindString("key", &key);
		h.FindString("value", &value);
		preview << key << ": " << value << "\n";
		h.MakeEmpty();
	}

	BString mode = fBodyPanel->CurrentMode();
	bool bodyAllowed = (method != "GET" && method != "HEAD" && mode != "none");

	if (bodyAllowed) {
		if (mode == "file") {
			BString path = fBodyPanel->CurrentFilePath();
			if (path.Length() > 0) {
				preview << "Content-Type: " << fBodyPanel->CurrentContentType() << "\n";
				preview << "Content-Length: " << fBodyPanel->CurrentBodyLength() << "\n\n";
				preview << "[File: " << path << ", "
						<< _FormatFileSize(fBodyPanel->CurrentBodyLength()) << "]";
			}
		} else {
			BString body = fBodyPanel->CurrentBody();
			if (body.Length() > 0) {
				preview << "Content-Type: " << fBodyPanel->CurrentContentType() << "\n";
				preview << "Content-Length: " << body.Length() << "\n\n";
				preview << body;
			}
		}
	}

	fPreviewPanel->SetText(preview.String());
}


status_t
MainWindow::_CollectionsDirectory(BPath& path)
{
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	status = path.Append(kCollectionsDirName);
	if (status != B_OK)
		return status;

	return create_directory(path.Path(), 0755);
}


status_t
MainWindow::_SaveCollection(Collection* collection)
{
	BPath dirPath;
	status_t status = _CollectionsDirectory(dirPath);
	if (status != B_OK)
		return status;

	BPath filePath(dirPath);
	status = filePath.Append(collection->FileName());
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(filePath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage archive;
	collection->Archive(archive);
	return archive.Flatten(&file);
}


status_t
MainWindow::_LoadCollection(const BString& fileName, Collection*& outCollection)
{
	BPath dirPath;
	status_t status = _CollectionsDirectory(dirPath);
	if (status != B_OK)
		return status;

	BPath filePath(dirPath);
	status = filePath.Append(fileName);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(filePath.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	BMessage archive;
	status = archive.Unflatten(&file);
	if (status != B_OK)
		return status;

	outCollection = new Collection(archive);
	outCollection->SetFileName(fileName);
	return B_OK;
}


status_t
MainWindow::_SaveCollectionsIndex()
{
	BPath dirPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &dirPath);
	if (status != B_OK)
		return status;
	status = dirPath.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	BPath indexPath(dirPath);
	status = indexPath.Append(kCollectionsIndexFileName);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(indexPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage index;
	for (int32 i = 0; i < fCollections.CountItems(); ++i)
		index.AddString("fileName", fCollections.ItemAt(i)->FileName());

	if (fActiveCollectionIndex >= 0 && fActiveCollectionIndex < fCollections.CountItems())
		index.AddString("activeFileName", fCollections.ItemAt(fActiveCollectionIndex)->FileName());

	return index.Flatten(&file);
}


status_t
MainWindow::_LoadCollectionsIndex()
{
	BPath dirPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &dirPath);
	if (status != B_OK)
		return status;
	status = dirPath.Append(kSettingsDirName);
	if (status != B_OK)
		return status;

	BPath indexPath(dirPath);
	status = indexPath.Append(kCollectionsIndexFileName);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(indexPath.Path(), B_READ_ONLY);
	if (status != B_OK)
		return B_OK; // no index yet (first run)

	BMessage index;
	status = index.Unflatten(&file);
	if (status != B_OK)
		return status;

	BString fileName;
	for (int32 i = 0; index.FindString("fileName", i, &fileName) == B_OK; ++i) {
		Collection* collection = nullptr;
		if (_LoadCollection(fileName, collection) == B_OK)
			fCollections.AddItem(collection);
	}

	fActiveCollectionIndex = -1; // default: none, if nothing matches

	BString activeFileName;
	if (index.FindString("activeFileName", &activeFileName) == B_OK) {
		for (int32 i = 0; i < fCollections.CountItems(); ++i) {
			if (fCollections.ItemAt(i)->FileName() == activeFileName) {
				fActiveCollectionIndex = i;
				break;
			}
		}
	}

	_RefreshCollectionMenu();

	return B_OK;
}


void
MainWindow::_RefreshCollectionMenu()
{
	for (int32 i = fCollectionMenu->CountItems() - 1; i >= 0; --i)
		delete fCollectionMenu->RemoveItem(i);

	for (int32 i = 0; i < fCollections.CountItems(); ++i) {
		BMessage* msg = new BMessage(M_SELECT_COLLECTION);
		msg->AddInt32("index", i);
		BMenuItem* item = new BMenuItem(fCollections.ItemAt(i)->Name().String(), msg);
		fCollectionMenu->AddItem(item);
		if (i == fActiveCollectionIndex)
			item->SetMarked(true);
	}
	fCollectionMenu->SetTargetForItems(this);

	if (fActiveCollectionIndex >= 0 && fActiveCollectionIndex < fCollections.CountItems()) {
		fCollectionMenuField->MenuItem()->SetLabel(
			fCollections.ItemAt(fActiveCollectionIndex)->Name().String());
	} else {
		fCollectionMenuField->MenuItem()->SetLabel("(no collections)");
	}

	fDeleteCollectionButton->SetEnabled(fActiveCollectionIndex >= 0);
}


void
MainWindow::_RefreshCollectionItemList()
{
	fCollectionListView->MakeEmpty();

	if (fActiveCollectionIndex < 0 || fActiveCollectionIndex >= fCollections.CountItems())
		return;

	Collection* collection = fCollections.ItemAt(fActiveCollectionIndex);
	for (int32 i = 0; i < collection->CountItems(); ++i)
		fCollectionListView->AddItem(new BStringItem(collection->ItemAt(i)->Text()));
}


BString
MainWindow::_BuildFullUrl() const
{
	BString url(fUrlField->Text());
	BString params = fQueryParamsEditor->FormEncodedValues();

	if (params.IsEmpty())
		return url;

	url << (url.FindFirst('?') >= 0 ? "&" : "?");
	url << params;

	return url;
}


BMessage
MainWindow::_ComputedHeaders() const
{
	BMessage headers;

	BString urlText = _BuildFullUrl();
	BUrl url(urlText, false);
	if (url.IsValid()) {
		BMessage h;
		h.AddString("key", "Host");
		h.AddString("value", url.Host());
		headers.AddMessage("header", &h);
	}

	BMenuItem* marked = fMethodMenu->FindMarked();
	BString method(marked ? marked->Label() : "GET");
	bool bodyAllowed = (method != "GET" && method != "HEAD" && fBodyPanel->CurrentMode() != "none");

	if (bodyAllowed) {
		BString body = fBodyPanel->CurrentBody();
		if (body.Length() > 0) {
			BMessage h;
			h.AddString("key", "Content-Type");
			h.AddString("value", fBodyPanel->CurrentContentType());
			headers.AddMessage("header", &h);

			BMessage h2;
			h2.AddString("key", "Content-Length");
			BString len;
			len << body.Length();
			h2.AddString("value", len);
			headers.AddMessage("header", &h2);
		}
	}

	BHttpFields authFields;
	fAuthPanel->ApplyTo(authFields);
	for (const BHttpFields::Field& field : authFields) {
		std::string_view name = field.Name();
		std::string_view value = field.Value();
		BMessage h;
		h.AddString("key", BString(name.data(), name.size()));
		h.AddString("value", BString(value.data(), value.size()));
		headers.AddMessage("header", &h);
	}

	return headers;
}


void
MainWindow::_ApplySettings()
{
	fResponseBodyView->SetWordWrap(fAppSettings->fWordWrap);
	fPreviewPanel->SetWordWrap(fAppSettings->fWordWrap);
	_TrimHistory();
}


void
MainWindow::_ApplyDefaultUserAgent()
{
	if (fAppSettings->fDefaultUserAgent.Length() > 0) {
		fHeadersPanel->AddCustomHeaderIfMissing("User-Agent", fAppSettings->fDefaultUserAgent);
		_UpdatePreview();
	}
}


void
MainWindow::_TrimHistory()
{
	while (fHistoryPanel->CountItems() > fAppSettings->fMaxHistoryItems) {
		BListItem* item = fHistoryPanel->RemoveItem(fHistoryPanel->CountItems() - 1);
		delete item;
	}
	_UpdateHistoryButtons();
}


void
MainWindow::_CancelCurrentRequest()
{
	if (!fCurrentResult.has_value())
		return;

	fSession.Cancel(fCurrentResult.value());
	fCurrentResult.reset();

	_StopRequestTimeout();
}


void
MainWindow::_StopRequestTimeout()
{
	delete fRequestTimeoutRunner;
	fRequestTimeoutRunner = nullptr;
}


BString
MainWindow::_FormatFileSize(int64 bytes)
{
	BString result;
	if (bytes < 1024)
		result << bytes << " B";
	else if (bytes < 1024 * 1024)
		result << (bytes / 1024.0f) << " KB";
	else
		result << (bytes / (1024.0f * 1024.0f)) << " MB";
	return result;
}
