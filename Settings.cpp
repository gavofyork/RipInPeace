#include <QFileDialog>
#include "Paranoia.h"
#include "RIP.h"
#include "ui_Settings.h"
#include "Settings.h"
using namespace std;

Settings::Settings(RIP* _parent):
	QDialog(nullptr),
	ui(new Ui::Settings),
	m_rip(nullptr)
{
	ui->setupUi(this);

	// populate devices before m_rip is set.
	for (auto d: Paranoia::devices())
		ui->device->addItem(QString::fromUtf8(d.second.c_str()), QString::fromUtf8(d.first.c_str()));
	m_rip = _parent;

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

void Settings::on_device_currentIndexChanged(int _i)
{
	if (m_rip)
		m_rip->setDevice(ui->device->itemData(_i).toString());
}

void Settings::on_paranoia_clicked()
{
	if (m_rip){}
	// m_rip->setParanoia(/*compiled flags*/);
}

void Settings::on_filename_textChanged()
{
	if (m_rip)
		m_rip->setFilename(ui->filename->toPlainText());
}

void Settings::populate()
{
	ui->directory->setText(m_rip->directory());
	ui->filename->setPlainText(m_rip->filename());
	QString d = m_rip->device();
	int i = ui->device->findData(d);
	ui->device->setCurrentIndex(i);
//	ui->paranoia-> /*set each flag from*/ m_rip->paranoia();
}
