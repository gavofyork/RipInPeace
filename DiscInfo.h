#pragma once

#include <string>
#include <functional>
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
	DiscInfo(unsigned _n = 0): title(""), artist(""), year(1900), setIndex(0), setTotal(1), tracks(_n) { for (unsigned i = 0; i < _n; ++i) tracks[i] = TrackInfo(i + 1); }

	bool isCompilation() const
	{
		for (auto t: tracks)
			if (t.artist != tracks[0].artist)
				return true;
		return false;
	}

	std::string title;
	std::string artist;		///< "" = Various Artists; all tracks' artists must be defined.
	int year;				///< 1900 -> unknown.
	int setIndex;			///< 0 -> only disc, otherwise number in set.
	int setTotal;			///< 1 -> only disc, otherwise total in set.
	std::vector<TrackInfo> tracks;
	std::string discid;		///< MusicBrainz disc ID.
};

struct cddb_conn_s;
struct cddb_disc_s;

class DiscIdentity
{
public:
	explicit DiscIdentity(std::string const& _device = std::string());
	~DiscIdentity();

	std::string const& asString() const { return m_text; }

	bool isNull() const { return m_discId; }
	bool identify(std::string const& _device);
	std::vector<DiscInfo> lookup(int _forceTracks, std::function<bool()> const& _aborting) const;

private:
	void** m_discId;
	std::string m_text;
	cddb_disc_s* m_disc;
};
