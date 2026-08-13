/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef RADIO_CARD_GROUP_H
#define RADIO_CARD_GROUP_H

#include <Layout.h>
#include <Message.h>
#include <SupportDefs.h>

#include <vector>

class BView;
class BHandler;
class BRadioButton;
class BCardLayout;

class RadioCardGroup {
public:
    RadioCardGroup();

    BView* View() const { return fView; }

    int32 AddOption(const char* label, BView* card);

    void SetTarget(BHandler* target, uint32 changedWhat);

    void UpdateVisibleCard();

    int32 SelectedIndex() const;
    void SetSelectedIndex(int32 index);

private:
    BView*                      fView;
    BView*                      fRadioRow;
    BCardLayout*                fCardLayout;
    std::vector<BRadioButton*>  fRadios;
	BLayoutItem*				fGlueItem;
};

#endif // RADIO_CARD_GROUP_H
