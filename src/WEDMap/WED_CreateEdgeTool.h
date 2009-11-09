/* 
 * Copyright (c) 2009, Laminar Research.
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

#ifndef WED_CreateEdgeTool_H
#define WED_CreateEdgeTool_H

#include "WED_CreateToolBase.h"

enum CreateEdge_t {
	create_TaxiRoute = 0
};

class	WED_Thing;
class	IGISEntity;

class WED_CreateEdgeTool : public WED_CreateToolBase {
public:

	WED_CreateEdgeTool(
									const char *		tool_name,
									GUI_Pane *			host,
									WED_MapZoomerNew *	zoomer,
									IResolver *			resolver,
									WED_Archive *		archive,
									CreateEdge_t		tool_type);

	virtual				~WED_CreateEdgeTool();
				
	// WED_MapToolNew
	virtual	const char *		GetStatusText(void);

private:

	WED_PropBoolText		mOneway;
	WED_PropStringText		mName;

	virtual	void		AcceptPath(
							const vector<Point2>&	pts,
							const vector<Point2>&	dirs_lo,
							const vector<Point2>&	dirs_hi,
							const vector<int>		has_dirs,
							const vector<int>		has_split,
							int						closed);
	virtual	bool		CanCreateNow(void);

			void		FindNear(WED_Thing * host, IGISEntity * ent, const Point2& loc, WED_Thing *& out_thing, double& out_dsq);
			void		SplitByLine(WED_Thing * host, IGISEntity * ent, const Segment2& s, vector<Point2>& out_splits);
			WED_Thing *	GetHost(int& idx);

		CreateEdge_t	mType;

};

#endif /* WED_CreateEdgeTool_H */