#include "contrib/cd_access.c"
#include "contrib/do_read.c"
#include <musicbrainz4/Query.h>
#include <musicbrainz4/Disc.h>
#include <musicbrainz4/Release.h>
#include <musicbrainz4/ArtistCredit.h>
#include <musicbrainz4/Artist.h>
#include <musicbrainz4/ArtistList.h>
#include <musicbrainz4/NameCreditList.h>
#include <musicbrainz4/NameCredit.h>
#include <musicbrainz4/MediumList.h>
#include <musicbrainz4/Medium.h>
#include <musicbrainz4/Track.h>
#include <musicbrainz4/Recording.h>
#include <discid/discid.h>
#include "DiscInfo.h"
using namespace std;
namespace mb4 = MusicBrainz4;

inline string toString(mb4::CNameCreditList* _a)
{
	string ret;
	for (int i = 0; i < _a->NumItems(); ++i)
		ret += _a->Item(i)->Artist()->Name() + _a->Item(i)->JoinPhrase();
	return ret;
}

DiscIdentity::DiscIdentity(std::string const& _device): m_discId(nullptr), m_disc(nullptr)
{
	if (!_device.empty())
		identify(_device);
}

DiscIdentity::~DiscIdentity()
{
	if (m_disc)
		cddb_disc_destroy(m_disc);
}

bool DiscIdentity::identify(string const& _device)
{
	if (m_disc)
		cddb_disc_destroy(m_disc);
	m_disc = cd_read((char*)_device.c_str());

	if (m_discId)
		discid_free(m_discId);
	m_discId = discid_new();
	if (discid_read(m_discId, _device.c_str()))
	{
		m_text = discid_get_id(m_discId);
		return true;
	}
	else
	{
		fprintf(stderr, "Error: %s\n", discid_get_error_msg(m_discId));
		discid_free(m_discId);
		m_discId = nullptr;
		return false;
	}
}

static bool isVA(string const& _a)
{
	return _a == "Various" || _a == "Various Artists" || _a == "various" || _a == "VA" || _a == "various artists" || _a == "Various artists";
}

std::vector<DiscInfo> DiscIdentity::lookup(int _forceTracks, std::function<bool()> const& _aborting) const
{
	std::vector<DiscInfo> ret;
	DiscInfo di;
	if (m_discId)
	{
		mb4::CQuery query("RipInPeace");
		try
		{
			mb4::CMetadata metadata = query.Query("discid", discid_get_id(m_discId));
			if (metadata.Disc() && metadata.Disc()->ReleaseList())
			{
				mb4::CReleaseList* releaseList = metadata.Disc()->ReleaseList();
				int nr = releaseList->NumItems();
				for (int count = 0; count < nr && !_aborting(); count++)
					if (mb4::CRelease* release = releaseList->Item(count))
					{
						mb4::CQuery::tParamMap params;
						params["inc"] = "artists labels recordings release-groups url-rels discids artist-credits";
						metadata = query.Query("release", release->ID(), "", params);
						if (metadata.Release())
						{
							mb4::CRelease* fullRelease = metadata.Release();
							cerr << (*fullRelease) << endl;
							di = DiscInfo(_forceTracks);
							di.discid = m_text;
							di.title = fullRelease->Title();
							di.artist = toString(fullRelease->ArtistCredit()->NameCreditList());
							if (isVA(di.artist))
								di.artist = "";
							di.setTotal = fullRelease->MediumList()->Count();
							di.year = atoi(fullRelease->Date().substr(0, 4).c_str());
							auto ml = fullRelease->MediaMatchingDiscID(discid_get_id(m_discId));
							for (int j = 0; j < ml.NumItems() && !_aborting(); ++j)
							{
								di.setIndex = ml.Item(j)->Position() - 1;
								for (int i = 0; i < ml.Item(j)->TrackList()->NumItems() && !_aborting(); ++i)
									if (ml.Item(j)->TrackList()->Item(i)->Position() <= (int)di.tracks.size())
									{
										di.tracks[ml.Item(j)->TrackList()->Item(i)->Position() - 1].title = ml.Item(j)->TrackList()->Item(i)->Recording()->Title();
										di.tracks[ml.Item(j)->TrackList()->Item(i)->Position() - 1].artist = toString(ml.Item(j)->TrackList()->Item(i)->Recording()->ArtistCredit()->NameCreditList());
									}
							}
							ret.push_back(di);
							break;
						}
					}
			}
		}
		catch (...)
		{
			cout << "Exception" << endl;
		}
	}

	if (m_disc)
	{
		cddb_conn_s* conn = cddb_new();
		int matches = cddb_query(conn, m_disc);
		for (int i = 0; i < matches && !_aborting(); ++i)
		{
			string category = cddb_disc_get_category_str(m_disc);
			auto discid = cddb_disc_get_discid(m_disc);
			auto ndisc = do_read(conn, category.c_str(), discid, 0);
			if (ndisc)
			{
				di = DiscInfo(_forceTracks);
				di.discid = m_text;
				di.title = cddb_disc_get_title(ndisc);
				di.artist = cddb_disc_get_artist(ndisc);
				if (isVA(di.artist))
					di.artist = "";
				di.year = cddb_disc_get_year(ndisc);
				for (auto track = cddb_disc_get_track_first(ndisc); !!track && !_aborting(); track = cddb_disc_get_track_next(ndisc))
					if (cddb_track_get_number(track) <= (int)di.tracks.size())
					{
						di.tracks[cddb_track_get_number(track) - 1].title = cddb_track_get_title(track);
						di.tracks[cddb_track_get_number(track) - 1].artist = cddb_track_get_artist(track);
					}
				ret.push_back(di);
				cddb_disc_destroy(ndisc);
			}
			cddb_query_next(conn, m_disc);
		}
		cddb_destroy(conn);
	}
	return ret;
}
