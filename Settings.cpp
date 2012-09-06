#include <QFileDialog>
#include <QStringList>
#include <QScriptEngine>
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

	// populate devices before m_rip is set so they don't interfere with the configuration.
	for (auto d: Paranoia::devices())
		ui->device->addItem(QString::fromUtf8(d.second.c_str()), QString::fromUtf8(d.first.c_str()));
	for (auto f: Paranoia::flags())
	{
		auto it = new QTreeWidgetItem(ui->paranoia, QStringList(QString::fromUtf8(f.second.c_str())));
		it->setData(0, Qt::UserRole, f.first);
		it->setCheckState(0, Qt::Unchecked);
	}

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
	QString s = QFileDialog::getExistingDirectory(this, "Locate base directory for music", ui->directory->text());
	if (!s.isEmpty())
		ui->directory->setText(s);
}

void Settings::on_device_currentIndexChanged(int _i)
{
	if (m_rip)
		m_rip->setDevice(ui->device->itemData(_i).toString());
}

void Settings::on_paranoia_itemChanged()
{
	if (m_rip)
	{
		int f = 0;
		for (int i = 0; i < ui->paranoia->topLevelItemCount(); ++i)
			if (ui->paranoia->topLevelItem(i)->checkState(0) == Qt::Checked)
				f |= ui->paranoia->topLevelItem(i)->data(0, Qt::UserRole).toInt();
		m_rip->setParanoia(f);
	}
}

void Settings::on_filename_textChanged()
{
	if (m_rip)
		m_rip->setFilename(ui->filename->toPlainText());

	QScriptEngine s;
	s.globalObject().setProperty("disctitle", "Programmed to Love", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("discartist", "Bent", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("index", 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("total", 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("year", 1999, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("compilation", false, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("number", 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("sortnumber", "01", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("title", "Exercise 1", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("artist", "Bent", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	auto t = s.evaluate(ui->filename->toPlainText()).toString();
	s.globalObject().setProperty("disctitle", "Batucada Vol. 2", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("discartist", "Various Artists", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("index", 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("total", 2, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("year", 2002, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("compilation", true, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("number", 1, QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("sortnumber", "01", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("title", "Toujours L'Amore", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	s.globalObject().setProperty("artist", "Dimitri from Paris", QScriptValue::ReadOnly|QScriptValue::Undeletable);
	t += "\n"+s.evaluate(ui->filename->toPlainText()).toString();
	ui->examples->setPlainText(t);
}

void Settings::populate()
{
	ui->directory->setText(m_rip->directory());
	ui->filename->setPlainText(m_rip->filename());
	ui->device->setCurrentIndex(ui->device->findData(m_rip->device()));
	int fs = m_rip->paranoia();
	for (int i = 0; i < ui->paranoia->topLevelItemCount(); ++i)
	{
		int f = ui->paranoia->topLevelItem(i)->data(0, Qt::UserRole).toInt();
		ui->paranoia->topLevelItem(i)->setCheckState(0, (f & fs) ? Qt::Checked : Qt::Unchecked);
	}
}
