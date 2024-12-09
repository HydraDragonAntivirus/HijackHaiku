/*
 * Copyright 2024, Emirhan Ucan <semaemirhan555@gmail.com>
 * All rights reserved. Distributed under the terms of the GNU GPLv2 license.
 */

#ifndef APP_H
#define APP_H


#include <Application.h>


class App : public BApplication
{
public:
							App();
	virtual					~App();

	virtual void			AboutRequested();

private:
};

#endif // APP_H

