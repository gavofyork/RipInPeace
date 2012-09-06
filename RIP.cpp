#include "contrib/cd_access.c"
#include "contrib/do_read.c"

#include <fcntl.h>
#include <linux/cdrom.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <queue>
#include <thread>
#include <mutex>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <FLAC++/all.h>
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
#include <QApplication>
#include <QMessageBox>
#include <QWidget>
#include <QMenu>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QProcess>
#include <QScriptEngine>
#include <QDebug>
#include <QSettings>
#include "Settings.h"
#include "ui_Info.h"
#include "RIP.h"
using namespace std;
namespace mb4 = MusicBrainz4;

template <class _T> string toString(_T const& _t)
{
	ostringstream o;
	o << _t;
	return o.str();
}

inline QString fSS(string const& _s)
{
	return QString::fromUtf8(_s.c_str());
}

inline string tSS(QString const& _s)
{
	return string(_s.toUtf8().data());
}

/* TODO: cover art
 * TODO: repot freedb & mb4 into DiscInfo
 * TODO: optimize for tag insertion.
 */

static void paintLogo(QPainter& _p, QRect _r, int _degrees = 360, QColor _back = QColor::fromHsv(0, 128, 192), QColor _fore = QColor::fromHsv(0, 0, 232))
{
	int gs = 105;
	QRect gr = _r.adjusted(6, 6, -6, -6);
	QLinearGradient grad(gr.topLeft(), gr.bottomRight());
	_p.setRenderHint(QPainter::Antialiasing, true);
	_p.setPen(Qt::NoPen);
	grad.setStops(QGradientStops() << QGradientStop(0, _back) << QGradientStop(0.5, _back.lighter(gs)) << QGradientStop(0.51, _back.darker(gs)) << QGradientStop(1, _back));
	_p.setBrush(grad);
	_p.drawEllipse(_r);
	grad.setStops(QGradientStops() << QGradientStop(0, _fore) << QGradientStop(0.5, _fore.lighter(gs)) << QGradientStop(0.51, _fore.darker(gs)) << QGradientStop(1, _fore));
	_p.setBrush(grad);
	if (_degrees)
		_p.drawPie(_r, 90 * 16, -_degrees * 16);
	_p.setPen(QColor::fromHsv(0, 0, 0, 96));
	_p.setBrush(Qt::NoBrush);
	_p.drawEllipse(_r);
	_p.setCompositionMode(QPainter::CompositionMode_Source);
	_p.setPen(Qt::NoPen);
	_p.setBrush(Qt::transparent);
	_p.drawEllipse(_r.adjusted(8, 8, -8, -8));
}

RIP::RIP(): m_path("/media/Data/Music"), m_filename("discartist+' - '+disctitle+(total>1 ? ' ['+index+'-'+total+']' : '')+'/'+sortnumber+' '+(compilation ? artist+' - ' : '')+title+'.flac'"), m_device("/dev/cdrom2"), m_discId(nullptr), m_ripper(nullptr), m_ripped(false)
{
	QApplication::setQuitOnLastWindowClosed(false);
	m_logo = QImage(":/rip.png");
	{
		QPixmap px(22, 22);
		px.fill(Qt::transparent);
		{
			QPainter p(&px);
			paintLogo(p, px.rect().adjusted(1, 1, -1, -1));
		}
		m_normal = QIcon(px);
	}
	{
		QPixmap px(22, 22);
		px.fill(Qt::transparent);
		{
			QPainter p(&px);
			p.setOpacity(0.95);
			paintLogo(p, px.rect().adjusted(1, 1, -1, -1));
		}
		m_inactive = QIcon(px);
	}

	setIcon(m_inactive);

	readSettings();

	m_settings = new Settings(this);
	m_popup = new QWidget(0, Qt::FramelessWindowHint);
	m_info.setupUi(m_popup);
	m_popup->setEnabled(false);
	connect(m_info.presets, SIGNAL(currentIndexChanged(int)), SLOT(updatePreset(int)));

	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
	setContextMenu(new QMenu("Rip In Peace"));
	(m_abortRip = contextMenu()->addAction("Abort Rip", this, SLOT(onAbortRip())))->setEnabled(false);
#if DEBUG
	(m_testIcon = contextMenu()->addAction("Test Icon"))->setCheckable(true);
#endif
	contextMenu()->addSeparator();
	contextMenu()->addAction("Settings", m_settings, SLOT(show()));
	contextMenu()->addAction("About", this, SLOT(onAbout()));
	contextMenu()->addAction("Quit", this, SLOT(onQuit()));

	m_conn = cddb_new();
	m_disc = nullptr;

	startTimer(1000);
}

RIP::~RIP()
{
	writeSettings();
	m_aborting = true;
	if (m_ripper)
	{
		m_ripper->join();
		delete m_ripper;
		m_ripper = nullptr;
	}
	if (m_identifier)
	{
		m_identifier->join();
		delete m_identifier;
		m_identifier = nullptr;
	}
	if (m_disc)
		cddb_disc_destroy(m_disc);
	if (m_conn)
		cddb_destroy(m_conn);
}

void RIP::readSettings()
{
	QSettings s("Gav", "RIP");
	m_filename = tSS(s.value("filename", fSS(m_filename)).toString());
	m_path = tSS(s.value("directory", fSS(m_path)).toString());
	m_device = tSS(s.value("device", fSS(m_device)).toString());
	m_paranoia = s.value("paranoia", m_paranoia).toInt();
}

void RIP::writeSettings()
{
	QSettings s("Gav", "RIP");
	s.setValue("filename", fSS(m_filename));
	s.setValue("directory", fSS(m_path));
	s.setValue("device", fSS(m_device));
	s.setValue("paranoia", m_paranoia);
}

void RIP::updatePreset(int _i)
{
	m_di = (int)m_dis.size() > _i && _i >= 0 ? m_dis[_i] : DiscInfo();
	m_info.title->setText(fSS(m_di.title));
	m_info.artist->setText(fSS(m_di.artist));
	m_info.setIndex->setValue(m_di.setIndex + 1);
	m_info.setTotal->setValue(m_di.setTotal);
	m_info.year->setValue(m_di.year);
	m_info.tracks->setRowCount(m_p.tracks());
	for (unsigned i = 0; i < m_di.tracks.size(); ++i)
	{
		m_info.tracks->setItem(i, 0, new QTableWidgetItem(fSS(m_di.tracks[i].title)));
		m_info.tracks->setItem(i, 1, new QTableWidgetItem(fSS(m_di.tracks[i].artist)));
	}
}

void RIP::onAbout()
{
	QMessageBox::about(nullptr, "About Rip in Peace", "<b>Rip in Peace:</b><p>A Ripper that doesn't get in your way.<p>By Gav Wood, 2012.");
}

void RIP::onAbortRip()
{
	m_aborting = true;
}

void RIP::onQuit()
{
	qApp->quit();
}

void RIP::onActivated(QSystemTrayIcon::ActivationReason _r)
{
	if (_r == QSystemTrayIcon::Trigger)
	{
		m_popup->setVisible(!m_popup->isVisible());
		if (m_popup->isVisible())
			m_popup->move(QCursor::pos());
		m_poppedUp = true;
		--m_lastPercentDone;	// Trick the system into recalculating the icon.
	}
}

void RIP::tagAll()
{
	for (unsigned i = 0; i < m_di.tracks.size(); ++i)
		if (m_p.trackLength(i))
		{
			string filename = m_path + "/" + m_temp + "/" + toString(i);

			FLAC::Metadata::Chain chain;
			if (!chain.read(filename.c_str()))
			{
				cerr << "ERROR: Couldn't read FLAC chain.";
				continue;
			}

			FLAC::Metadata::VorbisComment* vc = nullptr;
			{
				FLAC::Metadata::Iterator iterator;
				iterator.init(chain);
				do {
					FLAC::Metadata::Prototype* block = iterator.get_block();
					if (block->get_type() == FLAC__METADATA_TYPE_VORBIS_COMMENT)
					{
						vc = (FLAC::Metadata::VorbisComment*)block;
						break;
					}
				} while (iterator.next());
				if (!vc)
				{
					while (iterator.next()) {}
					vc = new FLAC::Metadata::VorbisComment();
					vc->set_is_last(true);
					if (!iterator.insert_block_after(vc))
					{
						delete vc;
						cerr << "ERROR: Couldn't insert vorbis comment in chain.";
						continue;
					}
				}
				while (vc->get_num_comments() > 0)
					vc->delete_comment(vc->get_num_comments() - 1);
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("album", m_di.title.c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("albumartist", m_di.artist.c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("discid", m_di.discid.c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("date", toString(m_di.year).c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("tracknumber", toString(i + 1).c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("title", m_di.tracks[i].title.c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("artist", m_di.tracks[i].artist.c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("discnumber", toString(m_di.setIndex + 1).c_str()));
				vc->append_comment(FLAC::Metadata::VorbisComment::Entry("disctotal", toString(m_di.setTotal).c_str()));
			}

			if (!chain.write())
			{
				cerr << "ERROR: Couldn't write FLAC chain.";
				continue;
			}

			delete vc;
		}
}

QString scrubbed(QString _s)
{
	_s.replace('*', "");
	_s.replace(':', "");
	_s.replace('?', "");
	_s.replace('/', "");
	_s.replace('\\', "");
	return _s;
}

void RIP::moveAll()
{
	QScriptEngine s;
	s.globalObject().setProperty("disctitle", scrubbed(fSS(m_di.title)), QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("discartist", scrubbed(fSS(m_di.artist)), QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("index", m_di.setIndex + 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("total", m_di.setTotal, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("year", m_di.year, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("compilation", m_di.isCompilation(), QScriptValue::ReadOnly|QScriptValue::Undeletable);
	for (unsigned i = 0; i < m_di.tracks.size(); ++i)
		if (m_p.trackLength(i))
		{
			s.globalObject().setProperty("number", i + 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
			s.globalObject().setProperty("sortnumber", QString("%1").arg(i + 1, 2, 10, QChar('0')), QScriptValue::ReadOnly|QScriptValue::Undeletable);
			s.globalObject().setProperty("title", scrubbed(fSS(m_di.tracks[i].title)), QScriptValue::ReadOnly|QScriptValue::Undeletable);
			s.globalObject().setProperty("artist", scrubbed(fSS(m_di.tracks[i].artist)), QScriptValue::ReadOnly|QScriptValue::Undeletable);
			auto filename = s.evaluate(fSS(m_filename)).toString();
			QDir().mkpath((fSS(m_path) + "/" + filename).section('/', 0, -2));
			QFile::rename(fSS(m_path + "/" + m_temp) + QString("/%1").arg(i), fSS(m_path) + "/" + filename);
		}
	QDir().rmdir(fSS(m_path + "/" + m_temp));
}

void RIP::timerEvent(QTimerEvent*)
{
	if (m_ripped)
	{
		m_ripper->join();
		m_identifier->join();
		delete m_ripper;
		m_ripper = nullptr;
		delete m_identifier;
		m_identifier = nullptr;
		m_ripped = false;
		if (!m_aborting)
		{
			takeDiscInfo();
			if (m_di.title.empty())
			{
				tagAll();
				moveAll();
			}
			else
			{
				showMessage("Unknown CD", "Couldn't find the CD with the available resources. Please reinsert once a database entry is accessible and the rip will be finished.");
			}
		}
		m_p.close();
		m_progress.clear();
		eject();
		m_dis.clear();
		updatePreset(-1);
		m_popup->setEnabled(false);
		m_aborting = false;
		m_abortRip->setEnabled(false);
	}
	if (!m_ripper)
	{
		if (m_p.open(m_device) && m_p.tracks() > 0)
		{
			// Begin ripping.
			m_lastPercentDone = 100;
			m_progress.clear();
			for (unsigned i = 0; i < m_p.tracks(); ++i)
				m_progress.push_back(make_pair(0, m_p.trackLength(i)));

			if (m_disc)
				cddb_disc_destroy(m_disc);
			m_disc = cd_read((char*)m_device.c_str());

			if (m_discId)
				discid_free(m_discId);
			m_discId = discid_new();
			if (discid_read(m_discId, m_device.c_str()))
			{
				m_temp = string("RIP-") + discid_get_id(m_discId);
				QDir().mkpath(fSS(m_path + "/" + m_temp));
				m_poppedUp = m_popup->isVisible();
				m_aborting = false;
				m_identified = false;
				m_ripper = new std::thread([&](){ rip(); m_ripped = true; });
				m_abortRip->setEnabled(true);
				m_identifier = new std::thread([&](){ getDiscInfo(); m_identified = true; });
			}
			else
			{
				fprintf(stderr, "Error: %s\n", discid_get_error_msg(m_discId));
				discid_free(m_discId);
				m_discId = nullptr;
			}
		}
		else
		{
			m_p.close();
			setIcon(m_inactive);
		}
	}
	if (m_identified && !m_popup->isEnabled() && m_dis.size())
	{
		m_info.presets->clear();
		for (DiscInfo const& di: m_dis)
			m_info.presets->addItem(fSS(di.artist + " - " + di.title));

		updatePreset(0);
		m_popup->setEnabled(true);
	}
	QString tt;
	if (m_progress.size())
		for (unsigned i = 0; i < m_progress.size(); ++i)
			if (m_progress[i].first != 0 && m_progress[i].first != m_progress[i].second)
				tt += QString("%1: '%5' %4%\n").arg(int(i + 1)).arg(int(m_progress[i].first * 100.0 / m_progress[i].second)).arg(m_di.tracks[i].title.c_str());
			else{}
	else
		tt = "Ready\n";
	tt.chop(1);
	setToolTip(tt);

	size_t total = 0;
	size_t done = 0;
	for (auto p: m_progress)
	{
		done += p.first;
		total += p.second;
	}
	int percentDone = total ? int(done * 100.0 / total) : 0;
#if DEBUG
	if (m_testIcon->isChecked())
	{
		total = 1;
		percentDone = m_lastPercentDone == 100 ? 0 : (m_lastPercentDone + 5);
	}
#endif
	if (total && m_lastPercentDone != percentDone)
	{
		QPixmap px(22, 22);
		px.fill(Qt::transparent);
		{
			QPainter p(&px);
			paintLogo(p, px.rect().adjusted(1, 1, -1, -1), percentDone * 360 / 100);
			if (!m_poppedUp)
			{
				p.setPen(QColor(64, 64, 64));
				p.drawText(px.rect().translated(1, 0), Qt::AlignCenter, "!");
				p.drawText(px.rect().translated(-1, 0), Qt::AlignCenter, "!");
				p.drawText(px.rect().translated(0, 1), Qt::AlignCenter, "!");
				p.drawText(px.rect().translated(0, -1), Qt::AlignCenter, "!");
				p.setPen(QColor(192, 192, 192));
				p.drawText(px.rect(), Qt::AlignCenter, "!");
			}
		}
		setIcon(QIcon(px));
		if (percentDone >= 90 && m_lastPercentDone < 90 && !m_poppedUp)
			showMessage("Ripping nearly finished", "Ripping is almost complete; tagging will begin shortly. Are you sure the tags are OK?");
		m_lastPercentDone = percentDone;
	}
}

void RIP::eject()
{
	QProcess::execute("eject", QStringList() << fSS(m_device));
}

inline string toString(mb4::CNameCreditList* _a)
{
	string ret;
	for (int i = 0; i < _a->NumItems(); ++i)
		ret += _a->Item(i)->Artist()->Name() + _a->Item(i)->JoinPhrase();
	return ret;
}

void RIP::getDiscInfo()
{
	m_dis.clear();
	m_di = DiscInfo(m_p.tracks());
	string id;
	if (m_discId)
	{
		id = discid_get_id(m_discId);
		mb4::CQuery query("RipInPeace");
		try
		{
			mb4::CMetadata metadata = query.Query("discid", discid_get_id(m_discId));
			if (metadata.Disc() && metadata.Disc()->ReleaseList())
			{
				mb4::CReleaseList* releaseList = metadata.Disc()->ReleaseList();
				for (int count = 0; count < releaseList->NumItems() && !m_aborting; count++)
					if (mb4::CRelease* release = releaseList->Item(count))
					{
						mb4::CQuery::tParamMap params;
						params["inc"] = "artists labels recordings release-groups url-rels discids artist-credits";
						metadata = query.Query("release", release->ID(), "", params);
						if (metadata.Release())
						{
							mb4::CRelease* fullRelease = metadata.Release();
							cerr << (*fullRelease) << endl;
							m_di = DiscInfo();
							m_di.discid = id;
							m_di.title = fullRelease->Title();
							m_di.artist = toString(fullRelease->ArtistCredit()->NameCreditList());
							m_di.setTotal = fullRelease->MediumList()->Count();
							m_di.year = atoi(fullRelease->Date().substr(0, 4).c_str());
							auto ml = fullRelease->MediaMatchingDiscID(discid_get_id(m_discId));
							for (int j = 0; j < ml.NumItems() && !m_aborting; ++j)
							{
								m_di.setIndex = ml.Item(j)->Position() - 1;
								for (int i = 0; i < ml.Item(j)->TrackList()->NumItems() && !m_aborting; ++i)
									if (ml.Item(j)->TrackList()->Item(i)->Position() <= (int)m_p.tracks())
									{
										m_di.tracks[ml.Item(j)->TrackList()->Item(i)->Position() - 1].title = ml.Item(j)->TrackList()->Item(i)->Recording()->Title();
										m_di.tracks[ml.Item(j)->TrackList()->Item(i)->Position() - 1].artist = toString(ml.Item(j)->TrackList()->Item(i)->Recording()->ArtistCredit()->NameCreditList());
									}
							}
							m_dis.push_back(m_di);
						}
					}
			}
		}
		catch (...)
		{
			cout << "Exception" << endl;
		}
	}
	{
		int matches = cddb_query(m_conn, m_disc);
		for (int i = 0; i < matches && !m_aborting; ++i)
		{
			string category = cddb_disc_get_category_str(m_disc);
			auto discid = cddb_disc_get_discid(m_disc);
			auto ndisc = do_read(m_conn, category.c_str(), discid, 0);
			if (ndisc)
			{
				m_di = DiscInfo();
				m_di.discid = id;
				m_di.title = cddb_disc_get_title(ndisc);
				m_di.artist = cddb_disc_get_artist(ndisc);
				m_di.year = cddb_disc_get_year(ndisc);
				for (auto track = cddb_disc_get_track_first(ndisc); !!track && !m_aborting; track = cddb_disc_get_track_next(ndisc))
					if (cddb_track_get_number(track) <= (int)m_di.tracks.size())
					{
						m_di.tracks[cddb_track_get_number(track) - 1].title = cddb_track_get_title(track);
						m_di.tracks[cddb_track_get_number(track) - 1].artist = cddb_track_get_artist(track);
					}
				m_dis.push_back(m_di);
				cddb_disc_destroy(ndisc);
			}
			cddb_query_next(m_conn, m_disc);
		}
	}
	if (!m_dis.size())
		m_dis.push_back(m_di);
}

void RIP::takeDiscInfo()
{
	m_di.title = m_info.title->text().toStdString();
	m_di.artist = m_info.artist->text().toStdString();
	m_di.setIndex = m_info.setIndex->value() - 1;
	m_di.setTotal = m_info.setTotal->value();
	m_di.year = m_info.year->value();
	for (unsigned i = 0; i < m_di.tracks.size(); ++i)
	{
		m_di.tracks[i].title = tSS(m_info.tracks->item(i, 0)->text());
		m_di.tracks[i].artist = tSS(m_info.tracks->item(i, 1)->text());
	}
}

size_t flacLength(string const& _fn)
{
	FLAC::Metadata::StreamInfo si;
	if (FLAC::Metadata::get_streaminfo(_fn.c_str(), si))
		return si.get_total_samples();
	return 0;
}

void RIP::rip()
{
	unsigned t = m_p.tracks();
	vector<std::thread*> encoders;
	for (unsigned i = 0; i < t && !m_aborting; ++i)
		if (m_progress[i].second)
		{
			string fn = ((ostringstream&)(ostringstream()<<m_path<<"/"<<m_temp<<"/"<<i)).str();
			if (QFile::exists(fSS(fn)) && flacLength(fn) == m_progress[i].second)
			{
				m_progress[i].first = m_progress[i].second;
				continue;
			}
			auto m = new mutex;
			auto incoming = new queue<int32_t*>;
			auto encoder = [=]()
			{
				FLAC::Encoder::File f;
				f.init(fn);
				f.set_channels(2);
				f.set_bits_per_sample(16);
				f.set_sample_rate(44100);
				f.set_compression_level(0);
				f.set_total_samples_estimate(m_progress[i].second);

				while (true)
				{
					m->lock();
					if (incoming->size())
					{
						auto n = incoming->front();
						if (!n)
							break;
						incoming->pop();
						m->unlock();
						f.process_interleaved(n, Paranoia::frameLength());
						delete n;
						m_progress[i].first += Paranoia::frameLength();
					}
					else
					{
						m->unlock();
						usleep(100000);
					}
				}
				f.finish();
				delete incoming;
				delete m;
			};
			auto ripper = [&](unsigned, unsigned, int16_t const* d) -> bool
			{
				int32_t* o = new int32_t[Paranoia::frameLength() * 2];
				auto dl = d + Paranoia::frameLength() * 2;
				for (int32_t* oi = o; d < dl; d += 8, oi += 8)
					oi[0] = d[0], oi[1] = d[1], oi[2] = d[2], oi[3] = d[3],
					oi[4] = d[4], oi[5] = d[5], oi[6] = d[6], oi[7] = d[7];
				m->lock();
				incoming->push(o);
				m->unlock();
				return !m_aborting;
			};
			encoders.push_back(new std::thread(encoder));
			m_p.rip(i, ripper);
			m->lock();
			incoming->push(nullptr);
			m->unlock();
		}
	for (auto e: encoders)
	{
		e->join();
		delete e;
	}
	encoders.clear();
}


