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

#include "WED_TaxiRoute.h"
#include "WED_EnumSystem.h"
#include "AptDefs.h"
#include "WED_ToolUtils.h"
#include "STLUtils.h"

#if AIRPORT_ROUTING

static void get_runway_parts(int rwy, set<int>& rwy_parts)
{
	if(rwy == atc_rwy_None)
		return;

	int rwy_int = ENUM_Export(rwy);
	int rwy_dir = rwy_int/10;
	int rwy_lane =rwy_int%10;
	if(rwy_dir < 37)
	{
		rwy_parts.insert(ENUM_Import(ATCRunwayOneway,rwy_int));
		return;
	}
	
	rwy_dir -= 36;
	int rwy_lane_recip = rwy_lane;
	if(rwy_lane_recip) rwy_lane_recip = 4 - rwy_lane_recip;
	rwy_parts.insert(ENUM_Import(ATCRunwayOneway,rwy_dir*10+rwy_lane));
	rwy_parts.insert(ENUM_Import(ATCRunwayOneway,rwy_dir*10+180+rwy_lane_recip));
	

}

DEFINE_PERSISTENT(WED_TaxiRoute)
TRIVIAL_COPY(WED_TaxiRoute, WED_GISEdge)

WED_TaxiRoute::WED_TaxiRoute(WED_Archive * a, int i) : WED_GISEdge(a,i),
	oneway(this,"One-Way",				SQL_Name("WED_taxiroute","oneway"),				XML_Name("taxi_route","oneway"),		1),
	runway(this,"Runway",				SQL_Name("WED_taxiroute","runway"),				XML_Name("taxi_route","runway"),		ATCRunwayTwoway, atc_rwy_None),
	hot_depart(this,"Departures",		SQL_Name("WED_taxiroute_depart","departures"),	XML_Name("departures","runway"),		ATCRunwayOneway,false),
	hot_arrive(this,"Arrivals",			SQL_Name("WED_taxiroute_arrive","arrivals"),	XML_Name("arrivals","runway"),			ATCRunwayOneway,false),
	hot_ils(this,"ILS Precision Area",	SQL_Name("WED_taxiroute_ils","ils"),			XML_Name("ils_holds","runway"),			ATCRunwayOneway,false)
{
}

WED_TaxiRoute::~WED_TaxiRoute()
{
}

void		WED_TaxiRoute::SetOneway(int d)
{
	oneway = d;
}

void		WED_TaxiRoute::SetRunway(int r)
{
	runway = r;
}

void		WED_TaxiRoute::SetHotDepart(const set<int>& rwys)
{
	hot_depart = rwys;
}

void		WED_TaxiRoute::SetHotArrive(const set<int>& rwys)
{
	hot_arrive = rwys;
}

void		WED_TaxiRoute::SetHotILS(const set<int>& rwys)
{
	hot_ils = rwys;
}

void	WED_TaxiRoute::Import(const AptRouteEdge_t& info, void (* print_func)(void *, const char *, ...), void * ref)
{
	SetName(info.name);
	oneway = info.oneway;
	if(info.runway)
	{
		int r = ENUM_LookupDesc(ATCRunwayTwoway, info.name.c_str());
		if (r == -1)
		{
			print_func(ref,"Runway name %s is illegal.\n", info.name.c_str());
			runway = atc_rwy_None;
		} else
			runway = r;
		
	}
	else
	{
		runway = atc_rwy_None;
	}

	for(set<string>::iterator h = info.hot_depart.begin(); h != info.hot_depart.end(); ++h)
	{
		int r = ENUM_LookupDesc(ATCRunwayOneway,h->c_str());
		if(r == -1)
			print_func(ref,"Runway name %s is illegal.\n", h->c_str());
		else
			hot_depart += r;
	}

	for(set<string>::iterator h = info.hot_arrive.begin(); h != info.hot_arrive.end(); ++h)
	{
		int r = ENUM_LookupDesc(ATCRunwayOneway,h->c_str());
		if(r == -1)
			print_func(ref,"Runway name %s is illegal.\n", h->c_str());
		else
			hot_arrive += r;
	}

	for(set<string>::iterator h = info.hot_ils.begin(); h != info.hot_ils.end(); ++h)
	{
		int r = ENUM_LookupDesc(ATCRunwayOneway,h->c_str());
		if(r == -1)
			print_func(ref,"Runway name %s is illegal.\n", h->c_str());
		else
			hot_ils += r;
	}

}

void	WED_TaxiRoute::Export(		 AptRouteEdge_t& info) const
{	
	if(runway.value == atc_rwy_None)
	{
		this->GetName(info.name);
		info.runway = 0;
	}
	else
	{
		info.runway = 1;
		info.name = ENUM_Desc(runway.value);
	}
	
	info.oneway = oneway.value;
	info.hot_depart.clear();
	info.hot_arrive.clear();
	info.hot_ils.clear();
	
	set<int>::iterator h;
	for (h = hot_depart.value.begin(); h != hot_depart.value.end(); ++h)
		info.hot_depart.insert(ENUM_Desc(*h));
	for (h = hot_arrive.value.begin(); h != hot_arrive.value.end(); ++h)
		info.hot_arrive.insert(ENUM_Desc(*h));
	for (h = hot_ils.value.begin(); h != hot_ils.value.end(); ++h)
		info.hot_ils.insert(ENUM_Desc(*h));


}

void	WED_TaxiRoute::GetNthPropertyDict(int n, PropertyDict_t& dict) const
{
	dict.clear();
	if(n == PropertyItemNumber(&runway))
	{
		const WED_Airport * airport = WED_GetParentAirport(this);
		if(airport)
		{
			PropertyDict_t full;
			WED_GISEdge::GetNthPropertyDict(n,full);			
			set<int> legal;
			WED_GetAllRunwaysTwoway(airport, legal);
			legal.insert(runway.value);
			legal.insert(atc_rwy_None);
			dict.clear();
			for(PropertyDict_t::iterator f = full.begin(); f != full.end(); ++f)
			if(legal.count(f->first))
				dict.insert(PropertyDict_t::value_type(f->first,f->second));
		}
	}
	else if (n == PropertyItemNumber(&hot_depart) ||
			 n == PropertyItemNumber(&hot_arrive) ||
			 n == PropertyItemNumber(&hot_ils))
	{
		const WED_Airport * airport = WED_GetParentAirport(this);
		if(airport)
		{
			set<int>	runway_parts;
			get_runway_parts(runway.value,runway_parts);
			
		
			PropertyDict_t full;
			WED_GISEdge::GetNthPropertyDict(n,full);			
			set<int> legal;
			WED_GetAllRunwaysOneway(airport, legal);
			PropertyVal_t val;
			this->GetNthProperty(n,val);
			DebugAssert(val.prop_kind == prop_EnumSet);
			copy(val.set_val.begin(),val.set_val.end(),set_inserter(legal));
			dict.clear();
			for(PropertyDict_t::iterator f = full.begin(); f != full.end(); ++f)
			if(legal.count(f->first))
				dict.insert(PropertyDict_t::value_type(f->first,make_pair(f->second.first,runway_parts.count(f->first) == 0)));
		}
	}
	else
		WED_GISEdge::GetNthPropertyDict(n,dict);			
}

void		WED_TaxiRoute::GetNthPropertyInfo(int n, PropertyInfo_t& info) const
{
	WED_GISEdge::GetNthPropertyInfo(n, info);
	if(runway.value != atc_rwy_None)
	if(n == PropertyItemNumber(&name))
	{
		info.can_edit = false;
	}
}

void		WED_TaxiRoute::GetNthProperty(int n, PropertyVal_t& val) const
{
	WED_GISEdge::GetNthProperty(n, val);
	if(runway.value != atc_rwy_None)
	{
		if(n == PropertyItemNumber(&name))
		{
			val.string_val = ENUM_Desc(runway.value);		
		}
		
		if(n == PropertyItemNumber(&hot_depart) ||
		n == PropertyItemNumber(&hot_arrive) ||
		n == PropertyItemNumber(&hot_ils))
		{
			set<int>	runway_parts;
			get_runway_parts(runway.value,runway_parts);
			copy(runway_parts.begin(),runway_parts.end(),set_inserter(val.set_val));
		}
	}
}

bool	WED_TaxiRoute::IsOneway(void) const
{
	return oneway.value;
}

bool	WED_TaxiRoute::IsRunway(void) const
{
	return runway.value != atc_rwy_None;
}

bool	WED_TaxiRoute::HasHotArrival(void) const
{
	// Ben says: since we auto-include ourselves in our flags if we are a runway, 
	// set our status to match.  
	return !hot_arrive.value.empty() || runway.value != atc_rwy_None;
}

bool	WED_TaxiRoute::HasHotDepart(void) const
{
	return !hot_depart.value.empty() || runway.value != atc_rwy_None;
}

bool	WED_TaxiRoute::HasHotILS(void) const
{
	return !hot_ils.value.empty() || runway.value != atc_rwy_None;
}

bool	WED_TaxiRoute::HasInvalidHotZones(const set<int>& legal_rwys) const
{
	set<int>::const_iterator z;

	for(z = hot_depart.value.begin(); z != hot_depart.value.end(); ++z)
	if(legal_rwys.count(*z) == 0)
		return true;

	for(z = hot_arrive.value.begin(); z != hot_arrive.value.end(); ++z)
	if(legal_rwys.count(*z) == 0)
		return true;

	for(z = hot_ils.value.begin(); z != hot_ils.value.end(); ++z)
	if(legal_rwys.count(*z) == 0)
		return true;
	
	return false;
}

int		WED_TaxiRoute::GetRunway(void) const
{
	return runway.value;
}


#endif
