/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AuthPanel.h"

#include <CardLayout.h>
#include <ControlLook.h>
#include <HttpFields.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
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
	fNoneRadio = new BRadioButton("authNone", "None", nullptr);
	fBasicRadio = new BRadioButton("authBasic", "Basic", nullptr);
	fBearerRadio = new BRadioButton("authBearer", "Bearer token", nullptr);
	fApiKeyRadio = new BRadioButton("authApiKey", "API key", nullptr);
	fNoneRadio->SetValue(B_CONTROL_ON);

	BView* typeRow = BLayoutBuilder::Group<>(B_HORIZONTAL, B_USE_SMALL_SPACING)
						 .Add(fNoneRadio)
						 .Add(fBasicRadio)
						 .Add(fBearerRadio)
						 .Add(fApiKeyRadio)
						 .AddGlue()
						 .View();

	BView* noneCard = new BView("authNoneCard", B_WILL_DRAW);

	fUsernameField = new BTextControl("authUsername", "Username", "", nullptr);
	fPasswordField = new BTextControl("authPassword", "Password", "", nullptr);
	fPasswordField->TextView()->HideTyping(true);
	BView* basicCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
						   .Add(fUsernameField)
						   .Add(fPasswordField)
						   .AddGlue()
						   .View();

	fTokenField = new BTextControl("authToken", "Token", "", nullptr);
	BView* bearerCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							.Add(fTokenField)
							.AddGlue()
							.View();

	fApiKeyNameField = new BTextControl("authApiKeyName", "Header name", "", nullptr);
	fApiKeyValueField = new BTextControl("authApiKeyValue", "Value", "", nullptr);
	BView* apiKeyCard = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
							.Add(fApiKeyNameField)
							.Add(fApiKeyValueField)
							.AddGlue()
							.View();

	BView* cardsView = new BView("authCards", 0);
	fCardLayout = new BCardLayout();
	cardsView->SetLayout(fCardLayout);
	fCardLayout->AddView(noneCard);
	fCardLayout->AddView(basicCard);
	fCardLayout->AddView(bearerCard);
	fCardLayout->AddView(apiKeyCard);
	fCardLayout->SetVisibleItem((int32)0);

	fView = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_WINDOW_INSETS)
				.Add(typeRow)
				.Add(cardsView)
				.View();
}


void
AuthPanel::SetTarget(BHandler* target, uint32 changedWhat)
{
	fNoneRadio->SetTarget(target);
	fBasicRadio->SetTarget(target);
	fBearerRadio->SetTarget(target);
	fApiKeyRadio->SetTarget(target);

	fNoneRadio->SetMessage(new BMessage(changedWhat));
	fBasicRadio->SetMessage(new BMessage(changedWhat));
	fBearerRadio->SetMessage(new BMessage(changedWhat));
	fApiKeyRadio->SetMessage(new BMessage(changedWhat));
}


void
AuthPanel::UpdateVisibleCard()
{
	int32 index = 0;
	if (fBasicRadio->Value() == B_CONTROL_ON)
		index = 1;
	else if (fBearerRadio->Value() == B_CONTROL_ON)
		index = 2;
	else if (fApiKeyRadio->Value() == B_CONTROL_ON)
		index = 3;

	fCardLayout->SetVisibleItem(index);
}


BString
AuthPanel::CurrentType() const
{
	if (fBasicRadio->Value() == B_CONTROL_ON)
		return "basic";
	if (fBearerRadio->Value() == B_CONTROL_ON)
		return "bearer";
	if (fApiKeyRadio->Value() == B_CONTROL_ON)
		return "apikey";
	return "none";
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
		BString header;
		header << "Basic " << Base64Encode(credentials);
		fields.AddField("Authorization", header.String());
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
		if (name.Length() > 0)
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

	if (type == "basic")
		fBasicRadio->SetValue(B_CONTROL_ON);
	else if (type == "bearer")
		fBearerRadio->SetValue(B_CONTROL_ON);
	else if (type == "apikey")
		fApiKeyRadio->SetValue(B_CONTROL_ON);
	else
		fNoneRadio->SetValue(B_CONTROL_ON);

	UpdateVisibleCard();
}
