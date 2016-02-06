/*
 * Copyright (c) 2007, Laminar Research.
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

#include "WED_GISChain.h"

TRIVIAL_COPY(WED_GISChain, WED_Entity)

WED_GISChain::WED_GISChain(WED_Archive * parent, int id) :
	WED_Entity(parent, id)
{
}

WED_GISChain::~WED_GISChain()
{
}

GISClass_t		WED_GISChain::GetGISClass		(void				 ) const
{
	if(IsJustPoints()) return gis_Composite;
	return IsClosed() ? gis_Ring : gis_Chain;
}

const char *	WED_GISChain::GetGISSubtype	(void				 ) const
{
	return GetClass();
}

bool			WED_GISChain::HasLayer			(GISLayer_t l) const
{
	RebuildCache(CacheBuild(cache_Topological));
	return (l == gis_UV) ? mHasUV : true;
}

void			WED_GISChain::GetBounds		(GISLayer_t l, Bbox2&  bounds) const
{
	RebuildCache(CacheBuild(cache_Spatial));
	bounds = (l == gis_UV) ? mCacheBoundsUV : mCacheBounds;
}

bool				WED_GISChain::IntersectsBox	(GISLayer_t l,const Bbox2&  bounds) const
{
	Bbox2	me;
	GetBounds(l,me);
	if (!bounds.overlap(me)) return false;

	#if BENTODO
		this is not good enough
	#endif
	return true;
}

bool			WED_GISChain::WithinBox		(GISLayer_t l,const Bbox2&  bounds) const
{
	Bbox2	me;
	GetBounds(l,me);
	if (bounds.contains(me)) return true;

	int n = GetNumSides();
	for (int i = 0; i < n; ++i)
	{
		Segment2 s;
		Bezier2 b;
		if (GetSide(l,i,s,b))
		{
			Bbox2	bb;
			b.bounds(bb);
			// Ben says: bounding-box of a bezier is assured to touch the extreme points of the bezier.
			// Thus if the bezier's bounding box is inside, the whole bezier must be inside since all extreme
			// points are.  And since there is no "slop", this is the only test we need.
			if (!bounds.contains(bb)) return false;
		} else {
			if (!bounds.contains(s.p1)) return false;
			if (!bounds.contains(s.p2)) return false;
		}
	}
	return true;
}

bool			WED_GISChain::PtWithin		(GISLayer_t l,const Point2& p	 ) const
{
	// Rings do NOT contain area!  They are just lines that are self-connected.
	return false;
}

bool			WED_GISChain::PtOnFrame		(GISLayer_t l,const Point2& p, double d	 ) const
{
	Bbox2	me;
	GetBounds(l,me);
	me.p1 -= Vector2(d,d);
	me.p2 += Vector2(d,d);
	if (!me.contains(p)) return false;

	int c = GetNumSides();
	for (int n = 0; n < c; ++n)
	{
		Segment2 s;
		Bezier2 b;
		if (GetSide(l,n,s,b))
		{
			if (b.is_near(p,d)) return true;
		} else {
			if (s.is_near(p,d)) return true;
		}
	}
	return false;
}

bool	WED_GISChain::Cull(const Bbox2& bounds) const
{
	Bbox2	me;
	GetBounds(gis_Geo, me);
	return bounds.overlap(me);
}


void			WED_GISChain::Rescale			(GISLayer_t l,const Bbox2& old_bounds,const Bbox2& new_bounds)
{
	int t = GetNumPoints();
	for (int n = 0; n <  t; ++n)
	{
		IGISPoint * p = GetNthPoint(n);
		p->Rescale(l,old_bounds,new_bounds);
	}
}

IGISPoint *	WED_GISChain::SplitSide   (const Point2& p, double dist)
{
	int s = GetNumSides();
	int best = -1;
	double d = dist*dist;
	Segment2	best_p;
	for(int n = 0; n < s; ++n)
	{
		Segment2 s;
		Bezier2 b;
		if(!GetSide(gis_Geo, n, s, b))
		{
			double dd = s.squared_distance_supporting_line(p);
			if (dd < d && s.collinear_has_on(p))
			{
				d = dd;
				best = n;
				best_p = s;
			}
		}
	}
	if(best != -1)
	{
		WED_Thing * np = dynamic_cast<WED_Thing*>(GetNthChild(best)->Clone());
		IGISPoint * npp = dynamic_cast<IGISPoint *>(np);		
		Point2	bpp_proj = best_p.projection(p);
		npp->SetLocation(gis_Geo, bpp_proj);
		
		if(HasLayer(gis_UV))
		{
			double t = sqrt(best_p.p1.squared_distance(bpp_proj)) /
					   sqrt(best_p.squared_length());
			Segment2 uvs;
			Bezier2  uvb;
			if(GetSide(gis_UV, best, uvs, uvb))
				npp->SetLocation(gis_UV, uvb.midpoint(t));
			else
				npp->SetLocation(gis_UV, uvs.midpoint(t));
		}
		// Why wait?  Cuz...Getside on old side will USE this as soon as we are its parent!
		np->SetParent(this, best+1);
		
		return npp;
	}
	return NULL;
}

void			WED_GISChain::Rotate			(GISLayer_t l,const Point2& ctr, double angle)
{
	int t = GetNumPoints();
	for (int n = 0; n <  t; ++n)
	{
		IGISPoint * p = GetNthPoint(n);
		p->Rotate(l,ctr, angle);
	}
}

int					WED_GISChain::GetNumPoints(void ) const
{
	RebuildCache(CacheBuild(cache_Topological));
	return mCachePts.size();
}

/*
void	WED_GISChain::DeletePoint(int n)
{
	WED_Thing * k = GetNthChild(n);
	k->SetParent(NULL, 0);
	k->Delete();
}
*/


IGISPoint *	WED_GISChain::GetNthPoint (int n) const
{
	RebuildCache(CacheBuild(cache_Topological));
	return mCachePts[n];
}


int			WED_GISChain::GetNumSides(void) const
{
	RebuildCache(CacheBuild(cache_Topological));
	int n = mCachePts.size();
	return (IsClosed()) ? n : (n-1);
}

bool		WED_GISChain::GetSide(GISLayer_t l,int n, Segment2& s, Bezier2& b) const
{
	RebuildCache(CacheBuild(cache_Topological));

	int n1 = n;
	int n2 = (n + 1) % mCachePts.size();

	IGISPoint * p1 = mCachePts[n1];
	IGISPoint * p2 = mCachePts[n2];
	IGISPoint_Bezier * c1 = mCachePtsBezier[n1];
	IGISPoint_Bezier * c2 = mCachePtsBezier[n2];

	p1->GetLocation(l, b.p1);
	p2->GetLocation(l, b.p2);
	b.c1 = b.p1;		// Mirror end-points to controls so that if we are a half-bezier,
	b.c2 = b.p2;		// we don't have junk in our bezier.

	// If we have a bezier point, fetch i.  Null out our ptrs to the bezier point
	// if the bezier handle doesn't exist -- this is a flag to us!
	if (c1) if (!c1->GetControlHandleHi(l, b.c1)) { c1 = NULL; b.c1 = b.p1; }
	if (c2) if (!c2->GetControlHandleLo(l, b.c2)) { c2 = NULL; b.c2 = b.p2; }

	// If we have neither end, we either had no bezier pt, or the bezier pt has no control handle.
	// Simpify down to a segment and return it -- some code may use this 'fast case'.
	if (!c1 && !c2)
	{
		s.p1 = b.p1;
		s.p2 = b.p2;
		return false;
	}
	return true;
}

void WED_GISChain::RebuildCache(int flags) const
{
	if(flags & cache_Topological)
	{
		mCachePts.clear();
		mCachePtsBezier.clear();
		int nc = CountChildren();
		mCachePts.reserve(nc);
		mCachePtsBezier.reserve(nc);
		mHasUV = nc > 0;
		
		int n;
		for (n = 0; n < nc; ++n)
		{
			WED_Thing * c = GetNthChild(n);
			IGISPoint *		   p = NULL;
			IGISPoint_Bezier * b = dynamic_cast<IGISPoint_Bezier *>(c);
			if (b) p = b; else p = dynamic_cast<IGISPoint *>(c);
			if (p)
			{
				mCachePts.push_back(p);
				mCachePtsBezier.push_back(b);
				if(mHasUV && !p->HasLayer(gis_UV))
					mHasUV = false;
			}
		}
	}

	if(flags & cache_Spatial)
	{
		int n = GetNumSides();			// We MUST ensure that this only builds topo cache or we are dead dead dead!!
		mCacheBounds = Bbox2();
		mCacheBoundsUV = Bbox2();
		
		for (int i = 0; i < n; ++i)
		{
			Segment2 s;
			Bezier2 b;
			if (GetSide(gis_Geo, i,s,b))
			{
				Bbox2	bb;
				b.bounds(bb);
				mCacheBounds += bb;
			} else {
				mCacheBounds += s.p1;
				mCacheBounds += s.p2;
			}

			if(mHasUV)
			{
				if (GetSide(gis_UV, i,s,b))
				{
					Bbox2	bb;
					b.bounds(bb);
					mCacheBoundsUV += bb;
				} else {
					mCacheBoundsUV += s.p1;
					mCacheBoundsUV += s.p2;
				}
			}
		}
	
	}
}

void WED_GISChain::Reverse(GISLayer_t l)
{
	RebuildCache(CacheBuild(cache_Topological));
	int n,t,np = GetNumPoints();
	
	// Sanity checking: our point count, and our cache really should
	// be in size sync.  Then make sure that we are CONSISTENT in our
	// having or not having beziers.  Heterogeneous _types_ of points
	// are not what we want!
	DebugAssert(mCachePtsBezier.size() == mCachePts.size());
	DebugAssert(mCachePtsBezier.size() == np);
	
	bool has_bezier = mCachePtsBezier[0] != NULL;
	for(n = 1; n < np; ++n)
	{
		Assert(has_bezier == (mCachePtsBezier[n] != NULL));
	}
	
	vector<Point2>	p(np);
	vector<Point2>	p_l(np);
	vector<Point2>	p_h(np);
	vector<int>		split(np);
	vector<int>		has_lo(np);
	vector<int>		has_hi(np);

	for(n = 0; n < np; ++n)
	{
		mCachePts[n]->GetLocation(l, p[n]);
		if(has_bezier)
		{
			has_lo[n] = mCachePtsBezier[n]->GetControlHandleLo(l, p_l[n]);
			has_hi[n] = mCachePtsBezier[n]->GetControlHandleHi(l, p_h[n]);
			split[n] = mCachePtsBezier[n]->IsSplit();
		}
	}

	for(n = 0; n < np; ++n)
	{
		t = np - n - 1;
		mCachePts[t]->SetLocation(l, p[n]);
		
		if(has_bezier)
		{
			mCachePtsBezier[t]->SetSplit(split[n]);

			if (has_lo[n])	mCachePtsBezier[t]->SetControlHandleHi(l, p_l[n]);
			else			mCachePtsBezier[t]->DeleteHandleHi();

			if (has_hi[n])	mCachePtsBezier[t]->SetControlHandleLo(l, p_h[n]);
			else			mCachePtsBezier[t]->DeleteHandleLo();
		}
	}	
}

void WED_GISChain::Shuffle(GISLayer_t l)
{
	RebuildCache(CacheBuild(cache_Topological));
	int n,t,np = GetNumPoints();
	
	DebugAssert(mCachePtsBezier.size() == mCachePts.size());
	DebugAssert(mCachePtsBezier.size() == np);
	
	bool has_bezier = mCachePtsBezier[0] != NULL;
	for(n = 1; n < np; ++n)
	{
		Assert(has_bezier == (mCachePtsBezier[n] != NULL));
	}
	
	vector<Point2>	p(np);
	vector<Point2>	p_l(np);
	vector<Point2>	p_h(np);
	vector<int>		split(np);
	vector<int>		has_lo(np);
	vector<int>		has_hi(np);

	for(n = 0; n < np; ++n)
	{
		mCachePts[n]->GetLocation(l, p[n]);
		if(has_bezier)
		{
			has_lo[n] = mCachePtsBezier[n]->GetControlHandleLo(l, p_l[n]);
			has_hi[n] = mCachePtsBezier[n]->GetControlHandleHi(l, p_h[n]);
			split[n] = mCachePtsBezier[n]->IsSplit();
		}
	}
	
	for(n = 0; n < np; ++n)
	{
		t = (n + 1) % np;
		mCachePts[t]->SetLocation(l, p[n]);
		if(has_bezier)
		{
			mCachePtsBezier[t]->SetSplit(split[n]);

			if (has_lo[n])	mCachePtsBezier[t]->SetControlHandleLo(l, p_l[n]);
			else			mCachePtsBezier[t]->DeleteHandleLo();

			if (has_hi[n])	mCachePtsBezier[t]->SetControlHandleHi(l, p_h[n]);
			else			mCachePtsBezier[t]->DeleteHandleHi();
		}	
	}
}



int				WED_GISChain::GetNumEntities(void ) const
{
	RebuildCache(CacheBuild(cache_Topological));
	return mCachePts.size();
}

IGISEntity *	WED_GISChain::GetNthEntity  (int n) const
{
	RebuildCache(CacheBuild(cache_Topological));
	return mCachePts[n];
}