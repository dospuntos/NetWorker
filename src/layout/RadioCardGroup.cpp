/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "RadioCardGroup.h"

#include <CardLayout.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <View.h>


RadioCardGroup::RadioCardGroup()
	:
	fGlueItem(nullptr)
{
	fRadioRow = new BView("radioRow", 0);
	fRadioRow->SetLayout(new BGroupLayout(B_HORIZONTAL, B_USE_SMALL_SPACING));

	BView* cardsView = new BView("cardsView", 0);
	fCardLayout = new BCardLayout();
	cardsView->SetLayout(fCardLayout);

	fView = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_SMALL_SPACING)
				.SetInsets(B_USE_WINDOW_INSETS)
				.Add(fRadioRow)
				.Add(cardsView)
				.View();
}


int32
RadioCardGroup::AddOption(const char* label, BView* card)
{
	BRadioButton* radio = new BRadioButton(label, label, nullptr);

	auto* layout = dynamic_cast<BGroupLayout*>(fRadioRow->GetLayout());
	if (layout != nullptr) {
		// Fix radio button size locking split view functionality
		if (fGlueItem != nullptr) {
			layout->RemoveItem(fGlueItem);
			delete fGlueItem;
		}

		layout->AddView(radio);

		fGlueItem = BSpaceLayoutItem::CreateGlue();
		layout->AddItem(fGlueItem);
	}

	fCardLayout->AddView(card);

	int32 index = (int32)fRadios.size();
	fRadios.push_back(radio);

	if (index == 0)
		radio->SetValue(B_CONTROL_ON);

	return index;
}


void
RadioCardGroup::SetTarget(BHandler* target, uint32 changedWhat)
{
	for (BRadioButton* radio : fRadios) {
		radio->SetTarget(target);
		radio->SetMessage(new BMessage(changedWhat));
	}
}


void
RadioCardGroup::UpdateVisibleCard()
{
	for (size_t i = 0; i < fRadios.size(); i++) {
		if (fRadios[i]->Value() == B_CONTROL_ON) {
			fCardLayout->SetVisibleItem((int32)i);
			return;
		}
	}
}


int32
RadioCardGroup::SelectedIndex() const
{
	for (size_t i = 0; i < fRadios.size(); i++) {
		if (fRadios[i]->Value() == B_CONTROL_ON)
			return (int32)i;
	}
	return -1;
}


void
RadioCardGroup::SetSelectedIndex(int32 index)
{
	if (index < 0 || index >= (int32)fRadios.size())
		return;
	fRadios[index]->SetValue(B_CONTROL_ON);
	fCardLayout->SetVisibleItem(index);
}
