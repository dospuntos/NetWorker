/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ParamsPanel.h"
#include "Constants.h"

#include <Button.h>
#include <ColumnListView.h>
#include <ColumnTypes.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <Url.h>


ParamsPanel::ParamsPanel()
{
	fList = new BColumnListView("paramsList", B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE,
		B_FANCY_BORDER);
	fList->AddColumn(new BStringColumn("Key", 180, 80, 400, 0), 0);
	fList->AddColumn(new BStringColumn("Value", 300, 80, 2000, 0), 1);

	fKeyField = new BTextControl("paramKey", "Key", "", nullptr);
	fValueField = new BTextControl("paramValue", "Value", "", nullptr);
	fAddButton = new BButton("paramAdd", "Add", nullptr);
	fRemoveButton = new BButton("paramRemove", "Remove", nullptr);

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
ParamsPanel::SetTarget(BHandler* target)
{
	fList->SetInvocationMessage(new BMessage(M_SELECT_PARAMETER));
	fList->SetTarget(target);

	fAddButton->SetMessage(new BMessage(M_ADD_PARAMETER));
	fAddButton->SetTarget(target);

	fRemoveButton->SetMessage(new BMessage(M_REMOVE_PARAMETER));
	fRemoveButton->SetTarget(target);
}


void
ParamsPanel::AddCurrentFields()
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
ParamsPanel::RemoveSelected()
{
	BRow* selected = fList->CurrentSelection();
	if (selected != nullptr)
		fList->RemoveRow(selected);
}


void
ParamsPanel::LoadSelectedIntoFields()
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
ParamsPanel::CurrentParams() const
{
	BMessage params;
	for (int32 i = 0; i < fList->CountRows(); ++i) {
		BRow* row = fList->RowAt(i);
		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		BMessage param;
		param.AddString("key", keyField->String());
		param.AddString("value", valField->String());
		params.AddMessage("param", &param);
	}
	return params;
}


BString
ParamsPanel::FormEncodedParams() const
{
	BString result;
	for (int32 i = 0; i < fList->CountRows(); ++i) {
		BRow* row = fList->RowAt(i);
		auto* keyField = static_cast<BStringField*>(row->GetField(0));
		auto* valField = static_cast<BStringField*>(row->GetField(1));

		if (result.Length() > 0)
			result << "&";

		result << BUrl::UrlEncode(keyField->String()) << "=" << BUrl::UrlEncode(valField->String());
	}

	return result;
}


void
ParamsPanel::LoadFrom(const BMessage& params)
{
	fList->Clear();

	BMessage param;
	for (int32 i = 0; params.FindMessage("param", i, &param) == B_OK; ++i) {
		BString key, value;
		param.FindString("key", &key);
		param.FindString("value", &value);

		BRow* row = new BRow();
		row->SetField(new BStringField(key.String()), 0);
		row->SetField(new BStringField(value.String()), 1);
		fList->AddRow(row);

		param.MakeEmpty();
	}
}
