#include <queue>
#include <thread>
#include <mutex>
#include <sstream>
#include <iostream>
#include <iomanip>
#include <cassert>
#include <FLAC++/all.h>
#include <musicbrainz4/Query.h>
#include <discid/discid.h>
#include <QtGui>
#include "ui_Info.h"
#include "RIP.h"
using namespace std;

RIP::RIP(): m_device("/dev/cdrom2"), m_discId(nullptr), m_ripper(nullptr), m_ripped(false)
{
	setIcon(QIcon(QIcon("/home/gav/rip.png").pixmap(QSize(22, 22), QIcon::Disabled)));
	startTimer(1000);
	connect(this, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), SLOT(onActivated(QSystemTrayIcon::ActivationReason)));
	setContextMenu(new QMenu("Rip In Peace"));
	contextMenu()->addAction("Abort Rip", this, SLOT(onAbortRip()));
	contextMenu()->addSeparator();
	contextMenu()->addAction("About", this, SLOT(onAbout()));
	contextMenu()->addAction("Quit", qApp, SLOT(quit()));
	m_popup = new QWidget(0, Qt::FramelessWindowHint);
	m_info.setupUi(m_popup);
}

RIP::~RIP()
{
}

void RIP::onAbout()
{
}

void RIP::onAbortRip()
{
}

void RIP::onActivated(QSystemTrayIcon::ActivationReason)
{
	m_popup->setVisible(!m_popup->isVisible());
	if (m_popup->isVisible())
		m_popup->move(QCursor::pos());
}

void RIP::timerEvent(QTimerEvent*)
{
	if (m_ripped)
	{
		m_ripper->join();
		delete m_ripper;
		m_ripper = nullptr;
		m_ripped = false;
		// TODO: Tag & move into place.
	}
	if (!m_ripper)
	{
		if ((m_p.isOpen() || m_p.open(m_device)) && m_p.tracks() > 0)
		{
			// Begin ripping.
			setIcon(QIcon("/home/gav/rip.png"));
			if (m_discId)
				discid_free(m_discId);
			m_discId = discid_new();
			if (discid_read(m_discId, m_device.c_str()) == 0 )
			{
				fprintf(stderr, "Error: %s\n", discid_get_error_msg(m_discId));
				discid_free(m_discId);
				m_discId = nullptr;
			}
			m_ripper = new std::thread([&](){ rip(); m_ripped = true; });
			getDiscInfo();
		}
		else
		{
			setIcon(QIcon(QIcon("/home/gav/rip.png").pixmap(QSize(22, 22), QIcon::Disabled)));
		}
	}
}

template <class _T> string toString(_T const& _t) { ostringstream o; o << _t; return o.str(); }

inline QString fSS(string const& _s) { return QString::fromStdString(_s); }

void RIP::getDiscInfo()
{
	m_di = DiscInfo(m_p.tracks());
	if (m_discId)
	{
		printf("DiscID        : %s\n", discid_get_id(m_discId));
		printf("Submit via    : %s\n", discid_get_submission_url(m_discId));
	}
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

	// NOTE: m_di.isCompilation is determinable.
}

void RIP::rip()
{
	unsigned t = m_p.tracks();
	vector<std::thread*> encoders(t);
	size_t incomingLen = 0;
	for (unsigned i = 0; i < t; ++i)
	{
		qDebug() << "Starting" << (i+1);
		auto m = new mutex;
		auto incoming = new queue<int32_t*>;
		auto encoder = [=]()
		{
			FLAC::Encoder::File f;
			f.init(((ostringstream&)(ostringstream()<<"/tmp/track"<<setw(2)<<setfill('0')<<(i+1)<<".flac")).str());
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
					f.process_interleaved(n, incomingLen / 2);
					delete n;
				}
				else
				{
					m->unlock();
					usleep(1);
				}
			}
			qDebug() << "Finished" << (i+1);
			f.finish();
			delete incoming;
			delete m;
		};
		auto ripper = [&](unsigned current, unsigned total, int16_t const* d, size_t l)
		{
//			qDebug() << t << current << total;
			if (incomingLen)
				assert(incomingLen == l);
			else
				incomingLen = l;
			int32_t* o = new int32_t[l];
			auto dl = d + l;
			for (int32_t* oi = o; d < dl; d += 4, oi += 4)
				oi[0] = d[0], oi[1] = d[1], oi[2] = d[2], oi[3] = d[3];
			m->lock();
			incoming->push(o);
			m->unlock();
		};
		encoders[i] = new std::thread(encoder);
		m_p.rip(i, ripper);
		qDebug() << "Ripped" << (i+1);
		m->lock();
		incoming->push(nullptr);
		m->unlock();
	}
	qDebug() << "Joining...";
	for (auto e: encoders)
	{
		e->join();
		delete e;
	}
	qDebug() << "Done.";
	encoders.clear();
}
