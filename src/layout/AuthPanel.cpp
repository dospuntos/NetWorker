/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AuthPanel.h"
#include "Constants.h"

#include <ControlLook.h>
#include <HttpFields.h>
#include <LayoutBuilder.h>
#include <TextControl.h>
#include <TextView.h>
#include <View.h>

#include <algorithm>

using BPrivate::Network::BHttpFields;

namespace {


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


AuthPanel::AuthPanel()
{
	fRadioGroup = new RadioCardGroup();

	BView* noneCard = new BView("authNoneCard", B_WILL_DRAW);
	fRadioGroup->AddOption("None", noneCard); // index 0

	fUsernameField = new BTextControl("authUsername", "Username", "", nullptr);
	fUsernameField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	fPasswordField = new BTextControl("authPassword", "Password", "", nullptr);
	fPasswordField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	fPasswordField->TextView()->HideTyping(true);
	BView* basicCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
						   .Add(fUsernameField)
						   .Add(fPasswordField)
						   .AddGlue()
						   .View();
	fRadioGroup->AddOption("Basic", basicCard); // index 1

	fTokenField = new BTextControl("authToken", "Token", "", nullptr);
	fTokenField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	BView* bearerCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							.Add(fTokenField)
							.AddGlue()
							.View();
	fRadioGroup->AddOption("Bearer token", bearerCard); // index 2

	fApiKeyNameField = new BTextControl("authApiKeyName", "Header name", "", nullptr);
	fApiKeyNameField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	fApiKeyValueField = new BTextControl("authApiKeyValue", "Value", "", nullptr);
	fApiKeyValueField->SetModificationMessage(new BMessage(M_UPDATE_PREVIEW));
	BView* apiKeyCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							.Add(fApiKeyNameField)
							.Add(fApiKeyValueField)
							.AddGlue()
							.View();
	fRadioGroup->AddOption("API key", apiKeyCard); // index 3
}


AuthPanel::~AuthPanel()
{
	delete fRadioGroup;
}


void
AuthPanel::SetTarget(BHandler* target, uint32 changedWhat)
{
	fRadioGroup->SetTarget(target, changedWhat);
}


void
AuthPanel::UpdateVisibleCard()
{
	fRadioGroup->UpdateVisibleCard();
}


BString
AuthPanel::CurrentType() const
{
	switch (fRadioGroup->SelectedIndex()) {
		case 1:
			return "basic";
		case 2:
			return "bearer";
		case 3:
			return "apikey";
		default:
			return "none";
	}
}


BMessage
AuthPanel::CurrentValues() const
{
	BMessage values;
	BString type = CurrentType();

	if (type == "basic") {
		values.AddString("username", fUsernameField->Text());
		values.AddString("password", fPasswordField->Text());
	} else if (type == "bearer") {
		values.AddString("token", fTokenField->Text());
	} else if (type == "apikey") {
		values.AddString("headerName", fApiKeyNameField->Text());
		values.AddString("headerValue", fApiKeyValueField->Text());
	}

	return values;
}


void
AuthPanel::ApplyTo(BHttpFields& fields) const
{
	BString type = CurrentType();

	if (type == "basic") {
		BString credentials;
		credentials << fUsernameField->Text() << ":" << fPasswordField->Text();
		if (credentials.Length() > 1) {
			BString header;
			header << "Basic " << Base64Encode(credentials);
			fields.AddField("Authorization", header.String());
		}
	} else if (type == "bearer") {
		BString token(fTokenField->Text());
		if (token.Length() > 0) {
			BString header;
			header << "Bearer " << token;
			fields.AddField("Authorization", header.String());
		}
	} else if (type == "apikey") {
		BString name(fApiKeyNameField->Text());
		BString value(fApiKeyValueField->Text());
		if (name.Length() > 0 && value.Length() > 0)
			fields.AddField(name.String(), value.String());
	}
}


void
AuthPanel::LoadFrom(const BString& type, const BMessage& values)
{
	BString username, password, token, headerName, headerValue;
	values.FindString("username", &username);
	values.FindString("password", &password);
	values.FindString("token", &token);
	values.FindString("headerName", &headerName);
	values.FindString("headerValue", &headerValue);

	fUsernameField->SetText(username.String());
	fPasswordField->SetText(password.String());
	fTokenField->SetText(token.String());
	fApiKeyNameField->SetText(headerName.String());
	fApiKeyValueField->SetText(headerValue.String());

	int32 index = 0;
	if (type == "basic")
		index = 1;
	else if (type == "bearer")
		index = 2;
	else if (type == "apikey")
		index = 3;

	fRadioGroup->SetSelectedIndex(index);
}
