/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "KeyValueEditor.h"

#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <Url.h>


KeyValueEditor::KeyValueEditor(const char* keyLabel, const char* valueLabel)
{
	fList
		= new BColumnListView("kvList", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE, B_FANCY_BORDER);
	fList->AddColumn(new BStringColumn(keyLabel, 180, 80, 400, 0), 0);
	fList->AddColumn(new BStringColumn(valueLabel, 300, 80, 2000, 0), 1);

	fKeyField = new BTextControl("kvKey", keyLabel, "", nullptr);
	fValueField = new BTextControl("kvValue", valueLabel, "", nullptr);
	fAddButton = new BButton("kvAdd", "Add", nullptr);
	fRemoveButton = new BButton("kvRemove", "Remove", nullptr);

	fView = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
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
KeyValueEditor::SetTarget(BHandler* target, uint32 addWhat, uint32 removeWhat, uint32 selectWhat)
{
	fList->SetInvocationMessage(new BMessage(selectWhat));
	fList->SetTarget(target);

	fAddButton->SetMessage(new BMessage(addWhat));
	fAddButton->SetTarget(target);

	fRemoveButton->SetMessage(new BMessage(removeWhat));
	fRemoveButton->SetTarget(target);
}


void
KeyValueEditor::AddCurrentFields()
{
	BString key(fKeyField->Text());
	BString value(fValueField->Text());
	if (key.Length() == 0)
		return;

	BRow* row = new BRow();
	row->SetField(new BStringField(key.String()), 0);
	row->SetField(new BStringField(value.String()), 1);
	fList->AddRow(row);

	fKeyField->SetText("");
	fValueField->SetText("");
}


void
KeyValueEditor::RemoveSelected()
{
	BRow* selected = fList->CurrentSelection();
	if (selected != nullptr)
		fList->RemoveRow(selected);
}


void
KeyValueEditor::LoadSelectedIntoFields()
{
	BRow* selected = fList->CurrentSelection();
	if (selected == nullptr)
		return;

	auto* keyField = static_cast<BStringField*>(selected->GetField(0));
	auto* valField = static_cast<BStringField*>(selected->GetField(1));
	fKeyField->SetText(keyField->String());
	fValueField->SetText(valField->String());
}


BMessage
KeyValueEditor::CurrentValues() const
{
	BMessage values;
	for (int32 i = 0; i < fList->CountRows(); i++) {
		BRow* row = fList->RowAt(i);
		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		BMessage entry;
		entry.AddString("key", keyField->String());
		entry.AddString("value", valField->String());
		values.AddMessage("param", &entry);
	}
	return values;
}


BString
KeyValueEditor::FormEncodedValues() const
{
	BString values;
	for (int32 i = 0; i < fList->CountRows(); ++i) {
		BRow* row = fList->RowAt(i);
		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		if (values.Length() > 0)
			values << "&";

		values << BUrl::UrlEncode(keyField->String()) << "=" << BUrl::UrlEncode(valField->String());
	}

	return values;
}


void
KeyValueEditor::LoadFrom(const BMessage& values)
{
	fList->Clear();

	BMessage entry;
	for (int32 i = 0; values.FindMessage("param", i, &entry) == B_OK; i++) {
		BString key, value;
		entry.FindString("key", &key);
		entry.FindString("value", &value);

		BRow* row = new BRow();
		row->SetField(new BStringField(key.String()), 0);
		row->SetField(new BStringField(value.String()), 1);
		fList->AddRow(row);

		entry.MakeEmpty();
	}
}
