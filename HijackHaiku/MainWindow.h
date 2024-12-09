/*
 * Copyright 2024, Emirhan Ucan <semaemirhan555@gmail.com>
 * All rights reserved. Distributed under the terms of the GNU GPLv2 license.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <FilePanel.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>


class MainWindow : public BWindow
{
public:
							MainWindow();
	virtual					~MainWindow();

	virtual void			MessageReceived(BMessage* msg);

private:
			BMenuBar*		_BuildMenu();

			status_t		_LoadSettings(BMessage& settings);
			status_t		_SaveSettings();

			BMenuItem*		fSaveMenuItem;
			BFilePanel*		fOpenPanel;
			BFilePanel*		fSavePanel;
};

#endif
