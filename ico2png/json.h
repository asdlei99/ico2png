#pragma once

struct PngInfo
{
	PngInfo(string& i,string& w,string& h,string& d)
	{
		index = i;
		width = w;
		height = h;
		depth = d;
	}
	string index;
	string width;
	string height;
	string depth;
};

bool ParseJson(const char* jsrc,string* jobid,vector<PngInfo>* pngs);
