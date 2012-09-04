#pragma once

#include <string>
#include <sstream>
#include <vector>

struct TrackInfo
{
	TrackInfo() {}
	TrackInfo(int _n): title("Track " + ((std::stringstream&)(std::stringstream() << _n)).str()) {}
	std::string title;
	std::string artist;		///< "" -> Disc's artist.
};

struct DiscInfo
{
	DiscInfo(unsigned _n = 0): title(""), artist(""), year(-1), setIndex(0), setTotal(1), tracks(_n) { for (unsigned i = 0; i < _n; ++i) tracks[i] = TrackInfo(i + 1); }

	bool isCompilation() const { return false; }// TODO: return !(foreach t: tracks, t.artist = tracks.first.artist)

	std::string title;
	std::string artist;		///< "" = Various Artists; all tracks' artists must be defined.
	int year;				///< -1 -> unknown.
	int setIndex;			///< 0 -> only disc, otherwise number in set.
	int setTotal;			///< 1 -> only disc, otherwise total in set.
	std::vector<TrackInfo> tracks;
};
