/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef BODY_PANEL_H
#define BODY_PANEL_H

#include <Message.h>
#include <String.h>

#include "KeyValueEditor.h"
#include "RadioCardGroup.h"

class BView;
class BHandler;
class BTextView;
class BScrollView;
class BTextControl;
class BButton;
class BFilePanel;

class BodyPanel {
public:
    BodyPanel();
	~BodyPanel();

    BView* View() const { return fRadioGroup->View(); }

    void SetTarget(BHandler* target, uint32 modeChangedWhat);
    void UpdateVisibleCard();

    KeyValueEditor* FormEditor() const { return fFormEditor; }

    void ShowFilePanel(BHandler* target, uint32 refsReceivedWhat);
    void SetFilePath(const BString& path);

    BString CurrentMode() const;       // none - raw - form - (TODO: file)
    BString CurrentBody() const;       // raw text. form-encoded, or file contents
    BString CurrentContentType() const;
    BString CurrentFilePath() const { return fFilePath; }

    void LoadFrom(const BString& mode, const BString& rawBody,
        const BMessage& formParams, const BString& filePath);

private:
    RadioCardGroup* fRadioGroup;
    BTextView*      fRawBodyView;
    KeyValueEditor* fFormEditor;
    BTextControl*   fFilePathField;
    BButton*        fBrowseButton;
    BFilePanel*     fFilePanel;
    BString         fFilePath;
};

#endif // BODY_PANEL_H
