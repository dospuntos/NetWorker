/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef PARAMS_PANEL_H
#define PARAMS_PANEL_H

#include <Message.h>
#include <String.h>
#include <SupportDefs.h>

class BView;
class BHandler;
class BColumnListView;
class BTextControl;
class BButton;

class ParamsPanel {
public:
    ParamsPanel();

    BView* View() const { return fView; }

    void SetTarget(BHandler* target);
    void AddCurrentFields();
    void RemoveSelected();
    void LoadSelectedIntoFields();

    BMessage CurrentParams() const;
	BString FormEncodedParams() const;
    void LoadFrom(const BMessage& params);

private:
    BView*            fView;
    BColumnListView*  fList;
    BTextControl*     fKeyField;
    BTextControl*     fValueField;
    BButton*          fAddButton;
    BButton*          fRemoveButton;
};

#endif // PARAMS_PANEL_H
