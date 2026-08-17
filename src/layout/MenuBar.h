/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MENU_BAR_H
#define MENU_BAR_H

#include <MenuBar.h>

class BMenuItem;

class MenuBar : public BMenuBar {
public:
    MenuBar();

    BMenuItem* TogglePreview() const { return fTogglePreview; }
    BMenuItem* ToggleSidebar() const { return fToggleSidebar; }

private:
    BMenuItem* fTogglePreview;
    BMenuItem* fToggleSidebar;
};

#endif // MENU_BAR_H
