/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RENAME_WINDOW_H
#define RENAME_WINDOW_H

#include <Messenger.h>
#include <String.h>
#include <Window.h>

class BTextControl;

class RenameWindow : public BWindow {
public:
    RenameWindow(BRect frame, const BString& initialText, int32 itemIndex,
        const BMessenger& target, uint32 resultWhat);

    void MessageReceived(BMessage* message) override;

private:
    BTextControl* fTextField;
    int32         fItemIndex;
    BMessenger    fTarget;
    uint32        fResultWhat;
};

#endif // RENAME_WINDOW_H
