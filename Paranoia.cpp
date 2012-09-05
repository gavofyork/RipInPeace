#include <cassert>
#include <cdio/paranoia.h>
#include <cdio/cdda.h>
#include <cdio/cd_types.h>
#include "Paranoia.h"
using namespace std;

struct CannotFindCDROMDevice {};
struct UnableToOpenDisc {};
struct TroubleGettingStartingLSN {};
struct ParanoiaSentNoData {};

Paranoia::Paranoia(): m_cdrom(nullptr), m_paranoia(nullptr)
{
}

Paranoia::~Paranoia()
{
	close();
}

bool Paranoia::open(string const& _device)
{
	close();

	if (_device.empty())
		m_cdrom = cdio_cddap_find_a_cdrom(0, nullptr);
	else
		m_cdrom = cdio_cddap_identify(_device.c_str(), 0, nullptr);

	if (!m_cdrom)
		throw CannotFindCDROMDevice();

	cdio_cddap_verbose_set(m_cdrom, CDDA_MESSAGE_PRINTIT, CDDA_MESSAGE_PRINTIT);

	if (cdio_cddap_open(m_cdrom))
		throw UnableToOpenDisc();

	if (m_cdrom)
		m_paranoia = cdio_paranoia_init(m_cdrom);

	return m_cdrom && m_paranoia;
}

void Paranoia::close()
{
	if (m_paranoia)
	{
		cdio_paranoia_free(m_paranoia);
		m_paranoia = nullptr;
	}

	if (m_cdrom)
	{
		cdio_cddap_close(m_cdrom);
		m_cdrom = nullptr;
	}
}

unsigned Paranoia::tracks() const
{
	int ret = cdio_cddap_tracks(m_cdrom);
	if (ret == 255)
		return 0;
	return ret;
}

size_t Paranoia::frameLength()
{
	return CDIO_CD_FRAMESIZE_RAW / sizeof(int16_t) / 2;
}

size_t Paranoia::trackLength(unsigned _track) const
{
	if (cdio_get_track_format(m_cdrom->p_cdio, _track + 1) != TRACK_FORMAT_AUDIO)
		return 0;
	lsn_t begin = cdio_cddap_track_firstsector(m_cdrom, _track + 1);
	lsn_t end = cdio_cddap_track_lastsector(m_cdrom, _track + 1);
	return (end - begin + 1) * frameLength();
}

void Paranoia::rip(unsigned _track, function<bool(unsigned, unsigned, int16_t const*)> const& _f, int _flags)
{
	if (cdio_get_track_format(m_cdrom->p_cdio, _track + 1) != TRACK_FORMAT_AUDIO)
		return;

	assert(_track < tracks());
	lsn_t begin = cdio_cddap_track_firstsector(m_cdrom, _track + 1);
	lsn_t end = cdio_cddap_track_lastsector(m_cdrom, _track + 1);

	if (begin < 0)
		throw TroubleGettingStartingLSN();

	// TODO: allow change
	cdio_paranoia_modeset(m_paranoia, _flags == -1 ? PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP : _flags);
	cdio_paranoia_seek(m_paranoia, begin, SEEK_SET);
	for (lsn_t cursor = begin; cursor <= end; ++cursor)
	{
		// read a sector
		int16_t* buffer = cdio_paranoia_read_limited(m_paranoia, nullptr, 1);
		char* psz_err = cdio_cddap_errors(m_cdrom);
		char* psz_mes = cdio_cddap_messages(m_cdrom);
		if (psz_mes || psz_err)
		{
			printf("%s%s\n", psz_mes ? psz_mes: "", psz_err ? psz_err: "");
			if (psz_err)
				free(psz_err);
			if (psz_mes)
				free(psz_mes);
		}
		if (!buffer)
			throw ParanoiaSentNoData();
		if (!_f(cursor - begin, end - begin, buffer))
			break;
	}
}
