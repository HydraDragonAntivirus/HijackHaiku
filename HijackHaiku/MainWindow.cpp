/*
 * Copyright 2024, Emirhan Ucan <semaemirhan555@gmail.com>
 * All rights reserved. Distributed under the terms of the GNU GPLv2 license.
 */

#include "MainWindow.h"

#include <Application.h>
#include <Catalog.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <Path.h>
#include <View.h>
#include <Button.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <TextView.h>
#include <ScrollView.h>    // Add this for BScrollView
#include <Alert.h>         // Add this for BAlert
#include <AppKit.h>        // Add this for app_info

#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

static const uint32 kMsgNewFile = 'fnew';
static const uint32 kMsgOpenFile = 'fopn';
static const uint32 kMsgSaveFile = 'fsav';

static const char* kSettingsFile = "MyApplication_settings";

// Add this member to MainWindow class
BButton* fScanButton;

// Add this member to MainWindow class
BTextView* fResultsView;


MainWindow::MainWindow()
    :
    BWindow(BRect(100, 100, 700, 500), B_TRANSLATE("HijackHaiku"), B_TITLED_WINDOW,
        B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
    BMenuBar* menuBar = _BuildMenu();

    fScanButton = new BButton("Start Scan", new BMessage('SCAN'));

    // Create the TextView and wrap it in a ScrollView
    fResultsView = new BTextView("ResultsView");
    fResultsView->SetStylable(true); // Enable text styling if needed
    fResultsView->MakeEditable(false); // Make it read-only
    fResultsView->SetText("Scan results will appear here...");

    // Set a minimum size for the TextView
    fResultsView->SetExplicitMinSize(BSize(400, 300)); // Minimum width and height

    // Wrap TextView in a BScrollView
    BScrollView* resultsScrollView = new BScrollView(
        "ResultsScrollView", fResultsView, B_WILL_DRAW | B_FRAME_EVENTS, false, true);

    BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
        .Add(menuBar)
        .Add(fScanButton)
        .AddStrut(10) // Add some spacing
        .AddGroup(B_VERTICAL)
            .SetInsets(10)
            .Add(resultsScrollView) // Add the ScrollView
        .End()
        .AddGlue()
        .End();

    BMessenger messenger(this);
    fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE, false);
    fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, B_FILE_NODE, false);

    BMessage settings;
    _LoadSettings(settings);

    BRect frame;
    if (settings.FindRect("main_window_rect", &frame) == B_OK) {
        MoveTo(frame.LeftTop());
        ResizeTo(frame.Width(), Bounds().Height());
    }
    MoveOnScreen();
}


void MainWindow::_PerformSystemScan()
{
    BString results;
    results << "System Scan Results:\n\n";

    // Check if the '/packages' directory exists
    BDirectory packagesDirectory("/packages");
    if (packagesDirectory.InitCheck() == B_OK) {
        results << "Files in /packages:\n";
        BEntry entry;
        while (packagesDirectory.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);
            results << "  - " << path.Path() << "\n";
        }
    } else {
        results << "  (Directory '/packages' not found)\n";
    }

    // Check the '/boot/home/config' directory and look for "settings"
    BDirectory configDirectory("/boot/home/config");
    if (configDirectory.InitCheck() == B_OK) {
        results << "\nFiles in /boot/home/config:\n";
        BEntry entry;
        bool settingsFound = false;
        while (configDirectory.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);

            // Check if the directory ends with 'settings'
            if (path.Leaf() == "settings") {
                settingsFound = true;
            }
            results << "  - " << path.Path() << "\n";
        }
        if (!settingsFound) {
            results << "  (Directory '/boot/home/config' does not contain 'settings')\n";
        }
    } else {
        results << "  (Directory '/boot/home/config' not found)\n";
    }

    // Check the '/boot/home/Desktop' directory and subdirectories for files without extensions
    BDirectory desktopDirectory("/boot/home/Desktop");
    if (desktopDirectory.InitCheck() == B_OK) {
        results << "\nApplications on Desktop without extensions:\n";
        BEntry entry;
        while (desktopDirectory.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);

            // If it's a directory, we need to check subfolders as well
            if (entry.IsDirectory()) {
                // Recursively check the subfolders
                _CheckDesktopSubfoldersForNoExtension(path.Path(), results);
            } else {
                // Check if the file does not have an extension
                BString leafName = path.Leaf();
                int32 dotPos = leafName.FindLast('.');
                if (dotPos == B_ERROR) { // No dot found, so no extension
                    results << "  - " << path.Path() << "\n";
                }
            }
        }
    } else {
        results << "  (Directory '/boot/home/Desktop' not found)\n";
    }

    // Get installed packages via "pkgman list"
    results << "\nInstalled Packages:\n";
    FILE* pkgmanOutput = popen("pkgman list", "r");
    if (pkgmanOutput) {
        char buffer[1024]; // Use a larger buffer for efficiency
        while (fgets(buffer, sizeof(buffer), pkgmanOutput)) {
            results << buffer;
        }
        pclose(pkgmanOutput);
    } else {
        results << "  (Failed to retrieve installed packages)\n";
    }

    results << "\nScan completed.\n";

    // Update the BTextView with the scan results
    fResultsView->SetText(results);

    // Save to a log file
    _SaveLogFile(results);
}

// Recursive function to check subfolders on the Desktop for files without extensions
void MainWindow::_CheckDesktopSubfoldersForNoExtension(const char* folderPath, BString& results)
{
    BDirectory subDir(folderPath);
    if (subDir.InitCheck() == B_OK) {
        BEntry entry;
        while (subDir.GetNextEntry(&entry) == B_OK) {
            BPath path;
            entry.GetPath(&path);

            // If it's a directory, recurse deeper
            if (entry.IsDirectory()) {
                _CheckDesktopSubfoldersForNoExtension(path.Path(), results);
            } else {
                // Check if the file does not have an extension
                BString leafName = path.Leaf();
                int32 dotPos = leafName.FindLast('.');
                if (dotPos == B_ERROR) { // No dot found, so no extension
                    results << "  - " << path.Path() << "\n";
                }
            }
        }
    }
}


void
MainWindow::_SaveLogFile(const BString& results)
{
    app_info appInfo;
    if (be_app->GetAppInfo(&appInfo) == B_OK) {
        BPath appPath(&appInfo.ref); // Path to the executable
        appPath.GetParent(&appPath); // Get the directory of the executable
        appPath.Append("scan_results.log"); // Append the log file name

        BFile logFile(appPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
        if (logFile.InitCheck() == B_OK) {
            logFile.Write(results.String(), results.Length());
            printf("Log saved to: %s\n", appPath.Path());
        } else {
            BAlert* alert = new BAlert("Error", "Failed to save log file.", "OK");
            alert->Go();
        }
    } else {
        BAlert* alert = new BAlert("Error", "Failed to retrieve application info.", "OK");
        alert->Go();
    }
}


MainWindow::~MainWindow()
{
	_SaveSettings();

	delete fOpenPanel;
	delete fSavePanel;
}


void
MainWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case B_SIMPLE_DATA:
        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if (message->FindRef("refs", &ref) != B_OK)
                break;

            fSaveMenuItem->SetEnabled(true);
            printf("File opened/dropped\n");
        } break;

        case B_SAVE_REQUESTED:
        {
            entry_ref ref;
            const char* name;
            if (message->FindRef("directory", &ref) == B_OK
                && message->FindString("name", &name) == B_OK) {
                BDirectory directory(&ref);
                BEntry entry(&directory, name);
                BPath path = BPath(&entry);

                printf("Save path: %s\n", path.Path());
            }
        } break;

        case 'SCAN':
        {
            _PerformSystemScan();
            break;
        }

        default:
        {
            BWindow::MessageReceived(message);
            break;
        }
    }
}


BMenuBar*
MainWindow::_BuildMenu()
{
	BMenuBar* menuBar = new BMenuBar("menubar");
	BMenu* menu;
	BMenuItem* item;

	// menu 'File'
	menu = new BMenu(B_TRANSLATE("File"));

	item = new BMenuItem(B_TRANSLATE("New"), new BMessage(kMsgNewFile), 'N');
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), new BMessage(kMsgOpenFile), 'O');
	menu->AddItem(item);

	fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"), new BMessage(kMsgSaveFile), 'S');
	fSaveMenuItem->SetEnabled(false);
	menu->AddItem(fSaveMenuItem);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(item);

	menuBar->AddItem(menu);

	return menuBar;
}


status_t
MainWindow::_LoadSettings(BMessage& settings)
{
	BPath path;
	status_t status;
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
MainWindow::_SaveSettings()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings;
	status = settings.AddRect("main_window_rect", Frame());

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}
