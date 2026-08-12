/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef AUTH_PANEL_H
#define AUTH_PANEL_H

#include <Message.h>
#include <String.h>

class BView;
class BHandler;
class BRadioButton;
class BCardLayout;
class BTextControl;

namespace BPrivate {
namespace Network {
class BHttpFields;
}
} // namespace BPrivate
using BPrivate::Network::BHttpFields;

class AuthPanel {
public:
    AuthPanel();

    BView* View() const { return fView; }

    void SetTarget(BHandler* target, uint32 changedWhat);
    void UpdateVisibleCard();
    void ApplyTo(BHttpFields& fields) const;
    BString CurrentType() const;
    BMessage CurrentValues() const;
    void LoadFrom(const BString& type, const BMessage& values);

private:
    BView*        fView;
    BRadioButton* fNoneRadio;
    BRadioButton* fBasicRadio;
    BRadioButton* fBearerRadio;
    BRadioButton* fApiKeyRadio;
    BCardLayout*  fCardLayout;
    BTextControl* fUsernameField;
    BTextControl* fPasswordField;
    BTextControl* fTokenField;
    BTextControl* fApiKeyNameField;
    BTextControl* fApiKeyValueField;
};

#endif // AUTH_PANEL_H
