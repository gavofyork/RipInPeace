#pragma once

#include <thread>
#include <list>
#include <QSystemTrayIcon>
#include "DiscInfo.h"
#include "Paranoia.h"
#include "ui_Info.h"

class QTableWidget;
class Settings;

struct cddb_conn_s;
struct cddb_disc_s;

class RIP: public QSystemTrayIcon
{
	Q_OBJECT
	
public:
	RIP();
	~RIP();

	QString directory() const { return QString::fromUtf8(m_path.c_str()); }
	QString filename() const { return QString::fromUtf8(m_filename.c_str()); }
	QString device() const { return QString::fromUtf8(m_device.c_str()); }
	int paranoia() const { return m_paranoia; }

public slots:
	void setDirectory(QString _s) { m_path = _s.toUtf8().data(); }
	void setFilename(QString _s) { m_filename = _s.toUtf8().data(); }
	void setParanoia(int _s) { m_paranoia = _s; }
	void setDevice(QString _s) { m_device = _s.toUtf8().data(); }

private slots:
	void onActivated(QSystemTrayIcon::ActivationReason);
	void onAbortRip();
	void onAbout();

private:
	void rip();
	void getDiscInfo();
	void eject();
	void takeDiscInfo();
	void tagAll();
	void moveAll();

	void readSettings();
	void writeSettings();

	virtual void timerEvent(QTimerEvent*);

	QWidget* m_popup;
	Settings* m_settings;
	Ui::Info m_info;

	DiscInfo m_di;
	std::list<DiscInfo> m_dis;

	std::string m_path;
	std::string m_filename;

	std::string m_device;
	int m_paranoia;

	void** m_discId;

	Paranoia m_p;
	std::thread* m_ripper;
	bool m_ripped;
	int m_lastPercentDone;
	bool m_poppedUp;

	std::vector<std::pair<unsigned, unsigned> > m_progress;

	std::string m_temp;

	QImage m_logo;
	QIcon m_normal;
	QIcon m_inactive;

	cddb_conn_s* m_conn;
	cddb_disc_s* m_disc;

	bool m_aborting;
};
