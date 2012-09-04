#pragma once

#include <thread>
#include <QSystemTrayIcon>
#include "DiscInfo.h"
#include "Paranoia.h"
#include "ui_Info.h"

class QTableWidget;

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

	virtual void timerEvent(QTimerEvent*);

	QWidget* m_popup;
	Ui::Info m_info;

	DiscInfo m_di;

	std::string m_device;
	void** m_discId;

	Paranoia m_p;
	std::thread* m_ripper;
	bool m_ripped;
};
