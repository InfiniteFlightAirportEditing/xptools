/*
 * Copyright (c) 2004, Laminar Research.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <stdio.h>
//#include <strings.h>
#include "XGrinderApp.h"

extern "C"      int strcasecmp(const char *s1, const char *s2);

bool ENV2Overlay(const char * inFileName, const char * inDSF);

void	XGrindFile(const char * inFileName);

void	XGrindFiles(const vector<string>& files)
{
	for (vector<string>::const_iterator i = files.begin(); i != files.end(); ++i)
	{
		XGrindFile(i->c_str());
	}
}

void	XGrindInit(string& t)
{
	t = "ENV2Overlay";
	XGrinder_ShowMessage("Drag a DSF or text file here to convert it.");
}

int	XGrinderMenuPick(xmenu menu, int item)
{
	return 0;
}


void	XGrindFile(const char * inFileName)
{
	char	newname[512];
	int l = strlen(inFileName);
	if (!strcasecmp(inFileName+l-4, ".env"))
	{
		strcpy(newname,inFileName);
		strcat(newname,".dsf");
		if (ENV2Overlay(inFileName, newname))
			XGrinder_ShowMessage("Env -> overlay Conversion successful.");
		else
			XGrinder_ShowMessage("Env -> overlay Conversion Failure.");
	}
}