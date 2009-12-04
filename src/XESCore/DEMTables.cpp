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
#include "DEMTables.h"
#include "EnumSystem.h"
#include "DEMDefs.h"
#include <ctype.h>
//#include "CoverageFinder.h"

EnumColorTable				gEnumColors;
ColorBandTable				gColorBands;
set<int>					gEnumDEMs;



NaturalTerrainTable			gNaturalTerrainTable;
//NaturalTerrainLandUseIndex	gNaturalTerrainLandUseIndex;
NaturalTerrainIndex			gNaturalTerrainIndex;
//TerrainPromoteTable			gTerrainPromoteTable;
//ManTerrainTable				gManTerrainTable;
BeachInfoTable				gBeachInfoTable;
BeachIndex					gBeachIndex;

LandUseTransTable			gLandUseTransTable;

TexProjTable					gTexProj;


static	void	ValidateNaturalTerrain(void);
static set<int>			sForests;

static set<int>			sAirports;

string	gNaturalTerrainFile;
string	gLanduseTransFile;
string	gReplacementClimate;
string 	gReplacementRoads;

#define R_VARY 0

inline double cosdeg(double deg)
{
	if (deg == 0.0) return 1.0;
	if (deg == 90.0) return 0.0;
	if (deg == 180.0) return -1.0;
	if (deg == -90.0) return 0.0;
	if (deg == -180.0) return -1.0;
	return cos(deg * DEG_TO_RAD);
}

static string	MakeLit(const string& inName)
{
	return inName.substr(0, inName.length()-4) + "_LIT.dds";
}

void MakeRVariant(string& io_string)
{
	io_string.insert(io_string.length()-4,"_R");
}


static	bool	LowerCheckName(string& ioName)
{
	string::size_type dir_char = ioName.find_last_of("\\/:");
	if (dir_char == ioName.npos) dir_char = 0;
	else						 dir_char++;
	for (string::size_type n = dir_char; n < ioName.length(); ++n)
		ioName[n] = tolower(ioName[n]);
	if ((ioName.size() - dir_char) > 27)
	{
		printf("The file name part of %s is too long - max limit for terrain names is 27 chars.\n", ioName.c_str());
		return false;
	}
	return true;
}

bool	ReadEnumColor(const vector<string>& tokens, void * ref)
{
	RGBColor_t	col;
	if (tokens.size() != 3 || !TokenizeColor(tokens[2], col))
	{
		printf("Parse error for enum colors.\n");
		return false;
	}

	int token;
	if (!TokenizeEnum(tokens[1],token, "Unknown token %s\n")) return false;
	if (gEnumColors.find(token) != gEnumColors.end())
		printf("WARNING: duplicate token %s\n", tokens[1].c_str());
	gEnumColors.insert(EnumColorTable::value_type(token, col));
	return true;
}

bool	ReadEnumBand(const vector<string>& tokens, void * ref)
{
	int				dem_key;
	float 			band_key;
	DEMColorBand_t	band;

	if (TokenizeLine(tokens, " effcc",
		&dem_key, &band.lo_value, &band.hi_value,
		&band.lo_color, &band.hi_color) != 6) return false;

	band_key = band.hi_value;

	ColorBandMap& table = gColorBands[dem_key];
	if (table.count(band_key) != 0)
		printf("WARNING: duplicate color band: %s/%f %f to %f\n",
			FetchTokenString(dem_key), band_key, band.lo_value, band.hi_value);
	table.insert(ColorBandMap::value_type(band_key, band));
	return true;

}

bool	ReadEnumDEM(const vector<string>& tokens, void * ref)
{
	int	dem;
	if (TokenizeLine(tokens, " e", &dem) != 2) return false;
	gEnumDEMs.insert(dem);
	return true;
}

bool	ReadBeachInfo(const vector<string>& tokens, void * ref)
{
	BeachInfo_t	info;

	if (TokenizeLine(tokens, " fffffffffffffifiii",
				&info.min_rain,
				&info.max_rain,
				&info.min_temp,
				&info.max_temp,
				&info.min_lat,
				&info.max_lat,
				&info.min_slope,
				&info.max_slope,
				&info.min_sea,
				&info.max_sea,
				&info.max_turn_convex,
				&info.max_turn_concave,
				&info.min_len,
				&info.require_open,
				&info.min_area,
				&info.require_airport,
				&info.x_beach_type,
				&info.x_backup) != 19) return false;

	info.max_turn_convex = cosdeg(info.max_turn_convex);
	info.max_turn_concave = cosdeg(info.max_turn_concave);

	info.min_slope=cosdeg(info.min_slope);
	info.max_slope=cosdeg(info.max_slope);			// NOTE: because we are storing cosigns, a flat beach is 1.0, so we are reversing the
	swap(info.min_slope,info.max_slope);			// ordering here.
	DebugAssert(gBeachIndex.count(info.x_beach_type) == 0);
	gBeachIndex[info.x_beach_type] = gBeachInfoTable.size();
	gBeachInfoTable.push_back(info);
	return true;
}

bool	ReadNaturalTerrainInfo(const vector<string>& tokens, void * ref)
{
	NaturalTerrainInfo_t	info;
	string					ter_name, tex_name, proj;

	int						has_lit = 0;;
	int						auto_vary;

	info.climate = NO_VALUE;
	info.urban_density_min = info.urban_density_max = 0.0;
	info.urban_radial_min = info.urban_radial_max = 0.0;
	info.urban_trans_min = info.urban_trans_max = 0.0;
	info.urban_square = 0;
	info.map_rgb[0] = info.map_rgb[1] = info.map_rgb[2] = 0.5;
	info.temp_rng_min = info.temp_rng_max = 0.0;
	info.custom_ter = tex_not_custom;

	if (tokens[0] == "STERRAIN")
	{
		if (TokenizeLine(tokens, " eeffffffffiffffffffffisifsfssefff",
			&info.terrain,
			&info.landuse,
			&info.elev_min,
			&info.elev_max,
			&info.slope_min,
			&info.slope_max,

			&info.temp_min,
			&info.temp_max,
			&info.rain_min,
			&info.rain_max,
			&info.near_water,

			&info.slope_heading_min,
			&info.slope_heading_max,
			&info.rel_elev_min,
			&info.rel_elev_max,
			&info.elev_range_min,
			&info.elev_range_max,
			&info.temp_rng_min,
			&info.temp_rng_max,

			&info.lat_min,
			&info.lat_max,
			&auto_vary,

			&ter_name,
			&info.layer,
			&info.xon_dist,
			&tex_name,
			&info.base_res,
			&proj,
			&info.border_tex,
			&info.forest_type,
			&info.map_rgb[0],
			&info.map_rgb[1],
			&info.map_rgb[2]
			) < 31) return false;
		info.xon_hack = true;
	} else 	if (tokens[0] == "MTERRAIN") {


		if (TokenizeLine(tokens, " eeeffffffffiffffffffffffiffisifsifssefff",
			&info.terrain,
			&info.landuse,
			&info.climate,
			&info.elev_min,
			&info.elev_max,
			&info.slope_min,
			&info.slope_max,

			&info.temp_min,
			&info.temp_max,
//			&info.temp_rng_min,
//			&info.temp_rng_max,
			&info.rain_min,
			&info.rain_max,
			&info.near_water,
			&info.slope_heading_min,
			&info.slope_heading_max,
			&info.rel_elev_min,
			&info.rel_elev_max,
			&info.elev_range_min,
			&info.elev_range_max,

			&info.urban_density_min,
			&info.urban_density_max,
			&info.urban_radial_min,
			&info.urban_radial_max,
			&info.urban_trans_min,
			&info.urban_trans_max,
			&info.urban_square,

			&info.lat_min,
			&info.lat_max,
			&auto_vary,

			&ter_name,
			&info.layer,
			&info.xon_dist,
			&tex_name,
			&has_lit,
	//		&info.comp_tex,
			&info.base_res,
	//		&info.comp_res,
	//		&info.base_alpha_invert,
	//		&info.comp_alpha_invert,
			&proj,
			&info.border_tex,
			&info.forest_type,
	//		&info.forest_ratio,
			&info.map_rgb[0],
			&info.map_rgb[1],
			&info.map_rgb[2]
			) != 41) return false;
		info.xon_hack = false;

	} else {

		if (TokenizeLine(tokens, " eeeffffffffffiffffffffffffiffisifsfssefff",
			&info.terrain,
			&info.landuse,
			&info.climate,
			&info.elev_min,
			&info.elev_max,
			&info.slope_min,
			&info.slope_max,

			&info.temp_min,
			&info.temp_max,
			&info.temp_rng_min,
			&info.temp_rng_max,
			&info.rain_min,
			&info.rain_max,
			&info.near_water,
			&info.slope_heading_min,
			&info.slope_heading_max,
			&info.rel_elev_min,
			&info.rel_elev_max,
			&info.elev_range_min,
			&info.elev_range_max,

			&info.urban_density_min,
			&info.urban_density_max,
			&info.urban_radial_min,
			&info.urban_radial_max,
			&info.urban_trans_min,
			&info.urban_trans_max,
			&info.urban_square,

			&info.lat_min,
			&info.lat_max,
			&auto_vary,

			&ter_name,
			&info.layer,
			&info.xon_dist,
			&tex_name,
	//		&info.comp_tex,
			&info.base_res,
	//		&info.comp_res,
	//		&info.base_alpha_invert,
	//		&info.comp_alpha_invert,
			&proj,
			&info.border_tex,
			&info.forest_type,
	//		&info.forest_ratio,
			&info.map_rgb[0],
			&info.map_rgb[1],
			&info.map_rgb[2]
			) != 42) return false;
		info.xon_hack = false;

	}
	if (info.elev_min > info.elev_max)	return false;
	if (info.slope_min > info.slope_max)	return false;
	if (info.temp_min > info.temp_max)	return false;
	if (info.temp_rng_min > info.temp_rng_max)	return false;
	if (info.rain_min > info.rain_max)	return false;
	if (info.slope_heading_min > info.slope_heading_max)	return false;
	if (info.rel_elev_min > info.rel_elev_max)	return false;
	if (info.elev_range_min > info.elev_range_max)	return false;
	if (info.urban_density_min > info.urban_density_max)	return false;
	if (info.urban_radial_min > info.urban_radial_max)	return false;
	if (info.urban_trans_min > info.urban_trans_max)	return false;
	if (info.urban_square < 0 || info.urban_square > 2)	return false;
	if (info.lat_min > info.lat_max)					return false;
	if (auto_vary != 0 && auto_vary != 1 && auto_vary != 2) 	return false;

	info.map_rgb[0] /= 255.0;
	info.map_rgb[1] /= 255.0;
	info.map_rgb[2] /= 255.0;

	if(info.forest_type != NO_VALUE)
		sForests.insert(info.forest_type);

//	int orig_forest = info.forest_type;
//	if (info.forest_type != NO_VALUE)	info.forest_type = LookupTokenCreate(ter_name.c_str());
//	if (info.forest_type != NO_VALUE)
//	{
//		if(sForests.count(info.forest_type) > 0)	if(sForests[info.forest_type] != orig_forest)
//			printf("WARNING: terrain %s has conflicting forest types %s and %s.\n", 
//								FetchTokenString(info.forest_type),
//								FetchTokenString(orig_forest),
//								FetchTokenString(sForests[info.forest_type]));
//		sForests[info.forest_type] = orig_forest;
//	}

						info.proj_angle = proj_Down;
	if (proj == "NS")	info.proj_angle = proj_NorthSouth;
	if (proj == "EW")	info.proj_angle = proj_EastWest;
	if (proj == "HDG")	auto_vary = 3;

	string::size_type nstart = tex_name.find_last_of("\\/:");
	if (nstart == tex_name.npos)	nstart = 0; else nstart++;
	if (tex_name.size()-nstart > 31)
		printf("WARNING: base tex %s too long.\n", tex_name.c_str());

	if (info.slope_min == info.slope_max &&	info.slope_min != 0.0)
		printf("WARNING: base tex %s has slope min and max both of %f\n", ter_name.c_str(), info.slope_min);

	if (info.proj_angle != proj_Down)
	if (info.slope_min < 30.0)
		printf("WARNING: base tex %s is projected but min slope is %f\n", ter_name.c_str(), info.slope_min);

	if (info.base_res == 0.0)
		printf("WARNING: bad base res on texture %s\n", ter_name.c_str());

	if (info.proj_angle == proj_NorthSouth)
	if (!(info.slope_heading_min == 0.0 && info.slope_heading_max == 0.0 ||
		info.slope_heading_min == 0.0 && info.slope_heading_max == 45.0 ||
		info.slope_heading_min == 135.0 && info.slope_heading_max == 180.0))
		printf("WARNING: base tex %s is projected north-south but has bad headings.\n",ter_name.c_str());

	if (info.proj_angle == proj_EastWest)
	if (!(info.slope_heading_min == 0.0 && info.slope_heading_max == 0.0 ||
		info.slope_heading_min == 45.0 && info.slope_heading_max == 135.0))
		printf("WARNING: base tex %s is projected east-west but has bad headings.\n",ter_name.c_str());

	// We use 1-cos notation, which keeps our order constant.
	info.slope_min = 1.0 - cosdeg(info.slope_min);
	info.slope_max = 1.0 - cosdeg(info.slope_max);

	// Slope heading - uses 1 for north and -1 for south internally.
	// So that's just cosign, but that does mean that Sergio's order (0-180)
	// is going to get reversed.
	info.slope_heading_min = cosdeg(info.slope_heading_min);
	info.slope_heading_max = cosdeg(info.slope_heading_max);
	swap(info.slope_heading_min, info.slope_heading_max);

	// Ben says: this used to be the non-auto-vary case!
	if (auto_vary < 3)
	{

		info.variant = 0;
		info.related = -1;

		string rep_name = ter_name;
		if(auto_vary > 0) rep_name += "_av";

		LowerCheckName(rep_name);
		info.name = LookupTokenCreate(rep_name.c_str());
		info.base_tex = tex_name;
		if(auto_vary == 2)
		{
			info.vary_tex = tex_name;
			info.vary_tex.insert(info.vary_tex.end()-4,'2');
		}
		if(auto_vary > 0)
			info.auto_vary = 1;
		else
			info.auto_vary = 0;

		if (has_lit)			info.lit_tex = MakeLit(info.base_tex);
		else					info.lit_tex.clear();

		#if R_VARY
		MakeRVariant(info.base_tex);
		if(!info.vary_tex.empty())
		MakeRVariant(info.vary_tex);
		#endif

		int rn = gNaturalTerrainTable.size();
		gNaturalTerrainTable.push_back(info);

//		gNaturalTerrainLandUseIndex.insert(NaturalTerrainLandUseIndex::value_type(info.landuse, rn));
		if (gNaturalTerrainIndex.count(info.name) == 0)
			gNaturalTerrainIndex[info.name] = rn;
	}


	info.vary_tex.clear();

	// AUTO-VARIATION - we take one rule and make four rules with variant codes.  Later the rule-finder will generate random codes to select rules spatially.
	// The auto-vary code is: 0 = none, 1 = vary by spatial blobs, 2 = vary by spatial blobs (2tex) 3 = vary by slope heading
	// The resulting codes in the struct are: 0 - no vary, 1-4 = spatial variants (all equal), 5-8 = heading variatns (N,E,S,W)
	if (auto_vary > 0)			// For ALL auto vary
//	if (auto_vary > 2)			// For only slope-based auto-vary
//	if (auto_vary > 3)			// For NO Variations
	{
		// -1 for related field means no relation.  Otherwise it is the index of the FIRST of four variants.
		// So...
		info.related = auto_vary == 3 ? -1 : gNaturalTerrainTable.size();
		info.auto_vary = 0;
		for (int rep = 1; rep <= 4; ++rep)
		{
			info.variant = rep + (auto_vary == 3 ? 4 : 0);
			info.map_rgb[2] += ((float) rep / 80.0);

			string rep_name = ter_name;
			rep_name += ('0' + rep);
			LowerCheckName(rep_name);
			info.name = LookupTokenCreate(rep_name.c_str());

			string tex_vari = tex_name;
			tex_vari.erase(tex_vari.size()-4);
			if (auto_vary == 2 && (rep == 2 || rep == 3))
				tex_vari += "2";
			tex_vari += ".dds";
			info.base_tex = tex_vari;

			if (has_lit)
				info.lit_tex = MakeLit(info.base_tex);
			else
				info.lit_tex.clear();

			#if R_VARY
			MakeRVariant(info.base_tex);
			#endif

			int rn = gNaturalTerrainTable.size();
			gNaturalTerrainTable.push_back(info);

//			gNaturalTerrainLandUseIndex.insert(NaturalTerrainLandUseIndex::value_type(info.landuse, rn));
			if (gNaturalTerrainIndex.count(info.name) == 0)
				gNaturalTerrainIndex[info.name] = rn;
		}
	}

	return true;
}

static bool HandleTranslate(const vector<string>& inTokenLine, void * inRef)
{
	int e1, e2;
	if (TokenizeLine(inTokenLine, " ee", &e1, &e2) == 3)
		gLandUseTransTable[e1] = e2;
	else
		return false;
	return true;
}


/*
bool	ReadPromoteTerrainInfo(const vector<string>& tokens, void * ref)
{
	pair<int, int> key;
	int			   value;
	if (TokenizeLine(tokens, " eee", &key.first, &key.second, &value) != 4)
		return false;

	if (gTerrainPromoteTable.count(key) != 0)
		printf("Warning: duplicate land use promotion key %s,%s\n",
			FetchTokenString(key.first),FetchTokenString(key.second));
	gTerrainPromoteTable[key] = value;
	return true;
}
*/

/*
bool	ReadManTerrainInfo(const vector<string>& tokens, void * ref)
{
	ManTerrainInfo_t	info;
	int					key;
	if (TokenizeLine(tokens, " esfsfss",
		&key, &info.terrain_name,
		&info.base_res, &info.base_tex,
		&info.comp_res, &info.comp_tex,
		&info.lit_tex) != 8) return false;
	if (gManTerrainTable.count(key) != 0)
		printf("Warning: duplicate man-made terrain info key %s\n",
			FetchTokenString(key));
	if (info.terrain_name == "-")	info.terrain_name.clear();
	if (!LowerCheckName(info.terrain_name)) return false;
	if (info.base_tex == "-")	info.base_tex.clear();
	if (info.comp_tex == "-")	info.comp_tex.clear();
	if (info.lit_tex  == "-")	info.lit_tex.clear();

	gManTerrainTable[key] = info;
	return true;
}*/


void	LoadDEMTables(void)
{
	gEnumColors.clear();
	gColorBands.clear();
	gEnumDEMs.clear();
	gNaturalTerrainTable.clear();
//	gNaturalTerrainLandUseIndex.clear();
	gNaturalTerrainIndex.clear();
//	gTerrainPromoteTable.clear();
	gBeachInfoTable.clear();
	gBeachIndex.clear();
	sForests.clear();
	gLandUseTransTable.clear();

	RegisterLineHandler("ENUM_COLOR", ReadEnumColor, NULL);
	RegisterLineHandler("COLOR_BAND", ReadEnumBand, NULL);
	RegisterLineHandler("COLOR_ENUM_DEM", ReadEnumDEM, NULL);
	RegisterLineHandler("BEACH", ReadBeachInfo, NULL);
	RegisterLineHandler("NTERRAIN", ReadNaturalTerrainInfo, NULL);
	RegisterLineHandler("STERRAIN", ReadNaturalTerrainInfo, NULL);
	RegisterLineHandler("MTERRAIN", ReadNaturalTerrainInfo, NULL);
//	RegisterLineHandler("PROMOTE_TERRAIN", ReadPromoteTerrainInfo, NULL);
	RegisterLineHandler("LU_TRANSLATE", HandleTranslate, NULL);

	if (gNaturalTerrainFile.empty())	LoadConfigFile("master_terrain.txt");
	else								LoadConfigFileFullPath(gNaturalTerrainFile.c_str());
	if (gLanduseTransFile.empty())		LoadConfigFile("landuse_translate.txt");
	else								LoadConfigFileFullPath(gLanduseTransFile.c_str());

	LoadConfigFile("enum_colors.txt");
	LoadConfigFile("beach_terrain.txt");

	ValidateNaturalTerrain();

	sAirports.clear();
	for(int n = 0; n < gNaturalTerrainTable.size(); ++n)
	if(gNaturalTerrainTable[n].terrain == terrain_Airport)
		sAirports.insert(gNaturalTerrainTable[n].name);

	/*
	printf("---forests---\n");
	for (set<int>::iterator f = sForests.begin(); f != sForests.end(); ++f)
	{
		printf("%s\n", FetchTokenString(*f));
	}*/

}

#pragma mark -

void CheckRuleUsage(void)
{
	bool	used_terrain = false;
	bool	used_landuse = false;
	bool	used_climate = false;
	bool	used_elev = false;
	bool	used_slope = false;
	bool	used_temp = false;
	bool	used_temp_rng = false;
	bool	used_rain = false;
	bool	used_near_water = false;
	bool	used_slope_heading = false;
	bool	used_rel_elev = false;
	bool	used_elev_range = false;
	bool	used_urban_density = false;
	bool	used_urban_radial = false;
	bool	used_urban_trans = false;
	bool	used_urban_square = false;
	bool	used_lat = false;
	bool	used_variant = false;

	for (int rec_num = 0; rec_num < gNaturalTerrainTable.size(); ++rec_num)
	{
		NaturalTerrainInfo_t& rec = gNaturalTerrainTable[rec_num];

		if (rec.temp_min != rec.temp_max)								used_temp=true;
		if (rec.slope_min != rec.slope_max)								used_slope=true;
		if (rec.rain_min != rec.rain_max)								used_rain=true;
		if (rec.temp_rng_min != rec.temp_rng_max)						used_temp_rng=true;
		if (rec.slope_heading_min != rec.slope_heading_max)				used_slope_heading=true;
		if (rec.variant != 0)											used_variant=true;
		if (rec.terrain != NO_VALUE)									used_terrain=true;
		if (rec.rel_elev_min != rec.rel_elev_max)						used_rel_elev=true;
		if (rec.elev_range_min != rec.elev_range_max)					used_elev_range=true;
		if (rec.urban_density_min != rec.urban_density_max)				used_urban_density=true;
		if (rec.urban_trans_min != rec.urban_trans_max)					used_urban_trans=true;
		if (rec.urban_square != 0)										
			used_urban_square=true;
		if (rec.lat_min != rec.lat_max)									used_lat=true;
		if (!rec.near_water)											used_near_water=true;
		if (rec.urban_radial_min != rec.urban_radial_max)				used_urban_radial=true;
		if (rec.landuse != NO_VALUE)									used_landuse=true;
		if (rec.climate != NO_VALUE)									used_climate=true;
		if (rec.elev_min != rec.elev_max)								used_elev=true;
	}
	
	if (!used_terrain)			printf("Terrain rule is not used.\n");
	if (!used_landuse)			printf("Landuse rule is not used.\n");
	if (!used_climate)			printf("Climate rule is not used.\n");
	if (!used_elev)				printf("Elev rule is not used.\n");
	if (!used_slope)			printf("Slope rule is not used.\n");
	if (!used_temp)				printf("Temp rule is not used.\n");
	if (!used_temp_rng)			printf("Temp range rule is not used.\n");
	if (!used_rain)				printf("Rain rule is not used.\n");
	if (!used_near_water)		printf("Near Water rule is not used.\n");
	if (!used_slope_heading)	printf("Slope Heading rule is not used.\n");
	if (!used_rel_elev)			printf("Rel Elev rule is not used.\n");
	if (!used_elev_range)		printf("Elev range rule is not used.\n");
	if (!used_urban_density)	printf("Urban density rule is not used.\n");
	if (!used_urban_radial)		printf("Urban radial rule is not used.\n");
	if (!used_urban_trans)		printf("Urban trans rule is not used.\n");
	if (!used_urban_square)		printf("Urban square rule is not used.\n");
	if (!used_lat)				printf("latitude rule is not used.\n");
	if (!used_variant)			printf("Variant rule is not used.\n");
}

int	FindNaturalTerrain(
				int		terrain,
				int 	landuse,
//				int 	climate,
//				float 	elevation,
				float 	slope,
				float 	slope_tri,
				float	temp,
				float	temp_rng,
				float	rain,
				int		water,
				float	slopeheading,
				float	relelevation,
				float	elevrange,
				float	urban_density,
				float	urban_radial,
				float	urban_trans,
				int		urban_square,
				float	lat,
				int		variant_blob,
				int		variant_head)
{
	// OPTIMIZE - figure out what the major keys should be.

//	pair<NaturalTerrainLandUseIndex::iterator,NaturalTerrainLandUseIndex::iterator>	range;
//	range = gNaturalTerrainLandUseIndex.equal_range(landuse);

	for (int rec_num = 0; rec_num < gNaturalTerrainTable.size(); ++rec_num)
	{
		NaturalTerrainInfo_t& rec = gNaturalTerrainTable[rec_num];

		float slope_to_use = rec.proj_angle == proj_Down ? slope : slope_tri;
//		float slope_to_use = slope_tri;

		if (rec.temp_min == rec.temp_max || temp == DEM_NO_DATA || (rec.temp_min <= temp && temp <= rec.temp_max))
		if (rec.slope_min == rec.slope_max || slope_to_use == DEM_NO_DATA || (rec.slope_min <= slope_to_use && slope_to_use <= rec.slope_max))
		if (rec.rain_min == rec.rain_max || rain == DEM_NO_DATA || (rec.rain_min <= rain && rain <= rec.rain_max))
		if (rec.temp_rng_min == rec.temp_rng_max || temp_rng == DEM_NO_DATA || (rec.temp_rng_min <= temp_rng && temp_rng <= rec.temp_rng_max))
		if (rec.slope_heading_min == rec.slope_heading_max || slopeheading == DEM_NO_DATA || (rec.slope_heading_min <= slopeheading && slopeheading <= rec.slope_heading_max))
		if (rec.variant == 0 || rec.variant == variant_blob || rec.variant == variant_head)
		// Ben says: v9 -- make sure that if the terrain is untagged that we don't use airport terrain -- that's an error!
		if (rec.terrain == NO_VALUE/* || terrain == NO_VALUE*/ || terrain == rec.terrain)
		if (rec.rel_elev_min == rec.rel_elev_max || relelevation == DEM_NO_DATA || (rec.rel_elev_min <= relelevation && relelevation <= rec.rel_elev_max))
		if (rec.elev_range_min == rec.elev_range_max || elevrange == DEM_NO_DATA || (rec.elev_range_min <= elevrange && elevrange <= rec.elev_range_max))
		if (rec.urban_density_min == rec.urban_density_max || urban_density == DEM_NO_DATA || (rec.urban_density_min <= urban_density && urban_density <= rec.urban_density_max))
		if (rec.urban_trans_min == rec.urban_trans_max || urban_trans == DEM_NO_DATA || (rec.urban_trans_min <= urban_trans && urban_trans <= rec.urban_trans_max))
		if (rec.urban_square == 0 || urban_square == DEM_NO_DATA || rec.urban_square == urban_square)
		if (rec.lat_min == rec.lat_max || lat == DEM_NO_DATA || (rec.lat_min <= lat && lat <= rec.lat_max))
		if (!rec.near_water || water)
		if (rec.urban_radial_min == rec.urban_radial_max || urban_radial == DEM_NO_DATA || (rec.urban_radial_min <= urban_radial && urban_radial <= rec.urban_radial_max))
		if (rec.landuse == NO_VALUE || landuse == rec.landuse)	// NOTE: no land use is NOT a free pass to match anything!!
//		if (rec.climate == NO_VALUE || climate == rec.climate)	// Same with  climate
//		if (rec.elev_min == rec.elev_max || elevation == DEM_NO_DATA || (rec.elev_min <= elevation && elevation <= rec.elev_max))
		{
			return rec.name;
		}
	}

	return -1;
}

#pragma mark -

void ValidateNaturalTerrain(void)
{
	map<int, int>	canonical;
	int				n, ref;
	for (n = 0; n < gNaturalTerrainTable.size(); ++n)
	{
		if (canonical.count(gNaturalTerrainTable[n].name) == 0)
			canonical[gNaturalTerrainTable[n].name] = n;
	}

	for (n = 0; n < gNaturalTerrainTable.size(); ++n)
	{
		ref = canonical[gNaturalTerrainTable[n].name];
		if (ref != n)
		{
			if (gNaturalTerrainTable[n].forest_type		  != gNaturalTerrainTable[ref].forest_type 		)	printf("ERROR: land use lines %d and %d - terrain 'forest type' does not match.  name = %s, layers = %s vs %s\n", ref, n, FetchTokenString(gNaturalTerrainTable[n].name), FetchTokenString(gNaturalTerrainTable[n].forest_type),FetchTokenString(gNaturalTerrainTable[ref].forest_type));
			if (gNaturalTerrainTable[n].layer    		  != gNaturalTerrainTable[ref].layer    		)	printf("ERROR: land use lines %d and %d - terrain 'layer' does not match.  name = %s, layers = %d vs %d\n", ref, n, FetchTokenString(gNaturalTerrainTable[n].name), gNaturalTerrainTable[n].layer,gNaturalTerrainTable[ref].layer);
			if (gNaturalTerrainTable[n].xon_dist 		  != gNaturalTerrainTable[ref].xon_dist 		)	printf("ERROR: land use lines %d and %d - terrain 'xon_dist' does not match.  name = %s, layers = %f vs %f\n", ref, n, FetchTokenString(gNaturalTerrainTable[n].name), gNaturalTerrainTable[n].xon_dist,gNaturalTerrainTable[ref].xon_dist);
			if (gNaturalTerrainTable[n].base_tex    	  != gNaturalTerrainTable[ref].base_tex    	 	)	printf("ERROR: land use lines %d and %d - terrain 'base_tex' does not match.  name = %s, layers = %s vs %s\n", ref, n, FetchTokenString(gNaturalTerrainTable[n].name), gNaturalTerrainTable[n].base_tex.c_str(),gNaturalTerrainTable[ref].base_tex.c_str());
			if (gNaturalTerrainTable[n].base_res		  != gNaturalTerrainTable[ref].base_res		 	)	printf("ERROR: land use lines %d and %d - terrain 'base_res' does not match.  name = %s, layers = %f vs %f\n", ref, n, FetchTokenString(gNaturalTerrainTable[n].name), gNaturalTerrainTable[n].base_res,gNaturalTerrainTable[ref].base_res);
		}
	}
}

struct float_between_iterator {
	typedef set<float>					set_type;
	typedef set_type::const_iterator	iter_type;
	iter_type		i_;
	bool			h_;
	const set_type&	s_;

	float_between_iterator(const set_type& s) : s_(s), i_(s.begin()), h_(false)
	{
	}

	bool operator()(void) const {
		return i_ != s_.end();
	}

	float_between_iterator& operator++(void) {
		if (h_) {
			h_ = false;
			++i_; }
		else {
			h_ = true;
			iter_type j = i_;
			++j;
			if (j == s_.end())
				++i_;
		}
		return *this;
	}

	float operator * (void) const {
		if (h_)
		{
			iter_type j = i_;
			++j;
			if (j == s_.end()) return *i_;
			return (*i_ + *j) * 0.5;
		} else
			return *i_;
		}

};



void	CheckDEMRuleCoverage(ProgressFunc func)
{
/*	CoverageFinder	cov(10);
	cov.NameAxis(0, "Terrain");
	cov.NameAxis(1, "Landuse");
	cov.NameAxis(2, "Climate");
	cov.NameAxis(3, "Elevation");
	cov.NameAxis(4, "Slope");
	cov.NameAxis(5, "Temp");
	cov.NameAxis(6, "NearWater");
	cov.NameAxis(7, "SlopeHeading");
	cov.NameAxis(8, "RelElev");
	cov.NameAxis(9, "ElevRange");
	cov.AddAxisEnum(0, terrain_Natural, "terrain_Natural");
	cov.AddAxisEnum(0, terrain_Airport, "terrain_Airport");
	cov.AddAxisEnum(1, lu_usgs_URBAN_SQUARE, "lu_usgs_URBAN_SQUARE");
	cov.AddAxisEnum(1, lu_usgs_URBAN_IRREGULAR, "lu_usgs_URBAN_IRREGULAR");
	cov.AddAxisEnum(2, climate_TropicalRainForest, "climate_TropicalRainForest");
	cov.AddAxisEnum(2, climate_TropicalMonsoon, "climate_TropicalMonsoon");
	cov.AddAxisEnum(2, climate_TropicalDry, "climate_TropicalDry");
	cov.AddAxisEnum(2, climate_DrySteppe, "climate_DrySteppe");
	cov.AddAxisEnum(2, climate_DryDesert, "climate_DryDesert");
	cov.AddAxisEnum(2, climate_TemperateAny, "climate_TemperateAny");
	cov.AddAxisEnum(2, climate_TemperateSummerDry, "climate_TemperateSummerDry");
	cov.AddAxisEnum(2, climate_TemperateWinterDry, "climate_TemperateWinterDry");
	cov.AddAxisEnum(2, climate_TemperateWet, "climate_TemperateWet");
	cov.AddAxisEnum(2, climate_ColdAny, "climate_ColdAny");
	cov.AddAxisEnum(2, climate_ColdSummerDry, "climate_ColdSummerDry");
	cov.AddAxisEnum(2, climate_ColdWinterDry, "climate_ColdWinterDry");
	cov.AddAxisEnum(2, climate_PolarTundra, "climate_PolarTundra");
	cov.AddAxisEnum(2, climate_PolarFrozen, "climate_PolarFrozen");
	cov.AddAxisRange(3, 0.0, 1000.0);
	cov.AddAxisRange(4, 0.0, 1.0);
	cov.AddAxisRange(5, 0.0, 10.0);
	cov.AddAxisEnum(6, 0, "No");
	cov.AddAxisEnum(6, 1, "Yes");
	cov.AddAxisRange(7, -1.0, 1.0);
	cov.AddAxisRange(8, 0, 1.0);
	cov.AddAxisRange(9, 0, 300);

	int i;
	for (i = 0; i < gNaturalTerrainTable.size(); ++i)
	{
		NaturalTerrainInfo_t& r = gNaturalTerrainTable[i];
		if (r.terrain != NO_VALUE)							cov.AddAxisEnum(0, r.terrain, FetchTokenString(r.terrain));
		if (r.landuse != NO_VALUE)							cov.AddAxisEnum(1, r.landuse, FetchTokenString(r.landuse));
		if (r.climate != NO_VALUE)							cov.AddAxisEnum(2, r.landuse, FetchTokenString(r.climate));
		if (r.elev_min != r.elev_max)						cov.AddAxisRange(3, r.elev_min, r.elev_max);
		if (r.slope_min != r.slope_max)						cov.AddAxisRange(4, r.slope_min, r.slope_max);
		if (r.temp_min != r.temp_max)						cov.AddAxisRange(5, r.temp_min, r.temp_max);
		if (r.slope_heading_min != r.slope_heading_max)		cov.AddAxisRange(7, r.slope_heading_min, r.slope_heading_max);
		if (r.rel_elev_min != r.rel_elev_max)				cov.AddAxisRange(8, r.rel_elev_min, r.rel_elev_max);
		if (r.elev_range_min != r.elev_range_max)			cov.AddAxisRange(9, r.elev_range_min, r.elev_range_max);
	}

	cov.FinishAxes();

	for (i = 0; i < gNaturalTerrainTable.size(); ++i)
	{
		bool	has = false;
		cov.StartRule();
		NaturalTerrainInfo_t& r = gNaturalTerrainTable[i];
		if (r.terrain != NO_VALUE)							{ cov.AddRuleEnum (0, r.terrain);											has = true; } else cov.AddRuleAny(0);
		if (r.landuse != NO_VALUE)							{ cov.AddRuleEnum (1, r.landuse);											has = true; } else cov.AddRuleAny(1);
		if (r.climate != NO_VALUE)							{ cov.AddRuleEnum (2, r.landuse);											has = true; } else cov.AddRuleAny(2);
		if (r.elev_min != r.elev_max)						{ cov.AddRuleRange(3, r.elev_min, r.elev_max);								has = true; } else cov.AddRuleAny(3);
		if (r.slope_min != r.slope_max)						{ cov.AddRuleRange(4, r.slope_min, r.slope_max);							has = true; } else cov.AddRuleAny(4);
		if (r.temp_min != r.temp_max)						{ cov.AddRuleRange(5, r.temp_min, r.temp_max);								has = true; } else cov.AddRuleAny(5);
		if (r.near_water != 0)								{ cov.AddRuleEnum (6, 1); 													has = true; } else cov.AddRuleAny(6);
		if (r.slope_heading_min != r.slope_heading_max)		{ cov.AddRuleRange(7, r.slope_heading_min, r.slope_heading_max);			has = true; } else cov.AddRuleAny(7);
		if (r.rel_elev_min != r.rel_elev_max)				{ cov.AddRuleRange(8, r.rel_elev_min, r.rel_elev_max);						has = true; } else cov.AddRuleAny(8);
		if (r.elev_range_min != r.elev_range_max)			{ cov.AddRuleRange(9, r.elev_range_min, r.elev_range_max);					has = true; } else cov.AddRuleAny(9);
		cov.EndRule(has);
	}

	cov.OutputGaps();
*/

	set<int>		terrain, landuse, urban_square, near_water;
	set<float>		/*elev, */slope, temp, temp_rng, rain, slope_head, rel_elev, elev_range, urban_density, urban_radial, urban_trans, lat;

	terrain.insert(NO_VALUE);
	landuse.insert(NO_VALUE);

	for (int n = 0; n < gNaturalTerrainTable.size(); ++n)
	{
		NaturalTerrainInfo_t& rec(gNaturalTerrainTable[n]);
		terrain.insert(rec.terrain);
		landuse.insert(rec.landuse);
//		elev.insert(rec.elev_min);
//		elev.insert(rec.elev_max);
		slope.insert(rec.slope_min);
		slope.insert(rec.slope_max);
		temp.insert(rec.temp_min);
		temp.insert(rec.temp_max);
		temp_rng.insert(rec.temp_rng_min);
		temp_rng.insert(rec.temp_rng_max);
		rain.insert(rec.rain_min);
		rain.insert(rec.rain_max);
		near_water.insert(rec.near_water);
		slope_head.insert(rec.slope_heading_min);
		slope_head.insert(rec.slope_heading_max);
		rel_elev.insert(rec.rel_elev_min);
		rel_elev.insert(rec.rel_elev_max);
		elev_range.insert(rec.elev_range_min);
		elev_range.insert(rec.elev_range_max);
		urban_density.insert(rec.urban_density_min);
		urban_density.insert(rec.urban_density_max);
		urban_radial.insert(rec.urban_radial_min);
		urban_radial.insert(rec.urban_radial_max);
		urban_trans.insert(rec.urban_trans_min);
		urban_trans.insert(rec.urban_trans_max);
		urban_square.insert(rec.urban_square);
		lat.insert(rec.lat_min);
		lat.insert(rec.lat_max);
	}

	int any_rule = gNaturalTerrainTable.back().name;

	printf("Landuse: %llu states.\n",(unsigned long long)landuse.size());
	printf("Terrain: %llu states.\n",(unsigned long long)terrain.size());
//	printf("Elev: %llu states.\n",(unsigned long long)elev.size() );
	printf("Slope: %llu states.\n",(unsigned long long)slope.size() );
	printf("Temp: %llu states.\n",(unsigned long long)temp.size() );
	printf("Temp Range: %llu states.\n",(unsigned long long)temp_rng.size() );
	printf("Rain: %llu states.\n",(unsigned long long)rain.size() );
	printf("Near Water: %llu states.\n",(unsigned long long)near_water.size());
	printf("Slope Headign: %llu states.\n",(unsigned long long)slope_head.size() );
	printf("Elevation Range: %llu states.\n",(unsigned long long)elev_range.size() );
	printf("Urban Density: %llu states.\n",(unsigned long long)urban_density.size() );
	printf("Urban Radial: %llu states.\n",(unsigned long long)urban_radial.size() );
	printf("Urban Trans: %llu states.\n",(unsigned long long)urban_trans.size() );
	printf("Urban Square: %llu states.\n",(unsigned long long)urban_square.size());
	printf("Latitude: %llu states.\n",(unsigned long long)lat.size());

	int total = landuse.size() *
				terrain.size() *
//				(elev.size() * 2 - 1) *
				(slope.size() * 2 - 1) *
				(temp.size() * 2 - 1) *
				(temp_rng.size() * 2 - 1)*
				(rain.size() * 2 - 1)*
				near_water.size() *
				(slope_head.size() * 2 - 1)*
				(elev_range.size() * 2 - 1)*
				(urban_density.size() * 2 - 1)*
				(urban_radial.size() * 2 - 1)*
				(urban_trans.size() * 2 - 1)*
				urban_square.size() *
				(lat.size() * 2 - 1);

	printf("Total pts to check: %d\n", total);
	int step = total / 200;

	PROGRESS_START(func, 0, 1, "Checking tables");

	int ctr = 0;

	for (set<int>::iterator lu = landuse.begin(); lu != landuse.end(); ++lu)
	for (set<int>::iterator ter = terrain.begin(); ter != terrain.end(); ++ter)
//	for (float_between_iterator el(elev); el(); ++el)
	for (float_between_iterator sd(slope); sd(); ++sd)
	for (float_between_iterator st(slope); st(); ++st)
	for (float_between_iterator t(temp); t(); ++t)
	for (float_between_iterator tr(temp_rng); tr(); ++tr)
	for (float_between_iterator r(rain); r(); ++r)
	for (set<int>::iterator nw = near_water.begin(); nw != near_water.end(); ++nw)
	for (float_between_iterator sh(slope_head); sh(); ++sh)
	for (float_between_iterator re(rel_elev); re(); ++re)
	for (float_between_iterator er(elev_range); er(); ++er)
	for (float_between_iterator ud(urban_density); ud(); ++ud)
	for (float_between_iterator ur(urban_radial); ur(); ++ur)
	for (float_between_iterator ut(urban_trans); ut(); ++ut)
	for (set<int>::iterator us = urban_square.begin(); us != urban_square.end(); ++us)
	for (float_between_iterator l(lat); l(); ++l)
	{
		PROGRESS_CHECK(func, 0, 1, "Checking tables", ctr, total, step);

		int found = FindNaturalTerrain(*ter, *lu, /*NO_VALUE,  *el, */ *sd, *st,
			*t, *tr, *r, *nw, *sh, *re, *er, *ud, *ur, *ut, *us, *l, 1, 5);

		if (found == any_rule)
		printf("Found %s rule on: ter=%s lu=%s el=XX sd=%f st=%f t=%f tr=%f r=%f w=%d sh=%f re=%f er=%f ud=XX ur=XX ut=%f us=%d l=%f\n",
			FetchTokenString(found),
			FetchTokenString(*ter), FetchTokenString(*lu), /* *el, */ *sd, *st,
			*t, *tr, *r, *nw, *sh, *re, *er, /* *ud, *ur, */ *ut, *us, *l);
		if (found == -1)
		printf("Found hole on: ter=%s lu=%s el=XX sd=%f st=%f t=%f tr=%f r=%f w=%d sh=%f re=%f er=%f ud=XX ur=XX ut=%f us=%d l=%f\n",
			FetchTokenString(*ter), FetchTokenString(*lu), /* *el, */ *sd, *st,
			*t, *tr, *r, *nw, *sh, *re, *er, /* *ud, *ur, */ *ut, *us, *l);

		++ctr;
	}
	PROGRESS_DONE(func, 0, 1, "Checking tables");
	printf("Total: %d\n", ctr);
}

void	GetNaturalTerrainColor(int terrain, float rgb[3])
{
	if (gNaturalTerrainIndex.count(terrain) == 0)
	{
		rgb[0] = rgb[2] = 1.0;
		rgb[1] = 0.0;
		return;
	}
	NaturalTerrainInfo_t& info = gNaturalTerrainTable[gNaturalTerrainIndex[terrain]];
	rgb[0] = info.map_rgb[0];
	rgb[1] = info.map_rgb[1];
	rgb[2] = info.map_rgb[2];
}

bool	IsForestType(int inType)
{
	return sForests.count(inType) != 0;
}

bool	IsAirportTerrain(int t)
{
	return sAirports.count(t) != 0;
}

void			GetForestTypes(set<int>& forests)
{
	forests = sForests;
}

//void	GetForestMapping(map<int,int>& forests)
//{
//	forests = sForests;
//}

void MakeDirectRules(void)
{
	vector<NaturalTerrainInfo_t>	nr;
	for(NaturalTerrainIndex::iterator all_names = gNaturalTerrainIndex.begin(); all_names != gNaturalTerrainIndex.end(); ++all_names)
	{
		NaturalTerrainInfo_t	rule(gNaturalTerrainTable[all_names->second]);
//		printf("Terrain %s comes from line %d (%s->%s)\n", 
//			FetchTokenString(all_names->first),
//			all_names->second,
//			FetchTokenString(gNaturalTerrainTable[all_names->second].terrain),
//			FetchTokenString(gNaturalTerrainTable[all_names->second].name));
		
		rule.terrain = all_names->first;
		rule.landuse = NO_VALUE;
		rule.climate = NO_VALUE;
		rule.elev_min = rule.elev_max = 0.0f;
		rule.slope_min = rule.slope_max = 0.0;
		rule.temp_min = rule.temp_max = 0.0;
		rule.temp_rng_min = rule.temp_rng_max = 0.0;
		rule.rain_min = rule.rain_max = 0.0;
		rule.near_water = 0;
		rule.slope_heading_min = rule.slope_heading_max = 0.0;
		rule.rel_elev_min = rule.rel_elev_max;
		rule.elev_range_min = rule.elev_range_max;

		rule.urban_density_min= 0.0;
		rule.urban_density_max= 0.0;
		rule.urban_radial_min= 0.0;
		rule.urban_radial_max= 0.0;
		rule.urban_trans_min= 0.0;
		rule.urban_trans_max= 0.0;
		rule.urban_square = 0;

		rule.lat_min=0.0;
		rule.lat_max=0.0;
		rule.variant=0;		// 0 = use all. 1-4 = flat variants. 5-8 = sloped variants, CW from N=5
//		printf("Adding rule: %s->%s\n", FetchTokenString(rule.terrain), FetchTokenString(rule.name));
		nr.push_back(rule);
	}
	
	int num_inserts = nr.size();
	gNaturalTerrainTable.insert(gNaturalTerrainTable.begin(), nr.begin(),nr.end());
	for(NaturalTerrainIndex::iterator all_names = gNaturalTerrainIndex.begin(); all_names != gNaturalTerrainIndex.end(); ++all_names)
		all_names->second += num_inserts;

}
