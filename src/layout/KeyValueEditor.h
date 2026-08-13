/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef KEY_VALUE_EDITOR_H
#define KEY_VALUE_EDITOR_H

#include <Message.h>
#include <String.h>
#include <SupportDefs.h>

class BView;
class BHandler;
class BColumnListView;
class BTextControl;
class BButton;

class KeyValueEditor {
public:
    KeyValueEditor(const char* keyLabel = "Key", const char* valueLabel = "Value");

    BView* View() const { return fView; }

    void SetTarget(BHandler* target, uint32 addWhat, uint32 removeWhat, uint32 selectWhat);

    void AddCurrentFields();
    void RemoveSelected();
    void LoadSelectedIntoFields();

    BMessage CurrentValues() const;
	BString FormEncodedValues() const;
    void LoadFrom(const BMessage& values);

private:
    BView*            fView;
    BColumnListView*  fList;
    BTextControl*     fKeyField;
    BTextControl*     fValueField;
    BButton*          fAddButton;
    BButton*          fRemoveButton;
};

#endif // KEY_VALUE_EDITOR_H
