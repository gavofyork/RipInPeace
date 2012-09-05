#include <QFileDialog>
#include "Settings.h"
#include "RIP.h"
#include "ui_Settings.h"

Settings::Settings(RIP* _parent):
	QDialog(nullptr),
	ui(new Ui::Settings),
	m_rip(_parent)
{
	ui->setupUi(this);

	// TODO: populate devices.
	//ui->device

	populate();
	connect(ui->directory, SIGNAL(textEdited(QString)), m_rip, SLOT(setDirectory(QString)));
}

Settings::~Settings()
{
	delete ui;
}

void Settings::on_close_clicked()
{
	hide();
}

void Settings::on_openDirectory_clicked()
{
	QString s = QFileDialog::getExistingDirectory(0, "Locate base directory for music", ui->directory->text());
	if (!s.isEmpty())
		ui->directory->setText(s);
}

void Settings::on_device_changed()
{
	// m_rip->setDevice(/*new device's filename*/);
}

void Settings::on_paranoia_clicked()
{
	// m_rip->setParanoia(/*compiled flags*/);
}

void Settings::on_filename_textChanged()
{
	m_rip->setFilename(ui->filename->toPlainText());
}

void Settings::populate()
{
	ui->directory->setText(m_rip->directory());
	ui->filename->setPlainText(m_rip->filename());
//	ui->device-> /*set device from*/ m_rip->device();
//	ui->paranoia-> /*set each flag from*/ m_rip->paranoia();
}
