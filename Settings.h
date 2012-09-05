#pragma once

#include <QDialog>

namespace Ui {
class Settings;
}

class RIP;
class QTreeWidgetItem;

class Settings: public QDialog
{
	Q_OBJECT
	
public:
	explicit Settings(RIP* _parent);
	~Settings();

public slots:
	void on_close_clicked();
	void on_openDirectory_clicked();
	void populate();
	void on_device_currentIndexChanged(int _i);
	void on_paranoia_itemChanged();
	void on_filename_textChanged();

private:
	Ui::Settings *ui;

	RIP* m_rip;
};
