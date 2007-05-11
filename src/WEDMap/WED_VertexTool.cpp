#include "WED_VertexTool.h"
#include "IResolver.h"
#include "ISelection.h"
#include "IOperation.h"
#include "AssertUtils.h"
#include "WED_Persistent.h"
#include "WED_Runway.h"
#include "WED_MapZoomerNew.h"
#include "GISUtils.h"
#include "XESConstants.h"

const double kCornerBlend0[6] = { 0.5, 0.5, 0.25, 0.0, 0.0, -0.25 };
const double kCornerBlend1[6] = { 0.0, 0.5, 0.25, 0.0, 0.5,  0.75 };
const double kCornerBlend2[6] = { 0.0, 0.0, 0.25, 0.5, 0.5,  0.75 };
const double kCornerBlend3[6] = { 0.5, 0.0, 0.25, 0.5, 0.0, -0.25 };

const double kRunwayBlend0[4] = { 0.75,		0.0,	0.75,	0.0		};
const double kRunwayBlend1[4] = { 0.0,		0.25,	0.0,	0.25	};
const double kRunwayBlend2[4] = { 0.0,		0.75,	0.0,	0.75	};
const double kRunwayBlend3[4] = { 0.25,		0.0,	0.25,	0.0		};

const int kSourceIndex[5] = { 2, 2, 2, 2, 2 };
const int kTargetIndex[5] = { 0, 1, 3, 4, 5 };

START_CASTING(WED_VertexTool)
IMPLEMENTS_INTERFACE(IControlHandles)
IMPLEMENTS_INTERFACE(IPropertyObject)
BASE_CASE
END_CASTING												\


WED_VertexTool::WED_VertexTool(
				const char *			tool_name,
				GUI_Pane *				host,
				WED_MapZoomerNew *		zoomer,
				IResolver *				resolver,
				const char *			root_path,
				const char *			selection_path,
				int						sel_verts) :
		WED_HandleToolBase(tool_name, host, zoomer, resolver, root_path, selection_path),				
		mSelection(selection_path),
		mSelVerts(sel_verts)
{
	SetControlProvider(this);
}

WED_VertexTool::~WED_VertexTool()
{
}

void	WED_VertexTool::BeginEdit(void)
{
	IOperation * sel = SAFE_CAST(IOperation,GetResolver()->Resolver_Find(mSelection.c_str()));
	DebugAssert(sel != NULL);
	sel->StartOperation("Marquee Drag");
}

void	WED_VertexTool::EndEdit(void)
{
	IOperation * sel = SAFE_CAST(IOperation,GetResolver()->Resolver_Find(mSelection.c_str()));
	DebugAssert(sel != NULL);
	sel->CommitOperation();
}

int		WED_VertexTool::CountEntities(void) const
{
	vector<IGISEntity *> e;
	GetEntityInternal(e);
	return e.size();
}

int		WED_VertexTool::GetNthEntityID(int n) const
{
	vector<IGISEntity *> e;
	GetEntityInternal(e);
	return reinterpret_cast<int>(e[n]);
}

int		WED_VertexTool::CountControlHandles(int id						  ) const
{
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	
	if (rwy)
	{
		return 9;
	}
	else
	switch(en->GetGISClass()) {
	case gis_Point:
		if ((pt = SAFE_CAST(IGISPoint,en)) != NULL)
		{
			return 1;
		}
		break;
	case gis_Point_Bezier:
		if ((pt_b = SAFE_CAST(IGISPoint_Bezier,en)) != NULL)
		{
			return 3;
		}
		break;
	case gis_Point_Heading:
		if ((pt_h = SAFE_CAST(IGISPoint_Heading, en)) != NULL)
		{
			return 2;
		}
		break;
	case gis_Point_HeadingWidthLength:
		if ((pt_wl = SAFE_CAST(IGISPoint_WidthLength, en)) != NULL)
		{
			return 6;
		}
		break;
	case gis_Line:
		if ((ln = SAFE_CAST(IGISLine,en)) != NULL)
		{
			return 2;
		}
		break;
	case gis_Line_Width:
		if ((ln_w = SAFE_CAST(IGISLine_Width,en)) != NULL)
		{
			return 5;
		}
		break;
	}
	return 0;
}

void	WED_VertexTool::GetNthControlHandle(int id, int n,		 Point2& p) const
{
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	
	if (rwy && n > 4)
	{
		Point2 corners[4];
		switch(n) {
		case 5:	rwy->GetCornersBlas1(corners);	break;
		case 6:	rwy->GetCornersDisp1(corners);	break;
		case 7:	rwy->GetCornersDisp2(corners);	break;
		case 8:	rwy->GetCornersBlas2(corners);	break;
		}
		p.x =	corners[0].x * kRunwayBlend0[n-5] + corners[1].x * kRunwayBlend1[n-5] + corners[2].x * kRunwayBlend2[n-5] + corners[3].x * kRunwayBlend3[n-5];
		p.y =	corners[0].y * kRunwayBlend0[n-5] + corners[1].y * kRunwayBlend1[n-5] + corners[2].y * kRunwayBlend2[n-5] + corners[3].y * kRunwayBlend3[n-5];
		return;
	}
	
	else switch(en->GetGISClass()) {
	case gis_Point:
		if ((pt = SAFE_CAST(IGISPoint,en)) != NULL)
		{
			pt->GetLocation(p);
			return;
		}
		break;
	case gis_Point_Bezier:
		if ((pt_b = SAFE_CAST(IGISPoint_Bezier,en)) != NULL)
		{
			switch(n) {
			case 0:	pt_b->GetLocation(p);	break;
			case 1:	pt_b->GetControlHandleLo(p);	break;
			case 2: pt_b->GetControlHandleHi(p);	break;
			}
			return;
		}
		break;
	case gis_Point_Heading:
		if ((pt_h = SAFE_CAST(IGISPoint_Heading, en)) != NULL)
		{
			pt_h->GetLocation(p);
			Vector2	dir;
			NorthHeading2VectorMeters(p,p,pt_h->GetHeading(),dir);
			if(n==1)				
			{ 
				p = GetZoomer()->LLToPixel(p);
				p += dir * 15.0;
				p = GetZoomer()->PixelToLL(p);				
			}
			#if !DEV
				sync with structure?  mmm..not sure.
			#endif
			return;
		}
		break;
	case gis_Point_HeadingWidthLength:
		if ((pt_wl = SAFE_CAST(IGISPoint_WidthLength, en)) != NULL)
		{
			Point2 corners[4];
			pt_wl->GetCorners(corners);
			p.x =	corners[0].x * kCornerBlend0[n] + corners[1].x * kCornerBlend1[n] + corners[2].x * kCornerBlend2[n] + corners[3].x * kCornerBlend3[n];
			p.y =	corners[0].y * kCornerBlend0[n] + corners[1].y * kCornerBlend1[n] + corners[2].y * kCornerBlend2[n] + corners[3].y * kCornerBlend3[n];
			return;
		}
		break;
	case gis_Line:
		if ((ln = SAFE_CAST(IGISLine,en)) != NULL)
		{
			if (n == 0) ln->GetSource()->GetLocation(p);
			else		ln->GetTarget()->GetLocation(p);
			return;
		}
		break;
	case gis_Line_Width:
		if ((ln_w = SAFE_CAST(IGISLine_Width,en)) != NULL)
		{
			Point2 corners[4];
			ln_w->GetCorners(corners);
			p.x =	corners[0].x * kCornerBlend0[n] + corners[1].x * kCornerBlend1[n] + corners[2].x * kCornerBlend2[n] + corners[3].x * kCornerBlend3[n];
			p.y =	corners[0].y * kCornerBlend0[n] + corners[1].y * kCornerBlend1[n] + corners[2].y * kCornerBlend2[n] + corners[3].y * kCornerBlend3[n];
			return;
		}
		break;
	}
	DebugAssert(!"Cast failed!");
	return; 
}

int		WED_VertexTool::GetLinks		    (int id) const
{	
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	
	switch(en->GetGISClass()) {
	case gis_Point:
		if ((pt = SAFE_CAST(IGISPoint,en)) != NULL)
		{
			return 0;
		}
		break;
	case gis_Point_Bezier:
		if ((pt_b = SAFE_CAST(IGISPoint_Bezier,en)) != NULL)
		{
			return 2;
		}
		break;
	case gis_Point_Heading:
		if ((pt_h = SAFE_CAST(IGISPoint_Heading, en)) != NULL)
		{
			return 1;
		}
		break;
	case gis_Point_HeadingWidthLength:
		if ((pt_wl = SAFE_CAST(IGISPoint_WidthLength, en)) != NULL)
		{
			return 5;
		}
		break;
	case gis_Line:
		if ((ln = SAFE_CAST(IGISLine,en)) != NULL)
		{
			return 1;
		}
		break;
	case gis_Line_Width:
		if ((ln_w = SAFE_CAST(IGISLine_Width,en)) != NULL)
		{
			return 4;
		}
		break;
	}
	return 0;
}

int		WED_VertexTool::GetNthLinkSource   (int id, int n) const
{
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	
	switch(en->GetGISClass()) {
	case gis_Point_Bezier:		return n;
	case gis_Point_Heading:		return 0;
	case gis_Line:				return 0;
	case gis_Point_HeadingWidthLength:	
	case gis_Line_Width:		return kSourceIndex[n];
	}
	return 0;
}

int		WED_VertexTool::GetNthLinkSourceCtl(int id, int n) const
{
	return -1;
}

int		WED_VertexTool::GetNthLinkTarget   (int id, int n) const
{
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	
	switch(en->GetGISClass()) {
	case gis_Point_Bezier:		return n+1;
	case gis_Point_Heading:		return 1;
	case gis_Line:				return 1;
	case gis_Point_HeadingWidthLength:	
	case gis_Line_Width:		return kTargetIndex[n];
	}
	return 0;
}

int		WED_VertexTool::GetNthLinkTargetCtl(int id, int n) const
{
	return -1;
}

bool	WED_VertexTool::PointOnStructure(int id, const Point2& p) const
{
	return false;
}

void	WED_VertexTool::ControlsMoveBy(int id, const Vector2& delta)
{
}

void	WED_VertexTool::ControlsHandlesBy(int id, int n, const Vector2& delta)
{
	IGISEntity * en = reinterpret_cast<IGISEntity *>(id);
	WED_Runway * rwy = SAFE_CAST(WED_Runway, en);
	
	IGISPoint * pt;
	IGISPoint_Bezier * pt_b;
	IGISPoint_Heading * pt_h;
	IGISPoint_WidthLength * pt_wl;
	IGISLine * ln;
	IGISLine_Width * ln_w;	
	
	Point2	dummy;
	Point2	p;
	if (rwy && n > 4)
	{
		Point2	p1, p2;
		rwy->GetSource()->GetLocation(p1);
		rwy->GetTarget()->GetLocation(p2);
		Vector2	axis(p1,p2);
		Vector2 delta_m;
		axis = VectorLLToMeters(p1,axis);
		if (n == 5 || n == 7) axis = -axis;
		axis.normalize();
		delta_m = VectorLLToMeters(p1,delta);

		switch(n) {
		case 5:	rwy->SetBlas1(rwy->GetBlas1() + axis.dot(delta_m));	break;
		case 6:	rwy->SetDisp1(rwy->GetDisp1() + axis.dot(delta_m));	break;
		case 7:	rwy->SetDisp2(rwy->GetDisp2() + axis.dot(delta_m));	break;
		case 8:	rwy->SetBlas2(rwy->GetBlas2() + axis.dot(delta_m));	break;
		}
		return;
	}
	else switch(en->GetGISClass()) {
	case gis_Point:
		if ((pt = SAFE_CAST(IGISPoint,en)) != NULL)
		{
			pt->GetLocation(p);
			p += delta;
			pt->SetLocation(p);
			return;
		}
		break;
	case gis_Point_Bezier:
		if ((pt_b = SAFE_CAST(IGISPoint_Bezier,en)) != NULL)
		{
			switch(n) {
			case 0:	pt_b->GetLocation(p);	break;
			case 1:	pt_b->GetControlHandleLo(p);	break;
			case 2: pt_b->GetControlHandleHi(p);	break;
			}
			p += delta;
			switch(n) {
			case 0:	pt_b->SetLocation(p);	break;
			case 1:	pt_b->SetControlHandleLo(p);	break;
			case 2: pt_b->SetControlHandleHi(p);	break;
			}
			return;
		}
		break;
	case gis_Point_Heading:
		if ((pt_h = SAFE_CAST(IGISPoint_Heading, en)) != NULL)
		{	
			pt_h->GetLocation(p);
			if (n == 0)	
			{
				p += delta;
				pt_h->SetLocation(p);
			} else {
				Point2 me = p;
				Vector2	dir;
				NorthHeading2VectorMeters(p,p,pt_h->GetHeading(),dir);
				p = GetZoomer()->LLToPixel(p);
				p += dir * 15.0;
				p = GetZoomer()->PixelToLL(p);				
				p += delta;				
				dir = Vector2(me,p);
				dir.normalize();
				pt_h->SetHeading(VectorDegs2NorthHeading(me,me,dir));
				#if !DEV
					hrm - since we work RELATIVE to our last mouse positoin, if the user "slops" way out on the lnie, small moves STILL have a big impact.  Hrm.
				#endif
			}
			return;
		}
		break;
	case gis_Point_HeadingWidthLength:
		if ((pt_wl = SAFE_CAST(IGISPoint_WidthLength, en)) != NULL)
		{
			pt_wl->GetLocation(p);
			Point2 me = p;
			Vector2	dir;
			Vector2	delta_m;
			NorthHeading2VectorMeters(p,p,pt_wl->GetHeading(),dir);
			
			switch(n) {
			case 0:
			case 4:
				if (n==0) dir = -dir;
				dir.normalize();
				delta_m = VectorLLToMeters(me, delta);
				pt_wl->SetLength(pt_wl->GetLength() + 2.0 * dir.dot(delta_m));
				break;
			case 1:
			case 3:
				if (n==1)	dir = dir.perpendicular_ccw();
				else		dir = dir.perpendicular_cw();
				dir.normalize();
				delta_m = VectorLLToMeters(me, delta);
				pt_wl->SetWidth(pt_wl->GetWidth() + 2.0 * dir.dot(delta_m));
				break;			
			case 2:
				me += delta;
				pt_wl->SetLocation(me);
				break;
			case 5:
				{
					Point2 corners[4];
					pt_wl->GetCorners(corners);
					p.x =	corners[0].x * kCornerBlend0[5] + corners[1].x * kCornerBlend1[5] + corners[2].x * kCornerBlend2[5] + corners[3].x * kCornerBlend3[5];
					p.y =	corners[0].y * kCornerBlend0[5] + corners[1].y * kCornerBlend1[5] + corners[2].y * kCornerBlend2[5] + corners[3].y * kCornerBlend3[5];
					p += delta;
					dir = Vector2(me,p);
//					dir.normalize();
					pt_wl->SetHeading(VectorDegs2NorthHeading(me,me,dir));
				}
				break;
			}
			return;
		}
		break;
	case gis_Line:
		if ((ln = SAFE_CAST(IGISLine,en)) != NULL)
		{
			if (n == 0) ln->GetSource()->GetLocation(p);
			else		ln->GetTarget()->GetLocation(p);
			p += delta;
			if (n == 0) ln->GetSource()->SetLocation(p);
			else		ln->GetTarget()->SetLocation(p);
			return;
		}
		break;
	case gis_Line_Width:
		if ((ln_w = SAFE_CAST(IGISLine_Width,en)) != NULL)
		{
			Point2	p1, p2;
			ln_w->GetSource()->GetLocation(p1);
			ln_w->GetTarget()->GetLocation(p2);
			Vector2	axis(p1,p2);
			Vector2 delta_m;
			switch(n) {
			case 0: p1 += delta; ln_w->GetSource()->SetLocation(p1);													break;
			case 2: p1 += delta; ln_w->GetSource()->SetLocation(p1); p2 += delta; ln_w->GetTarget()->SetLocation(p2);	break;
			case 4:													 p2 += delta; ln_w->GetTarget()->SetLocation(p2);	break;
			
			case 1:
			case 3:
				axis = VectorLLToMeters(p1,axis);
				if (n==1)	axis = axis.perpendicular_ccw();
				else		axis = axis.perpendicular_cw();
				axis.normalize();
				delta_m = VectorLLToMeters(p1,delta);
				ln_w->SetWidth(ln_w->GetWidth() + 2.0 * axis.dot(delta_m));
				break;
			}
			return;
		}
		break;
	}
	DebugAssert(!"Cast failed!");
	return; 
}

void	WED_VertexTool::ControlsLinksBy	 (int id, int c, const Vector2& delta)
{
}

WED_HandleToolBase::EntityHandling_t	WED_VertexTool::TraverseEntity(IGISEntity * ent)
{
	switch(ent->GetGISClass()) {
	case gis_Composite:		return	ent_AtomicOrContainer;
	case gis_Polygon:		return	ent_Container;
	case gis_PointSequence: return	ent_Container;
	case gis_Ring:			return	ent_Container;
	case gis_Chain:			return	ent_Container;
	default:				return	ent_Atomic;
	}
}

void WED_VertexTool::GetEntityInternal(vector<IGISEntity *>& e) const
{
	ISelection * sel = SAFE_CAST(ISelection,GetResolver()->Resolver_Find(mSelection.c_str()));
	DebugAssert(sel != NULL);

	vector<IUnknown *>	iu;
	vector<IGISEntity *> en;
	
	e.clear();
	
	sel->GetSelectionVector(iu);
	if (iu.empty()) return;
	en.reserve(iu.size());
	for (vector<IUnknown *>::iterator i = iu.begin(); i != iu.end(); ++i)
	{
		IGISEntity * ent = SAFE_CAST(IGISEntity,*i);
		if (ent) AddEntityRecursive(ent, e);
	}
}

void		WED_VertexTool::AddEntityRecursive(IGISEntity * e, vector<IGISEntity *>& vec) const
{
	IGISPointSequence * ps;
	IGISPolygon * poly;
	IGISComposite * cmp;
	int c, n;
	
	switch(e->GetGISClass()) {
	case gis_Point:
	case gis_Point_Bezier:
	case gis_Point_Heading:
	case gis_Point_HeadingWidthLength:
	case gis_Line:
	case gis_Line_Width:
		vec.push_back(e);
		break;
	case gis_PointSequence:
	case gis_Ring:
	case gis_Chain:
		if ((ps = SAFE_CAST(IGISPointSequence, e)) != NULL)
		{
			c = ps->GetNumPoints();
			for (n = 0; n < c; ++n)
				AddEntityRecursive(ps->GetNthPoint(n),vec);
		}
		break;
	case gis_Polygon:
		if ((poly = SAFE_CAST(IGISPolygon, e)) != NULL)
		{
			AddEntityRecursive(poly->GetOuterRing(),vec);
			c = poly->GetNumHoles();
			for (n = 0; n < c; ++n)
				AddEntityRecursive(poly->GetNthHole(n),vec);				
		}
		break;
	case gis_Composite:
		if ((cmp = SAFE_CAST(IGISComposite, e)) != NULL)
		{
			c = cmp->GetNumEntities();
			for (n = 0; n < c; ++n)
				AddEntityRecursive(cmp->GetNthEntity(n), vec);
		}
		break;
	}
}