/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "HeadersPanel.h"

#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <ControlLook.h>

#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <TextControl.h>

namespace {

class HeaderRow : public BRow {
public:
    HeaderRow(bool isCustom) : BRow(), fIsCustom(isCustom) {}
    bool fIsCustom;
};

class HeaderStringField : public BStringField {
public:
    HeaderStringField(const char* string, bool gray)
        : BStringField(string), fGray(gray) {}
    bool fGray;
};

class HeaderStringColumn : public BStringColumn {
public:
    using BStringColumn::BStringColumn;

    void DrawField(BField* field, BRect rect, BView* targetView) override
    {
        auto* gf = dynamic_cast<HeaderStringField*>(field);

        if (gf == nullptr)
            return;

        rgb_color color = gf->fGray
			? ui_color(B_CONTROL_MARK_COLOR)
			: ui_color(B_PANEL_TEXT_COLOR);

        targetView->SetHighColor(color);
        targetView->SetLowColor(targetView->ViewColor());
        targetView->SetDrawingMode(B_OP_OVER);

        BFont font;
        targetView->GetFont(&font);
        font_height fh;
        font.GetHeight(&fh);

        BString text(gf->String());
        targetView->TruncateString(&text, B_TRUNCATE_END, rect.Width() - 8);

        float y = rect.top + (rect.Height() - (fh.ascent + fh.descent)) / 2 + fh.ascent;
        targetView->DrawString(text.String(), BPoint(rect.left + 4, y));
    }
};
} // namespace


HeadersPanel::HeadersPanel()
{
	fList = new BColumnListView("headersList", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_FANCY_BORDER);
	fList->AddColumn(new HeaderStringColumn("Header", 180, 80, 400, 0), 0);
	fList->AddColumn(new HeaderStringColumn("Value", 300, 80, 2000, 0), 1);

	fKeyField = new BTextControl("headerKey", "Header name", "", nullptr);
	fValueField = new BTextControl("headerValue", "Value", "", nullptr);
	fAddButton = new BButton("headerAdd", "Add", nullptr);
	fRemoveButton = new BButton("headerRemove", "Remove", nullptr);

	fView = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_WINDOW_INSETS)
				.Add(fList)
				.AddGroup(B_HORIZONTAL, B_USE_SMALL_SPACING)
					.Add(fKeyField)
					.Add(fValueField)
					.Add(fAddButton)
					.Add(fRemoveButton)
					.End()
				.View();
}


void
HeadersPanel::SetTarget(BHandler* target, uint32 addWhat, uint32 removeWhat, uint32 selectWhat)
{
	fList->SetInvocationMessage(new BMessage(selectWhat));
	fList->SetTarget(target);
	fAddButton->SetMessage(new BMessage(addWhat));
	fAddButton->SetTarget(target);
	fRemoveButton->SetMessage(new BMessage(removeWhat));
	fRemoveButton->SetTarget(target);
}


void
HeadersPanel::AddCurrentFields()
{
	BString key(fKeyField->Text());
	BString value(fValueField->Text());
	if (key.Length() == 0)
		return;

	HeaderRow* row = new HeaderRow(true); // user-added
	row->SetField(new HeaderStringField(key.String(), false), 0);
	row->SetField(new HeaderStringField(value.String(), false), 1);
	fList->AddRow(row);

	fKeyField->SetText("");
	fValueField->SetText("");
}


void
HeadersPanel::RemoveSelected()
{
	BRow* selected = fList->CurrentSelection();
	if (selected == nullptr)
		return;

	auto* row = static_cast<HeaderRow*>(selected);
	if (!row->fIsCustom)
		return; // leave system headers

	fList->RemoveRow(selected);
}


void
HeadersPanel::LoadSelectedIntoFields()
{
	BRow* selected = fList->CurrentSelection();
	if (selected == nullptr)
		return;

	auto* row = static_cast<HeaderRow*>(selected);
	if (!row->fIsCustom)
		return; // ignore system headers

	auto* keyField = static_cast<HeaderStringField*>(selected->GetField(0));
	auto* valField = static_cast<HeaderStringField*>(selected->GetField(1));
	fKeyField->SetText(keyField->String());
	fValueField->SetText(valField->String());
}


BMessage
HeadersPanel::CurrentHeaders() const
{
	BMessage headers;
	for (int32 i = 0; i < fList->CountRows(); i++) {
		auto* row = static_cast<HeaderRow*>(fList->RowAt(i));
		if (!row->fIsCustom)
			continue;

		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		BMessage h;
		h.AddString("key", keyField->String());
		h.AddString("value", valField->String());
		headers.AddMessage("header", &h);
	}
	return headers;
}


void
HeadersPanel::LoadFrom(const BMessage& headers)
{
	// Remove custom rows only
	for (int32 i = fList->CountRows() - 1; i >= 0; i--) {
		auto* row = static_cast<HeaderRow*>(fList->RowAt(i));
		if (row->fIsCustom)
			fList->RemoveRow(row);
	}

	BMessage h;
	for (int32 i = 0; headers.FindMessage("header", i, &h) == B_OK; i++) {
		BString key, value;
		h.FindString("key", &key);
		h.FindString("value", &value);

		HeaderRow* row = new HeaderRow(true);
		row->SetField(new HeaderStringField(key.String(), false), 0);
		row->SetField(new HeaderStringField(value.String(), false), 1);
		fList->AddRow(row);

		h.MakeEmpty();
	}
}


void
HeadersPanel::SetComputedHeaders(const BMessage& headers)
{
	// Remove system rows only, leave custom rows exactly as they are.
	for (int32 i = fList->CountRows() - 1; i >= 0; i--) {
		auto* row = static_cast<HeaderRow*>(fList->RowAt(i));
		if (!row->fIsCustom)
			fList->RemoveRow(row);
	}

	BMessage h;
	int32 insertIndex = 0;
	for (int32 i = 0; headers.FindMessage("header", i, &h) == B_OK; i++) {
		BString key, value;
		h.FindString("key", &key);
		h.FindString("value", &value);

		HeaderRow* row = new HeaderRow(false);
		row->SetField(new HeaderStringField(key.String(), true), 0);
		row->SetField(new HeaderStringField(value.String(), true), 1);
		fList->AddRow(row, insertIndex++); // keep computed rows at the top

		h.MakeEmpty();
	}
}


bool
HeadersPanel::HasCustomHeader(const BString& key) const
{
	for (int32 i = 0; i < fList->CountRows(); ++i) {
		auto* row = static_cast<HeaderRow*>(fList->RowAt(i));
		if (!row->fIsCustom)
			continue;

		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		if (key.ICompare(keyField->String()) == 0)
			return true;
	}
	return false;
}


void
HeadersPanel::AddCustomHeaderIfMissing(const BString& key, const BString& value)
{
	if (value.Length() == 0 || HasCustomHeader(key))
		return;

	HeaderRow* row = new HeaderRow(true);
	row->SetField(new HeaderStringField(key.String(), false), 0);
	row->SetField(new HeaderStringField(value.String(), false), 1);
	fList->AddRow(row);
}