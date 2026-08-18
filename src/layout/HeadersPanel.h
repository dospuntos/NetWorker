/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef HEADERS_PANEL_H
#define HEADERS_PANEL_H

#include <Message.h>
#include <String.h>

class BView;
class BHandler;
class BColumnListView;
class BTextControl;
class BButton;

class HeadersPanel {
public:
    HeadersPanel();

    BView* View() const { return fView; }

    void SetTarget(BHandler* target, uint32 addWhat, uint32 removeWhat, uint32 selectWhat);
    void AddCurrentFields();
    void RemoveSelected();
    void LoadSelectedIntoFields();

    BMessage CurrentHeaders() const;
    void LoadFrom(const BMessage& headers);

    void SetComputedHeaders(const BMessage& headers);
	bool HasCustomHeader(const BString& key) const;
	void AddCustomHeaderIfMissing(const BString& key, const BString& value);

private:
    BView*            fView;
    BColumnListView*  fList;
    BTextControl*     fKeyField;
    BTextControl*     fValueField;
    BButton*          fAddButton;
    BButton*          fRemoveButton;
};

#endif // HEADERS_PANEL_H
