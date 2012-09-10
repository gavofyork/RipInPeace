#pragma once

#include <thread>
#include <vector>
#include <QSystemTrayIcon>
#include <QDialog>
#include <QTime>
#include "DiscInfo.h"
#include "Paranoia.h"
#include "ui_Info.h"

class QAction;
class QTableWidget;
class Settings;

struct cddb_conn_s;
struct cddb_disc_s;

class RIP;

class Progress: public QWidget
{
public:
	Progress(RIP* _r);

	RIP* rip() const { return m_r; }

private:
	void paintEvent(QPaintEvent*);

	RIP* m_r;
};

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
	int squeeze() const { return m_squeeze; }

	float amountDone() const;
	QVector<QPair<float, float> > progressVector() const;
	std::vector<std::pair<unsigned, unsigned> > const& progress() const { return m_progress; }

public slots:
	void setDirectory(QString _s) { m_path = _s.toUtf8().data(); }
	void setFilename(QString _s) { m_filename = _s.toUtf8().data(); }
	void setParanoia(int _s) { m_paranoia = _s; }
	void setDevice(QString _s) { m_device = _s.toUtf8().data(); }
	void setSqueeze(int _s) { m_squeeze = _s; }
	void update();

	void onConfirm();
	void onUnconfirm();

private slots:
	void onActivated(QSystemTrayIcon::ActivationReason);
	void onAbortRip();
	void onAbout();
	void onQuit();

	void updatePreset(int);
	void plantInfo();
	void harvestInfo();

private:
	void rip();
	void getDiscInfo();
	void eject();
	void tagAll();
	void moveAll();
	void createPopup();
	void showPopup();

	void readSettings();
	void writeSettings();

	virtual void timerEvent(QTimerEvent*);

	QFrame* m_popup;
	Settings* m_settings;
	Ui::Info m_info;

	DiscInfo m_di;
	std::vector<DiscInfo> m_dis;
	DiscIdentity m_id;

	std::string m_path;
	std::string m_filename;

	std::string m_device;
	int m_paranoia;
	int m_squeeze;

	Paranoia m_p;
	std::thread* m_ripper;
	std::thread* m_identifier;
	bool m_ripped;
	bool m_identified;
	bool m_justRipped;
	bool m_confirmed;
	QTime m_started;
	int m_lastPercentDone;
	bool m_startingRip;

	std::vector<std::pair<unsigned, unsigned> > m_progress;

	std::string m_temp;

	QIcon m_inactive;

	QTime m_aborting;

	Progress* m_progressPie;
	QAction* m_abortRip;
	QAction* m_unconfirm;
#if !defined(FINAL)
	QAction* m_testIcon;
#endif
};
