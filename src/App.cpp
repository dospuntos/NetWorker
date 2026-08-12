/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "App.h"
#include "MainWindow.h"
#include "Constants.h"

#include <AboutWindow.h>
#include <Catalog.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"


App::App()
	:
	BApplication(kApplicationSignature)
{
	MainWindow* mainWindow = new MainWindow();
	mainWindow->Show();
}


App::~App()
{
}


void
App::AboutRequested()
{
	BAboutWindow* about
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("NetWorker"), kApplicationSignature);
	about->AddDescription(B_TRANSLATE("A native REST API client for Haiku."));
	about->AddCopyright(2026, "Johan Wagenheim");
	about->AddText(B_TRANSLATE("Distributed under the terms of the MIT License."));
	about->Show();
}


int
main()
{
	App* app = new App();
	app->Run();
	delete app;
	return 0;
}
