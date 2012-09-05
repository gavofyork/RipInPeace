#pragma once

#include <thread>
#include <list>
#include <QSystemTrayIcon>
#include "DiscInfo.h"
#include "Paranoia.h"
#include "ui_Info.h"

class QTableWidget;

struct cddb_conn_s;
struct cddb_disc_s;

class RIP: public QSystemTrayIcon
{
	Q_OBJECT
	
public:
	RIP();
	~RIP();

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

	virtual void timerEvent(QTimerEvent*);

	QWidget* m_popup;
	Ui::Info m_info;

	DiscInfo m_di;
	std::list<DiscInfo> m_dis;

	std::string m_path;
	std::string m_filename;

	std::string m_device;

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
