//C-  -*- C++ -*-
//C- -------------------------------------------------------------------
//C- DjView4
//C- Copyright (c) 2006-  Leon Bottou
//C-
//C- This software is subject to, and may be distributed under, the
//C- GNU General Public License, either version 2 of the license,
//C- or (at your option) any later version. The license should have
//C- accompanied the software or you may obtain a copy of the license
//C- from the Free Software Foundation at http://www.fsf.org .
//C-
//C- This program is distributed in the hope that it will be useful,
//C- but WITHOUT ANY WARRANTY; without even the implied warranty of
//C- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//C- GNU General Public License for more details.
//C-  ------------------------------------------------------------------

#if AUTOCONF
# include "config.h"
#else
# define HAVE_STRING_H     1
# define HAVE_SYS_TYPES_H  1
# define HAVE_STRERROR     1
# ifndef WIN32
#  define HAVE_SYS_WAIT_H  1
#  define HAVE_WAITPID     1
#  define HAVE_UNISTD_H     1
# endif
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <limits.h>
#include <signal.h>
#if HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#if HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#if HAVE_STRING_H
# include <string.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_TIFF
# include <tiff.h>
# include <tiffio.h>
# include <tiffconf.h>
# include "tiff2pdf.h"
#endif
#ifdef WIN32
# include <io.h>
#endif

#include <QApplication>
#include <QBuffer>
#include <cstdarg>
#include <cstdio>
#include <QCheckBox>
#include <QDebug>
#include <QDir>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <jpeglib.h>
#include <setjmp.h>
#include <jbig2enc.h>
#include <leptonica/allheaders.h>
#include <QList>
#include <QMap>
#include <QMessageBox>
#include <QObject>
#include <QPointer>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintEngine>
#include <QPageLayout>
#include <QRegExp>
#include <QSettings>
#include <QSpinBox>
#include <QString>
#include <QTemporaryFile>
#include <QTimer>
#include <QPainter>
#include <QPageSize>
#include <QPdfWriter>
#include <QVector>

#include <libdjvu/miniexp.h>
#include <libdjvu/ddjvuapi.h>

#include "qdjview.h"
#include "qdjviewprefs.h"
#include "qdjviewdialogs.h"
#include "qdjviewexporters.h"

static QString
systemErrorString(int err)
{
#ifdef Q_OS_WIN
  char buffer[256] = {0};
  if (::strerror_s(buffer, sizeof(buffer), err) == 0)
    return QString::fromLocal8Bit(buffer);
  return QString();
#else
  return QString::fromLocal8Bit(::strerror(err));
#endif
}

#ifdef Q_OS_WIN
# define djv_fdopen _fdopen
# define djv_close _close
#else
# define djv_fdopen fdopen
# define djv_close close
#endif
#include "qdjvuwidget.h"
#include "qdjvu.h"

#ifdef WIN32
# define wdup(fh) _dup(fh)
#else
# define wdup(fh) (fh)
#endif

// ----------------------------------------
// FACTORY


typedef QDjViewExporter* (*exporter_creator_t)(QDialog*,QDjView*,QString);

static QStringList exporterNames;
static QMap<QString,QStringList> exporterInfo;
static QMap<QString,exporter_creator_t> exporterCreators;


static void
addExporterData(QString name, QString suffix, 
                QString lname, QString filter,
                exporter_creator_t creator)
{
  QStringList info;
  info << name << suffix << lname << filter;
  exporterNames << name;
  exporterInfo[name] = info;
  exporterCreators[name] = creator;
}


static void createExporterData();


QDjViewExporter *
QDjViewExporter::create(QDialog *dialog, QDjView *djview, QString name)
{
  if (exporterNames.isEmpty())
    createExporterData();
  if (exporterCreators.contains(name))
    return (*exporterCreators[name])(dialog, djview, name);
  return 0;
}

QStringList 
QDjViewExporter::names()
{
  if (exporterNames.isEmpty())
    createExporterData();
  return exporterNames;
}

QStringList 
QDjViewExporter::info(QString name)
{
  if (exporterNames.isEmpty())
    createExporterData();
  if (exporterInfo.contains(name))
    return exporterInfo[name];
  return QStringList();
}





// ----------------------------------------
// QDJVIEWEXPORTER



QDjViewExporter::~QDjViewExporter()
{ 
}


bool 
QDjViewExporter::exportOnePageOnly()
{ 
  return false; 
}


void 
QDjViewExporter::resetProperties() 
{ 
  // reset all properties to default values
}


void 
QDjViewExporter::loadProperties(QString) 
{ 
  // fill properties from settings keyed by string
}


void 
QDjViewExporter::saveProperties(QString) 
{ 
  // save properties into settings keyed by string
}


bool
QDjViewExporter::loadPrintSetup(QPrinter*, QPrintDialog*) 
{ 
  // fill properties using printer and print dialog data
  return false;
}


bool
QDjViewExporter::savePrintSetup(QPrinter*) 
{ 
  // save properties into printer settings
  return false;
}


int 
QDjViewExporter::propertyPages() 
{ 
  return 0; 
}


QWidget* 
QDjViewExporter::propertyPage(int) 
{ 
  return 0; 
}


void 
QDjViewExporter::setFromTo(int fp, int tp) 
{
  fromPage = fp;
  toPage = tp;
}


void 
QDjViewExporter::setErrorCaption(QString caption)
{
  errorCaption = caption;
}


ddjvu_status_t 
QDjViewExporter::status() 
{ 
  return DDJVU_JOB_NOTSTARTED; 
}


QString 
QDjViewExporter::name()
{
  return exporterName;
}


bool
QDjViewExporter::print(QPrinter *) 
{ 
  qWarning("QDjViewExporter does not support printing.");
  return false;
}


void 
QDjViewExporter::stop() 
{ 
}


void 
QDjViewExporter::error(QString message, QString filename, int lineno)
{ 
  if (! errorDialog)
    {
      errorDialog = new QDjViewErrorDialog(parent);
      errorDialog->prepare(QMessageBox::Critical, errorCaption);
      connect(errorDialog, SIGNAL(closing()), parent, SLOT(reject()));
      errorDialog->setWindowModality(Qt::WindowModal);
    }
  errorDialog->error(message, filename, lineno);
  errorDialog->show();
}


QDjViewExporter::QDjViewExporter(QDialog *parent, QDjView *djview, QString name)
  : parent(parent), djview(djview), 
    errorDialog(0),
    exporterName(name),
    fromPage(0), 
    toPage(-1)
{
}






// ----------------------------------------
// QDJVIEWDJVUEXPORTER




class QDjViewDjVuExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewDjVuExporter();
  QDjViewDjVuExporter(QDialog *parent, QDjView *djview, 
                      QString name, bool indirect);
  virtual bool save(QString fileName);
  virtual void stop();
  virtual ddjvu_status_t status();
protected:
  QFile file;
  QDjVuJob *job;
  FILE *output;
  bool indirect;
  bool failed;
};


QDjViewExporter*
QDjViewDjVuExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "DJVU/BUNDLED")
    return new QDjViewDjVuExporter(parent, djview, name, false);
  if (name == "DJVU/INDIRECT")
    return new QDjViewDjVuExporter(parent, djview, name, true);
  return 0;
}


void
QDjViewDjVuExporter::setup()
{
  addExporterData("DJVU/BUNDLED", "djvu",
                  tr("DjVu Bundled Document"),
                  tr("DjVu Files (*.djvu *.djv)"),
                  QDjViewDjVuExporter::create);
  addExporterData("DJVU/INDIRECT", "djvu",
                  tr("DjVu Indirect Document"),
                  tr("DjVu Files (*.djvu *.djv)"),
                  QDjViewDjVuExporter::create);
}


QDjViewDjVuExporter::QDjViewDjVuExporter(QDialog *parent, QDjView *djview, 
                                         QString name, bool indirect)
  : QDjViewExporter(parent, djview, name),
    job(0), 
    output(0),
    indirect(indirect), 
    failed(false)
{
}


QDjViewDjVuExporter::~QDjViewDjVuExporter()
{
  QIODevice::OpenMode mode = file.openMode();
  ddjvu_status_t st = status();
  if (st == DDJVU_JOB_STARTED)
	if (job)
      ddjvu_job_stop(*job);
  if (output)
    ::fclose(output);
  output = 0;
  if (file.openMode())
    file.close();
  if (st != DDJVU_JOB_OK)
      if (mode & (QIODevice::WriteOnly|QIODevice::Append))
        file.remove();
  if (djview)
    ddjvu_cache_clear(djview->getDjVuContext());
}


bool 
QDjViewDjVuExporter::save(QString fname)
{
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (output || document==0 || pagenum <= 0 || fname.isEmpty())
    return false;
  QFileInfo info(fname);
  QDir::Filters filters = QDir::AllEntries|QDir::NoDotAndDotDot;
  if (indirect && !info.dir().entryList(filters).isEmpty() &&
      QMessageBox::question(parent,
                            tr("Question - DjView", "dialog caption"),
                            tr("<html> This file belongs to a non empty "
                               "directory. Saving an indirect document "
                               "creates many files in this directory. "
                               "Do you want to continue and risk "
                               "overwriting files in this directory?"
                               "</html>"),
                            QMessageBox::Yes | QMessageBox::Cancel,
                            QMessageBox::Cancel) != QMessageBox::Yes )
    return false;
  toPage = qBound(0, toPage, pagenum-1);
  fromPage = qBound(0, fromPage, pagenum-1);
  QByteArray pagespec;
  if (fromPage == toPage && pagenum > 1)
    pagespec.append(QString("--pages=%1").arg(fromPage+1).toUtf8());
  else if (fromPage != 0 || toPage != pagenum - 1)
    pagespec.append(QString("--pages=%1-%2").arg(fromPage+1).arg(toPage+1).toUtf8());
  QByteArray namespec;
  if (indirect)
    namespec = "--indirect=" + fname.toUtf8();

  int argc = 0;
  const char *argv[2];
  if (! namespec.isEmpty())
    argv[argc++] = namespec.data();
  if (! pagespec.isEmpty())
    argv[argc++] = pagespec.data();
  
  file.setFileName(fname);
  file.remove();
  if (file.open(QIODevice::WriteOnly))
    output = djv_fdopen(wdup(file.handle()), "wb");
  if (! output)
    {
      failed = true;
      QString message = tr("Unknown error.");
      if (! file.errorString().isEmpty())
        message = tr("System error: %1.").arg(file.errorString());
      error(message, __FILE__, __LINE__);
      return false;
    }
  ddjvu_job_t *pjob;
  pjob = ddjvu_document_save(*document, output, argc, argv);
  if (! pjob)
    {
      failed = true;
      // error messages went to the document
      connect(document, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      qApp->sendEvent(&djview->getDjVuContext(), new QEvent(QEvent::User));
      disconnect(document, 0, this, 0);
      // main error message
      QString message = tr("Save job creation failed!");
      error(message, __FILE__, __LINE__);
      return false;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
  return true;
}


void 
QDjViewDjVuExporter::stop()
{
  if (job && status() == DDJVU_JOB_STARTED)
    ddjvu_job_stop(*job);
}


ddjvu_status_t 
QDjViewDjVuExporter::status()
{
  if (job)
    return ddjvu_job_status(*job);
  else if (failed)
    return DDJVU_JOB_FAILED;
  else
    return DDJVU_JOB_NOTSTARTED;
}





// ----------------------------------------
// QDJVIEWPSEXPORTER


#include "ui_qdjviewexportps1.h"
#include "ui_qdjviewexportps2.h"
#include "ui_qdjviewexportps3.h"


class QDjViewPSExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewPSExporter();
  QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                    QString name, bool eps);
  virtual bool exportOnePageOnly();
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual bool loadPrintSetup(QPrinter *printer, QPrintDialog *dialog);
  virtual bool savePrintSetup(QPrinter *printer);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
  virtual bool print(QPrinter *printer);
  virtual ddjvu_status_t status();
  virtual void stop();
public slots:
  void refresh();
protected:
  bool start();
  void openFile();
  void closeFile();
protected:
  QFile file;
  QDjVuJob *job;
  FILE *output;
  int outputfd;
  int copies;
  bool collate;
  bool lastfirst;
  bool duplex;
  bool failed;
  bool encapsulated;
  QPrinter *printer;
  QPointer<QWidget> page1;
  QPointer<QWidget> page2;
  QPointer<QWidget> page3;
  Ui::QDjViewExportPS1 ui1;
  Ui::QDjViewExportPS2 ui2;
  Ui::QDjViewExportPS3 ui3;
};


QDjViewExporter*
QDjViewPSExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PS")
    return new QDjViewPSExporter(parent, djview, name, false);
  if (name == "EPS")
    return new QDjViewPSExporter(parent, djview, name, true);
  return 0;
}


void
QDjViewPSExporter::setup()
{
  addExporterData("PS", "ps",
                  tr("PostScript"),
                  tr("PostScript Files (*.ps *.eps)"),
                  QDjViewPSExporter::create);
  addExporterData("EPS", "eps",
                  tr("Encapsulated PostScript"),
                  tr("PostScript Files (*.ps *.eps)"),
                  QDjViewPSExporter::create);
}


QDjViewPSExporter::~QDjViewPSExporter()
{
  // cleanup output
  QIODevice::OpenMode mode = file.openMode();
  ddjvu_status_t st = status();
  if (st == DDJVU_JOB_STARTED)
      if (job)
        ddjvu_job_stop(*job);
  closeFile();
  if (st != DDJVU_JOB_OK)
	if (mode & (QIODevice::WriteOnly|QIODevice::Append))
        file.remove();
  // delete property pages
  delete page1;
  delete page2;
  delete page3;
}


QDjViewPSExporter::QDjViewPSExporter(QDialog *parent, QDjView *djview, 
                                     QString name, bool eps)
  : QDjViewExporter(parent, djview, name),
    job(0), 
    output(0),
    outputfd(-1),
    copies(1),
    collate(true),
    lastfirst(false),
    duplex(false),
    failed(false),
    encapsulated(eps),
    printer(0)
{
  // create pages
  page1 = new QWidget();
  page2 = new QWidget();
  page3 = new QWidget();
  ui1.setupUi(page1);
  ui2.setupUi(page2);
  ui3.setupUi(page3);
  page1->setObjectName(tr("PostScript","tab caption"));
  page2->setObjectName(tr("Position","tab caption"));
  page3->setObjectName(tr("Booklet","tab caption"));
  resetProperties();
  // connect stuff
  connect(ui2.autoOrientCheckBox, SIGNAL(clicked()), 
          this, SLOT(refresh()) );
  connect(ui3.bookletCheckBox, SIGNAL(clicked()), 
          this, SLOT(refresh()) );
  connect(ui2.zoomSpinBox, SIGNAL(valueChanged(int)), 
          ui2.zoomButton, SLOT(click()) );
  // whatsthis
  page1->setWhatsThis(tr("<html><b>PostScript options.</b><br>"
                         "Option <tt>Color</tt> enables color printing. "
                         "Document pages can be decorated with frame "
                         "and crop marks. "
                         "PostScript language level 1 is only useful "
                         "with very old printers. Level 2 works with most "
                         "printers. Level 3 print color document faster "
                         "on recent printers.</html>") );
  page2->setWhatsThis(tr("<html><b>Position and scaling.</b><br>"
                         "Option <tt>Scale to fit</tt> accommodates "
                         "whatever paper size your printer uses. "
                         "Zoom factor <tt>100%</tt> reproduces the initial "
                         "document size. Orientation <tt>Automatic</tt> "
                         "chooses portrait or landscape on a page per "
                         "page basis.</html>") );
  page3->setWhatsThis(tr("<html><b>Producing booklets.</b><br>"
                         "The booklet mode prints the selected "
                         "pages as sheets suitable for folding one or several "
                         "booklets. Several booklets might be produced when "
                         "a maximum number of sheets per booklet is "
                         "specified. You can either use a duplex printer or "
                         "print rectos and versos separately.<p> "
                         "Shifting rectos and versos is useful "
                         "with poorly aligned duplex printers. "
                         "The center margins determine how much "
                         "space is left between the pages to fold the "
                         "sheets. This space slowly increases from the "
                         "inner sheet to the outer sheet."
                         "</html>") );
  // adjust ui
  refresh();
}


bool
QDjViewPSExporter::exportOnePageOnly()
{
  return encapsulated;
}


void 
QDjViewPSExporter::resetProperties()
{
  ui1.colorButton->setChecked(true);
  ui1.frameCheckBox->setChecked(false);
  ui1.cropMarksCheckBox->setChecked(false);
  ui1.levelSpinBox->setValue(3);
  ui2.autoOrientCheckBox->setChecked(true);
  ui2.scaleToFitButton->setChecked(true);
  ui2.zoomSpinBox->setValue(100);
  ui3.bookletCheckBox->setChecked(false);
  ui3.sheetsSpinBox->setValue(0);
  ui3.rectoVersoCombo->setCurrentIndex(0);
  ui3.rectoVersoShiftSpinBox->setValue(0);
  ui3.centerMarginSpinBox->setValue(18);
  ui3.centerIncreaseSpinBox->setValue(40);
  refresh();
}


void
QDjViewPSExporter::loadProperties(QString group)
{
  // load settings (ui1)
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  bool color = s.value("color", true).toBool();
  ui1.colorButton->setChecked(color);
  ui1.grayScaleButton->setChecked(!color);
  ui1.frameCheckBox->setChecked(s.value("frame", false).toBool());
  ui1.cropMarksCheckBox->setChecked(s.value("cropMarks", false).toBool());
  int level = s.value("psLevel", 3).toInt();
  ui1.levelSpinBox->setValue(qBound(1,level,3));
  // load settings (ui2)  
  ui2.autoOrientCheckBox->setChecked(s.value("autoOrient", true).toBool());
  bool landscape = s.value("landscape",false).toBool();
  ui2.portraitButton->setChecked(!landscape);
  ui2.landscapeButton->setChecked(landscape);
  int zoom = s.value("zoom",0).toInt();
  ui2.scaleToFitButton->setChecked(zoom == 0);
  ui2.zoomButton->setChecked(zoom!=0);
  ui2.zoomSpinBox->setValue(zoom ? qBound(25,zoom,2400) : 100);
  // load settings (ui3)  
  ui3.bookletCheckBox->setChecked(s.value("booklet", false).toBool());
  ui3.sheetsSpinBox->setValue(s.value("bookletSheets", 0).toInt());
  int rectoVerso = qBound(0, s.value("bookletRectoVerso",0).toInt(), 2);
  ui3.rectoVersoCombo->setCurrentIndex(rectoVerso);
  int rectoVersoShift = qBound(-72, s.value("bookletShift",0).toInt(), 72);
  ui3.rectoVersoShiftSpinBox->setValue(rectoVersoShift);
  int centerMargin = qBound(0, s.value("bookletCenter", 18).toInt(), 144);
  ui3.centerMarginSpinBox->setValue(centerMargin);
  int centerIncrease = qBound(0, s.value("bookletCenterAdd", 40).toInt(), 200);
  ui3.centerIncreaseSpinBox->setValue(centerIncrease);
}


void
QDjViewPSExporter::saveProperties(QString group)
{
  // save properties
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("color", ui1.colorButton->isChecked());
  s.setValue("frame", ui1.frameCheckBox->isChecked());
  s.setValue("cropMarks", ui1.cropMarksCheckBox->isChecked());
  s.setValue("psLevel", ui1.levelSpinBox->value());
  s.setValue("autoOrient", ui2.autoOrientCheckBox->isChecked());
  s.setValue("landscape", ui2.landscapeButton->isChecked());
  int zoom = ui2.zoomSpinBox->value();
  s.setValue("zoom", QVariant(ui2.zoomButton->isChecked() ? zoom : 0));
  s.setValue("booklet", ui3.bookletCheckBox->isChecked());
  s.setValue("bookletSheets", ui3.sheetsSpinBox->value());
  s.setValue("bookletRectoVerso", ui3.rectoVersoCombo->currentIndex());
  s.setValue("bookletShift", ui3.rectoVersoShiftSpinBox->value());
  s.setValue("bookletCenter", ui3.centerMarginSpinBox->value());
  s.setValue("bookletCenterAdd", ui3.centerIncreaseSpinBox->value());
}


bool
QDjViewPSExporter::loadPrintSetup(QPrinter *printer, QPrintDialog *dialog)
{
  bool grayscale = (printer->colorMode() == QPrinter::GrayScale);
  bool landscape = (printer->pageLayout().orientation() == QPageLayout::Landscape);
  ui1.grayScaleButton->setChecked(grayscale);  
  ui1.colorButton->setChecked(!grayscale);  
  ui2.landscapeButton->setChecked(landscape);
  ui2.portraitButton->setChecked(!landscape);
  // BEGIN HACK //
  // Directly steal settings from print dialog.
  copies = 1;
  collate = true;
  lastfirst = false;
#if QT_VERSION >= 0x50000
  QSpinBox* lcop = dialog->findChild<QSpinBox*>("copies");
  QCheckBox* lcol = dialog->findChild<QCheckBox*>("collate");
  QCheckBox* lplf = dialog->findChild<QCheckBox*>("reverse");
#elif QT_VERSION >= 0x40400
  QSpinBox* lcop = qFindChild<QSpinBox*>(dialog, "copies");
  QCheckBox* lcol = qFindChild<QCheckBox*>(dialog, "collate");
  QCheckBox* lplf = qFindChild<QCheckBox*>(dialog, "reverse");
#else
  QSpinBox* lcop = qFindChild<QSpinBox*>(dialog, "sbNumCopies");
  QCheckBox* lcol = qFindChild<QCheckBox*>(dialog, "chbCollate");
  QCheckBox* lplf = qFindChild<QCheckBox*>(dialog, "chbPrintLastFirst");
#endif
  if (lcop)
    copies = qMax(1, lcop->value());
  if (lcol)
    collate = lcol->isChecked();
  if (lplf)
    lastfirst = lplf->isChecked();
  // END HACK //
  return true;
}


bool
QDjViewPSExporter::savePrintSetup(QPrinter *printer)
{
  bool g = ui1.grayScaleButton->isChecked();
  bool l = ui2.landscapeButton->isChecked();
  printer->setColorMode(g ? QPrinter::GrayScale : QPrinter::Color);
  printer->setPageOrientation(l ? QPageLayout::Landscape : QPageLayout::Portrait);
  return true;
}


int 
QDjViewPSExporter::propertyPages()
{
  return (encapsulated) ? 1 : 3;
}


QWidget* 
QDjViewPSExporter::propertyPage(int n)
{
  if (n == 0)
    return page1;
  else if (n == 1)
    return page2;
  else if (n == 2)
    return page3;
  return 0;
}


void 
QDjViewPSExporter::refresh()
{
  if (encapsulated)
    {
      ui1.cropMarksCheckBox->setEnabled(false);
      ui1.cropMarksCheckBox->setChecked(false);
    }
  bool autoOrient = ui2.autoOrientCheckBox->isChecked();
  ui2.portraitButton->setEnabled(!autoOrient);
  ui2.landscapeButton->setEnabled(!autoOrient);
  bool booklet = ui3.bookletCheckBox->isChecked();
  ui3.bookletGroupBox->setEnabled(booklet);
  bool nojob = (status() < DDJVU_JOB_STARTED);
  page1->setEnabled(nojob);
  page2->setEnabled(nojob && !encapsulated);
  page3->setEnabled(nojob && !encapsulated);
}


void 
QDjViewPSExporter::openFile()
{
  if (printer)
    {
      file.close();
      if (! printer->outputFileName().isEmpty())
        file.setFileName(printer->outputFileName());
    }
  if (! file.fileName().isEmpty())
    {
      file.remove();
      if (file.open(QIODevice::WriteOnly))
        output = djv_fdopen(wdup(file.handle()), "wb");
    }
  else if (printer)
    {
      QString printerName = printer->printerName();
      // disable sigpipe
#ifdef SIGPIPE
# ifndef HAVE_SIGACTION
#  ifdef SA_RESTART
#   define HAVE_SIGACTION 1
#  endif
# endif
# if HAVE_SIGACTION
      sigset_t mask;
      struct sigaction act;
      sigemptyset(&mask);
      sigaddset(&mask, SIGPIPE);
      sigprocmask(SIG_BLOCK, &mask, 0);
      sigaction(SIGPIPE, 0, &act);
      act.sa_handler = SIG_IGN;
      sigaction(SIGPIPE, &act, 0);
# else
      signal(SIGPIPE, SIG_IGN);
# endif
#endif
      // Arguments for lp/lpr
      QVector<const char*> lpargs;
      QVector<const char*> lprargs;
      lpargs << "lp";
      lprargs << "lpr";
      QByteArray pname = printerName.toLocal8Bit();
      if (! pname.isEmpty())
        {
          lpargs << "-d";
          lpargs << pname.data();
          lprargs << "-P";
          lprargs << pname.data();
        }
      // Arguments for cups.
      bool cups = false;
      QList<QByteArray> cargs;
      QPrintEngine *engine = printer->printEngine();
# define PPK_CupsOptions QPrintEngine::PrintEnginePropertyKey(0xfe00)
# define PPK_CupsStringPageSize QPrintEngine::PrintEnginePropertyKey(0xfe03)
      QVariant cPageSize = engine->property(PPK_CupsStringPageSize);
      QVariant cOptions = engine->property(PPK_CupsOptions);
      if (! cPageSize.toString().isEmpty())
        {
          cups = true;
          cargs << "-o" << "media=" + cPageSize.toString().toLocal8Bit();
        }
      QStringList options = cOptions.toStringList();
      if (! options.isEmpty())
        {
          cups = true;
          QStringList::const_iterator it = options.constBegin();
          while (it != options.constEnd())
            {
              cargs << "-o";
              cargs << (*it).toLocal8Bit() + "=" + (*(it+1)).toLocal8Bit();
              it += 2;
            }
        }
      QByteArray bCopies = QByteArray::number(copies);
      if (cups)
        {
          if (collate)
            cargs << "-o" << "Collate=True";
          if (lastfirst)
            cargs << "-o" << "outputorder=reverse";
          lastfirst = false;
          if (duplex)
            cargs << "-o" << "sides=two-sided-long-edge";
          // add cups option for number of copies
          if (copies > 1)
            {
              lprargs << "-#" << bCopies.data();
              lpargs << "-n" << bCopies.data();
            }
          // add remaining cups options
          for(int i=0; i<cargs.size(); i++)
            {
              lprargs << cargs.at(i).data();
              lpargs << cargs.at(i).data();
            }
          // reset copies since cups takes care of it
          copies = 1;
        }
      // open pipe for lp/lpr.
      lpargs << 0;
      lprargs << 0;
#ifndef WIN32
      int fds[2];
      if (pipe(fds) == 0)
        {
          int pid = fork();
          if (pid == 0)
            {
              // rehash file descriptors
#if defined(_SC_OPEN_MAX)
              int nfd = (int)sysconf(_SC_OPEN_MAX);
#elif defined(_POSIX_OPEN_MAX)
              int nfd = (int)_POSIX_OPEN_MAX;
#elif defined(OPEN_MAX)
              int nfd = (int)OPEN_MAX;
#else
              int nfd = 256;
#endif
              djv_close(0);
              int status = ::dup(fds[0]);
              for (int i=1; i<nfd; i++)
                djv_close(i);
              if (status >= 0 && fork() == 0)
                {
                  // try lp and lpr
                  (void)execvp("lpr", (char**)lprargs.data());
                  (void)execv("/bin/lpr", (char**)lprargs.data());
                  (void)execv("/usr/bin/lpr", (char**)lprargs.data());
                  (void)execvp("lp", (char**)lpargs.data());
                  (void)execv("/bin/lp", (char**)lpargs.data());
                  (void)execv("/usr/bin/lp", (char**)lpargs.data());
                }
              // exit without running destructors
              (void)execlp("true", "true", (char *)0);
              (void)execl("/bin/true", "true", (char *)0);
              (void)execl("/usr/bin/true", "true", (char *)0);
              ::_exit(0);
              ::exit(0);
            }
          djv_close(fds[0]);
          outputfd = fds[1];
          output = djv_fdopen(outputfd, "wb");
          if (pid >= 0)
            {
# if HAVE_WAITPID
              ::waitpid(pid, 0, 0);
# else
              ::wait(0);
# endif
            }
          else
            closeFile();
        }
#endif
    }
}


void 
QDjViewPSExporter::closeFile()
{
  if (output)
    ::fclose(output);
  if (outputfd >= 0)
    djv_close(outputfd);
  if (file.openMode())
    file.close();
  output = 0;
  outputfd = -1;
  printer = 0;
}


bool
QDjViewPSExporter::save(QString fname)
{
  if (output) 
    return false;
  printer = 0;
  file.close();
  file.setFileName(fname);
  return start();
}


bool
QDjViewPSExporter::print(QPrinter *qprinter)
{
  if (output) 
    return false;
  printer = qprinter;
  file.close();
  file.setFileName(QString());
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  prefs->printReverse = lastfirst;
  prefs->printCollate = collate;
  return start();
}


bool 
QDjViewPSExporter::start()
{
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0)
    return false;
  int tpg = qBound(0, toPage, pagenum-1);
  int fpg = qBound(0, fromPage, pagenum-1);
  openFile();
  if (! output)
    return false;

  // prepare arguments
  QList<QByteArray> args;
  if (copies > 1)
    args << QString("--copies=%1").arg(copies).toLocal8Bit();
  if (fromPage == toPage && pagenum > 1)
    args << QString("--pages=%1").arg(fpg+1).toLocal8Bit();
  else if (fromPage != 0 || toPage != pagenum - 1)
    args << QString("--pages=%1-%2").arg(fpg+1).arg(tpg+1).toLocal8Bit();
  if (! ui2.autoOrientCheckBox->isChecked())
    {
      if (ui2.portraitButton->isChecked())
        args << QByteArray("--orientation=portrait");
      else if (ui2.landscapeButton->isChecked())
        args << QByteArray("--orientation=landscape");
    }
  if (ui2.zoomButton->isChecked())
    args << QString("--zoom=%1").arg(ui2.zoomSpinBox->value()).toLocal8Bit();
  if (ui1.frameCheckBox->isChecked())
    args << QByteArray("--frame=yes");
  if (ui1.cropMarksCheckBox->isChecked())
    args << QByteArray("--cropmarks=yes");
  if (ui1.levelSpinBox)
    args << QString("--level=%1").arg(ui1.levelSpinBox->value()).toLocal8Bit();
  if (ui1.grayScaleButton->isChecked())
    args << QByteArray("--color=no");
  if (prefs && prefs->printerGamma != 0.0)
    {
      double gamma = qBound(0.3, prefs->printerGamma, 5.0);
      args << QByteArray("--colormatch=no");
      args << QString("--gamma=%1").arg(gamma).toLocal8Bit();
    }
  if (ui3.bookletCheckBox->isChecked())
    {
      int bMode = ui3.rectoVersoCombo->currentIndex();
      if (bMode == 1)
        args << QByteArray("--booklet=recto");
      else if (bMode == 2)
        args << QByteArray("--booklet=verso");
      else
        args << QByteArray("--booklet=yes");
      int bMax = ui3.sheetsSpinBox->value();
      if (bMax > 0)
        args << QString("--bookletmax=%1").arg(bMax).toLocal8Bit();
      int bAlign = ui3.rectoVersoShiftSpinBox->value();
      args << QString("--bookletalign=%1").arg(bAlign).toLocal8Bit();
      int bCenter = ui3.centerMarginSpinBox->value();
      int bIncr = ui3.centerIncreaseSpinBox->value() * 10;
      args << QString("--bookletfold=%1+%2")
        .arg(bCenter).arg(bIncr).toLocal8Bit();
    }
  if (encapsulated)
    args << QByteArray("--format=eps");
  QDjVuWidget::DisplayMode displayMode;
  displayMode = djview->getDjVuWidget()->displayMode();
  if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
    args << QByteArray("--mode=black");
  else if (displayMode == QDjVuWidget::DISPLAY_BG)
    args << QByteArray("--mode=background");
  else if (displayMode == QDjVuWidget::DISPLAY_FG)
    args << QByteArray("--mode=foreground");

  // convert arguments
  int argc = args.count();
  QVector<const char*> argv(argc);
  for (int i=0; i<argc; i++)
    argv[i] = args[i].constData();

  // start print job
  ddjvu_job_t *pjob;
  pjob = ddjvu_document_print(*document, output, argc, argv.data());
  if (! pjob)
    {
      failed = true;
      // error messages went to the document
      connect(document, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      qApp->sendEvent(&djview->getDjVuContext(), new QEvent(QEvent::User));
      disconnect(document, 0, this, 0);
      // main error message
      QString message = tr("Save job creation failed!");
      error(message, __FILE__, __LINE__);
      return false;
    }
  job = new QDjVuJob(pjob, this);
  connect(job, SIGNAL(error(QString,QString,int)),
          this, SLOT(error(QString,QString,int)) );
  connect(job, SIGNAL(progress(int)),
          this, SIGNAL(progress(int)) );
  emit progress(0);
  refresh();
  return true;
}


void 
QDjViewPSExporter::stop()
{
  if (job && status() == DDJVU_JOB_STARTED)
    ddjvu_job_stop(*job);
}


ddjvu_status_t 
QDjViewPSExporter::status()
{
  if (job)
    return ddjvu_job_status(*job);
  else if (failed)
    return DDJVU_JOB_FAILED;
  else
    return DDJVU_JOB_NOTSTARTED;
}


// ----------------------------------------
// QDJVIEWPAGEEXPORTER


class QDjViewPageExporter : public QDjViewExporter
{
  Q_OBJECT
public:
  ~QDjViewPageExporter();
  QDjViewPageExporter(QDialog *parent, QDjView *djview, QString name);
  virtual ddjvu_status_t status() { return curStatus; }
public slots:
  virtual bool start();
  virtual void stop();
  virtual void error(QString message, QString filename, int lineno);
protected slots:
  void checkPage();
protected:
  virtual void openFile() {}
  virtual void closeFile() {}
  virtual void doFinal() {}
  virtual void doPage() = 0;
  ddjvu_status_t curStatus;
  QDjVuPage *curPage;
  int curProgress;
};


QDjViewPageExporter::~QDjViewPageExporter()
{
  curStatus = DDJVU_JOB_NOTSTARTED;
  closeFile();
}


QDjViewPageExporter::QDjViewPageExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewExporter(parent, djview, name),
    curStatus(DDJVU_JOB_NOTSTARTED), 
    curPage(0), curProgress(0)
{
}


bool 
QDjViewPageExporter::start()
{
  if (curStatus != DDJVU_JOB_STARTED)
    {
      curStatus = DDJVU_JOB_STARTED;
      openFile();
      checkPage();
    }
  if (curStatus <= DDJVU_JOB_OK)
    return true;
  return false;
}


void 
QDjViewPageExporter::stop()
{
  if (curStatus == DDJVU_JOB_STARTED)
    curStatus = DDJVU_JOB_STOPPED;
  if (curPage)
    if (ddjvu_page_decoding_status(*curPage) == DDJVU_JOB_STARTED)
      ddjvu_job_stop(ddjvu_page_job(*curPage));
  emit progress(curProgress);
}


void 
QDjViewPageExporter::error(QString message, QString filename, int lineno)
{
  if (curStatus == DDJVU_JOB_STARTED)
    curStatus = DDJVU_JOB_FAILED;
  QDjViewExporter::error(message, filename, lineno);
}


void 
QDjViewPageExporter::checkPage()
{
  QDjVuDocument *document = djview->getDocument();
  int pagenum = djview->pageNum();
  if (document==0 || pagenum <= 0)
    return;
  ddjvu_status_t pageStatus = DDJVU_JOB_NOTSTARTED;
  if (curPage)
    pageStatus = ddjvu_page_decoding_status(*curPage);
  if (pageStatus > DDJVU_JOB_OK)
    {
      curStatus = pageStatus;
      emit progress(curProgress);
      disconnect(curPage, 0, this, 0);
      delete curPage;
      curPage = 0;
      closeFile();
      return;
    }
  int tpg = qBound(0, toPage, pagenum-1);
  int fpg = qBound(0, fromPage, pagenum-1);
  int nextPage = fpg;
  if (pageStatus == DDJVU_JOB_OK)
    {
      nextPage = curPage->pageNo() + 1;
      doPage();
      curProgress = 100 * (nextPage - fpg) / (1 + tpg - fpg);
      disconnect(curPage, 0, this, 0);
      delete curPage;
      curPage = 0;
      if (curStatus < DDJVU_JOB_OK && nextPage > tpg)
        {
          curStatus = DDJVU_JOB_OK;
          doFinal();
        }
      emit progress(curProgress);
      if (curStatus >= DDJVU_JOB_OK)
        closeFile();
    }
  if (curStatus == DDJVU_JOB_STARTED && !curPage)
    {
      curPage = new QDjVuPage(document, nextPage, this);
      connect(curPage, SIGNAL(pageinfo()), 
              this, SLOT(checkPage()));
      connect(curPage, SIGNAL(error(QString,QString,int)),
              this, SLOT(error(QString,QString,int)) );
      if (ddjvu_page_decoding_done(*curPage))
        QTimer::singleShot(0, this, SLOT(checkPage()));
    }
}





// ----------------------------------------
// QDJVIEWTIFFEXPORTER


#include "ui_qdjviewexporttiff.h"


class QDjViewTiffExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewTiffExporter();
  QDjViewTiffExporter(QDialog *parent, QDjView *djview, QString name);
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
protected:
  virtual void closeFile();
  virtual void doPage();
protected:
  void checkTiffSupport();
  Ui::QDjViewExportTiff ui;
  QPointer<QWidget> page;
  QFile file;
#if HAVE_TIFF
  TIFF *tiff;
#else
  FILE *tiff;
#endif
};


QDjViewExporter*
QDjViewTiffExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "TIFF")
    return new QDjViewTiffExporter(parent, djview, name);
  return 0;
}


void
QDjViewTiffExporter::setup()
{
  addExporterData("TIFF", "tiff",
                  tr("TIFF Document"),
                  tr("TIFF Files (*.tiff *.tif)"),
                  QDjViewTiffExporter::create);
}


QDjViewTiffExporter::~QDjViewTiffExporter()
{
  closeFile();
  delete page;
}


QDjViewTiffExporter::QDjViewTiffExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewPageExporter(parent, djview, name), 
    tiff(0)
{
  page = new QWidget();
  ui.setupUi(page);
  page->setObjectName(tr("TIFF Options", "tab caption"));
  resetProperties();
  page->setWhatsThis(tr("<html><b>TIFF options.</b><br>"
                        "The resolution box specifies an upper "
                        "limit for the resolution of the TIFF images. "
                        "Forcing bitonal G4 compression "
                        "encodes all pages in black and white "
                        "using the CCITT Group 4 compression. "
                        "Allowing JPEG compression uses lossy JPEG "
                        "for all non bitonal or subsampled images. "
                        "Otherwise, allowing deflate compression "
                        "produces more compact (but less portable) files "
                        "than the default packbits compression."
                        "</html>") );
}


void 
QDjViewTiffExporter::checkTiffSupport()
{
#ifdef CCITT_SUPPORT
  ui.bitonalCheckBox->setEnabled(true);
#else
  ui.bitonalCheckBox->setEnabled(false);
#endif
#ifdef JPEG_SUPPORT
  ui.jpegCheckBox->setEnabled(true);
  ui.jpegSpinBox->setEnabled(true);
#else
  ui.jpegCheckBox->setEnabled(false);
  ui.jpegSpinBox->setEnabled(false);
#endif
#ifdef ZIP_SUPPORT
  ui.deflateCheckBox->setEnabled(true);
#else
  ui.deflateCheckBox->setEnabled(false);
#endif
}


void     
QDjViewTiffExporter::resetProperties()
{
  ui.dpiSpinBox->setValue(600);
  ui.bitonalCheckBox->setChecked(false);
  ui.jpegCheckBox->setChecked(true);
  ui.jpegSpinBox->setValue(75);
  ui.deflateCheckBox->setChecked(true);
  checkTiffSupport();
}


void
QDjViewTiffExporter::loadProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  ui.dpiSpinBox->setValue(s.value("dpi", 600).toInt());
  ui.bitonalCheckBox->setChecked(s.value("bitonal", false).toBool());
  ui.jpegCheckBox->setChecked(s.value("jpeg", true).toBool());
  ui.jpegSpinBox->setValue(s.value("jpegQuality", 75).toInt());
  ui.deflateCheckBox->setChecked(s.value("deflate", true).toBool());
  checkTiffSupport();
}


void
QDjViewTiffExporter::saveProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("dpi", ui.dpiSpinBox->value());
  s.setValue("bitonal", ui.bitonalCheckBox->isChecked());
  s.setValue("jpeg", ui.jpegCheckBox->isChecked());
  s.setValue("jpegQuality", ui.jpegSpinBox->value());
  s.setValue("deflate", ui.deflateCheckBox->isChecked());
}


int
QDjViewTiffExporter::propertyPages()
{
  return 1;
}


QWidget* 
QDjViewTiffExporter::propertyPage(int num)
{
  if (num == 0)
    return page;
  return 0;
}


#if HAVE_TIFF

static QDjViewTiffExporter *tiffExporter;

static void 
tiffHandler(const char *, const char *fmt, va_list ap)
{
  QString message;
#if QT_VERSION >= 0x50500
  message.vasprintf(fmt, ap);
#else
  message.vsprintf(fmt, ap);
#endif
  tiffExporter->error(message, __FILE__, __LINE__);
}

#endif


void 
QDjViewTiffExporter::closeFile()
{
#if HAVE_TIFF
  tiffExporter = this;
  TIFFSetErrorHandler(tiffHandler);
  TIFFSetWarningHandler(0);
  if (tiff)
    TIFFClose(tiff);
#endif
  tiff = 0;
  QIODevice::OpenMode mode = file.openMode();
  file.close();
  if (curStatus > DDJVU_JOB_OK)
    if (mode & (QIODevice::WriteOnly|QIODevice::Append))
      file.remove();
}


bool 
QDjViewTiffExporter::save(QString fname)
{
  if (file.openMode())
    return false;
  file.setFileName(fname);
  return start();
}


void 
QDjViewTiffExporter::doPage()
{
  ddjvu_format_t *fmt = 0;
  char *image = 0;
  QString message;
#if HAVE_TIFF
  tiffExporter = this;
  TIFFSetErrorHandler(tiffHandler);
  TIFFSetWarningHandler(0);
  QDjVuPage *page = curPage;
  // open file or write directory
  if (tiff)
    TIFFWriteDirectory(tiff);
  else if (file.open(QIODevice::ReadWrite))
    tiff = TIFFFdOpen(wdup(file.handle()), file.fileName().toLocal8Bit().data(),"w");
  if (!tiff)
    message = tr("Cannot open output file.");
  else
    {
      // determine rectangle
      ddjvu_rect_t rect;
      int imgdpi = ddjvu_page_get_resolution(*page);
      int dpi = qMin(imgdpi, ui.dpiSpinBox->value());
      rect.x = rect.y = 0;
      rect.w = ( ddjvu_page_get_width(*page) * dpi + imgdpi/2 ) / imgdpi;
      rect.h = ( ddjvu_page_get_height(*page) * dpi + imgdpi/2 ) / imgdpi;
      // determine mode
      ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
      QDjVuWidget::DisplayMode displayMode;
      displayMode = djview->getDjVuWidget()->displayMode();
      if (ui.bitonalCheckBox->isChecked())
        mode = DDJVU_RENDER_BLACK;
      else if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
        mode = DDJVU_RENDER_BLACK;
      else if (displayMode == QDjVuWidget::DISPLAY_BG)
        mode = DDJVU_RENDER_BACKGROUND;
      else if (displayMode == QDjVuWidget::DISPLAY_FG)
        mode = DDJVU_RENDER_FOREGROUND;
      // determine format and compression
      ddjvu_page_type_t type = ddjvu_page_get_type(*page);
      ddjvu_format_style_t style = DDJVU_FORMAT_RGB24;
      int compression = COMPRESSION_NONE;
      if (ui.bitonalCheckBox->isChecked())
        style = DDJVU_FORMAT_MSBTOLSB;
      else if (mode==DDJVU_RENDER_BLACK 
               || type==DDJVU_PAGETYPE_BITONAL)
        style = (dpi == imgdpi) 
          ? DDJVU_FORMAT_MSBTOLSB 
          : DDJVU_FORMAT_GREY8;
#ifdef CCITT_SUPPORT
      if (style == DDJVU_FORMAT_MSBTOLSB 
          && TIFFFindCODEC(COMPRESSION_CCITT_T6))
        compression = COMPRESSION_CCITT_T6;
#endif
#ifdef JPEG_SUPPORT
      else if (ui.jpegCheckBox->isChecked()
               && style != DDJVU_FORMAT_MSBTOLSB 
               && TIFFFindCODEC(COMPRESSION_JPEG))
        compression = COMPRESSION_JPEG;
#endif
#ifdef ZIP_SUPPORT
      else if (ui.deflateCheckBox->isChecked()
               && style != DDJVU_FORMAT_MSBTOLSB 
               && TIFFFindCODEC(COMPRESSION_DEFLATE))
        compression = COMPRESSION_DEFLATE;
#endif
#ifdef PACKBITS_SUPPORT
      else if (TIFFFindCODEC(COMPRESSION_PACKBITS))
        compression = COMPRESSION_PACKBITS;
#endif
      fmt = ddjvu_format_create(style, 0, 0);
      ddjvu_format_set_row_order(fmt, 1);
      ddjvu_format_set_gamma(fmt, 2.2);
      // set parameters
      int quality = ui.jpegSpinBox->value();
      TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, (uint32)rect.w);
      TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, (uint32)rect.h);
      TIFFSetField(tiff, TIFFTAG_XRESOLUTION, (float)(dpi));
      TIFFSetField(tiff, TIFFTAG_YRESOLUTION, (float)(dpi));
      TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
      TIFFSetField(tiff, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
#ifdef CCITT_SUPPORT
      if (compression != COMPRESSION_CCITT_T6)
#endif
#ifdef JPEG_SUPPORT
        if (compression != COMPRESSION_JPEG)
#endif
#ifdef ZIP_SUPPORT
          if (compression != COMPRESSION_DEFLATE)
#endif
            TIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, (uint32)64);
      if (style == DDJVU_FORMAT_MSBTOLSB) {
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, (uint16)1);
        TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
        TIFFSetField(tiff, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
        TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISWHITE);
      } else {
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, (uint16)8);
        TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
#ifdef JPEG_SUPPORT
        if (compression == COMPRESSION_JPEG)
          TIFFSetField(tiff, TIFFTAG_JPEGQUALITY, quality);
#endif
        if (style == DDJVU_FORMAT_GREY8) {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)1);
          TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
        } else {
          TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, (uint16)3);
          TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression);
#ifdef JPEG_SUPPORT
          if (compression == COMPRESSION_JPEG) {
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_YCBCR);
            TIFFSetField(tiff, TIFFTAG_JPEGCOLORMODE, JPEGCOLORMODE_RGB);
          } else 
#endif
            TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
        }
      }
      // render and save
      char white = (char)0xff;
      int rowsize = rect.w * 3;
      if (style == DDJVU_FORMAT_MSBTOLSB) {
        white = 0x00;
        rowsize = (rect.w + 7) / 8;
      } else if (style == DDJVU_FORMAT_GREY8)
        rowsize = rect.w;
      if (! (image = (char*)malloc(rowsize * rect.h)))
        message = tr("Out of memory.");
      else if (rowsize != TIFFScanlineSize(tiff))
        message = tr("Internal error.");
      else 
        {
          char *s = image;
          ddjvu_rect_t *r = &rect;
          if (! ddjvu_page_render(*page, mode, r, r, fmt, rowsize, image))
            memset(image, white, rowsize * rect.h);
          for (int i=0; i<(int)rect.h; i++, s+=rowsize)
            TIFFWriteScanline(tiff, s, i, 0);
        }
    }
#else
  message = tr("TIFF export has not been compiled.");
#endif
  if (! message.isEmpty())
    error(message, __FILE__, __LINE__);
  if (fmt)
    ddjvu_format_release(fmt);
  if (image)
    free(image);
}







// ----------------------------------------
// QDJVIEWPDFTEXTEXPORTER
//
// PDF exporter with optional invisible text overlay ("sandwich PDF").
// Writes PDF 1.4 directly – bypasses QPdfWriter which in Qt6 always uses
// FlateDecode (zlib) for images. Direct writing ensures DCT (JPEG) streams.

#include "ui_qdjviewexportpdf.h"

// Word-level zone collected from the DjVu hidden-text tree.
struct PdfWordZone {
  QRect   djvuRect;   // DjVu pixels, bottom-left origin, y increases upward
  QString text;
};

// Recursively collect word-level zones from a miniexp text expression.
static void
pdfCollectWords(miniexp_t p, QVector<PdfWordZone> &words)
{
  if (!miniexp_consp(p) || !miniexp_symbolp(miniexp_car(p)))
    return;
  miniexp_t r = miniexp_cdr(p);
  if (!miniexp_consp(r) || !miniexp_numberp(miniexp_car(r))) return;
  int x1 = miniexp_to_int(miniexp_car(r)); r = miniexp_cdr(r);
  if (!miniexp_consp(r) || !miniexp_numberp(miniexp_car(r))) return;
  int y1 = miniexp_to_int(miniexp_car(r)); r = miniexp_cdr(r);
  if (!miniexp_consp(r) || !miniexp_numberp(miniexp_car(r))) return;
  int x2 = miniexp_to_int(miniexp_car(r)); r = miniexp_cdr(r);
  if (!miniexp_consp(r) || !miniexp_numberp(miniexp_car(r))) return;
  int y2 = miniexp_to_int(miniexp_car(r)); r = miniexp_cdr(r);
  if (x2 < x1 || y2 < y1) return;
  if (miniexp_consp(r) && miniexp_stringp(miniexp_car(r))) {
    const char *s = miniexp_to_str(miniexp_car(r));
    if (s && *s) {
      PdfWordZone z;
      z.djvuRect = QRect(x1, y1, x2 - x1, y2 - y1);
      z.text     = QString::fromUtf8(s);
      words.append(z);
    }
  } else {
    while (miniexp_consp(r)) {
      pdfCollectWords(miniexp_car(r), words);
      r = miniexp_cdr(r);
    }
  }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Direct libjpeg grayscale encoder.
// Qt's JPEG plugin may convert Format_Grayscale8 → RGB before encoding;
// calling libjpeg directly guarantees 1-channel (Y-only) output which is
// typically 3× smaller than an equivalent RGB JPEG for B&W scanned pages.
// ---------------------------------------------------------------------------
struct JpegGrayErrorMgr { jpeg_error_mgr pub; jmp_buf jmpBuf; };
static void jpegGrayErrorExit(j_common_ptr cinfo)
{
  longjmp(reinterpret_cast<JpegGrayErrorMgr *>(cinfo->err)->jmpBuf, 1);
}
static QByteArray encodeJpegGray(const QImage &img, int quality)
{
  Q_ASSERT(img.format() == QImage::Format_Grayscale8);
  jpeg_compress_struct cinfo;
  JpegGrayErrorMgr     jerr;
  cinfo.err           = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = jpegGrayErrorExit;
  unsigned char *outbuf  = nullptr;
  unsigned long  outsize = 0;
  if (setjmp(jerr.jmpBuf)) {
    jpeg_destroy_compress(&cinfo);
    if (outbuf) free(outbuf);
    return QByteArray();
  }
  jpeg_create_compress(&cinfo);
  jpeg_mem_dest(&cinfo, &outbuf, &outsize);
  cinfo.image_width      = (JDIMENSION)img.width();
  cinfo.image_height     = (JDIMENSION)img.height();
  cinfo.input_components = 1;
  cinfo.in_color_space   = JCS_GRAYSCALE;
  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, qBound(1, quality, 100), TRUE);
  jpeg_start_compress(&cinfo, TRUE);
  while (cinfo.next_scanline < cinfo.image_height) {
    JSAMPROW row = const_cast<JSAMPLE *>(
        reinterpret_cast<const JSAMPLE *>(img.constScanLine(cinfo.next_scanline)));
    jpeg_write_scanlines(&cinfo, &row, 1);
  }
  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);
  QByteArray result(reinterpret_cast<char *>(outbuf), (int)outsize);
  free(outbuf);
  return result;
}

// ---------------------------------------------------------------------------
// JBIG2 encoding helpers (Leptonica + jbig2enc).
// Used for B&W (grayscale) pages — lossless, much smaller than JPEG.
// ---------------------------------------------------------------------------

// Convert QImage (Format_Grayscale8) to Leptonica PIX* 1bpp via threshold.
// Dark pixels (value < 128) → foreground (1).  Caller must pixDestroy().
static PIX *qImageToPix1bpp(const QImage &gray)
{
  Q_ASSERT(gray.format() == QImage::Format_Grayscale8);
  const int w = gray.width(), h = gray.height();
  PIX *pix = pixCreate(w, h, 1);
  if (!pix) return nullptr;
  const int wpl = pixGetWpl(pix);
  for (int y = 0; y < h; y++) {
    const uchar  *src = gray.constScanLine(y);
    l_uint32     *dst = pixGetData(pix) + y * wpl;
    memset(dst, 0, wpl * sizeof(l_uint32));
    for (int x = 0; x < w; x++) {
      if (src[x] < 128) {             // dark pixel → set (foreground)
        // Leptonica bit layout: bit 31-k of word k/32, MSB-first.
        dst[x >> 5] |= (0x80000000u >> (x & 31));
      }
    }
  }
  return pix;
}

// Render a DjVu page using RENDER_BLACK at the given rect size directly into
// a Leptonica PIX* 1bpp — no RGB24 intermediate, no threshold needed.
// ddjvu MSBTOLSB format: each row is ceil(w/8) bytes, MSB-first.
// Leptonica 1bpp: each row is ceil(w/32)*4 bytes (32-bit words), MSB-first.
// Both are MSB-first so we can copy row-by-row with word-alignment padding.
// Returns nullptr on failure. Caller must pixDestroy().
static PIX *renderBlackToPix1bpp(ddjvu_page_t *page,
                                  int w, int h)
{
  const int srcStride = (w + 7) / 8;   // bytes per row, MSBTOLSB
  QByteArray buf(h * srcStride, '\0');

  ddjvu_rect_t rect; rect.x = rect.y = 0;
  rect.w = (unsigned)w; rect.h = (unsigned)h;
  ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_MSBTOLSB, 0, nullptr);
  ddjvu_format_set_row_order(fmt, 1);
  bool ok = ddjvu_page_render(page, DDJVU_RENDER_BLACK,
                               &rect, &rect, fmt,
                               srcStride,
                               buf.data());
  ddjvu_format_release(fmt);
  if (!ok) return nullptr;

  PIX *pix = pixCreate(w, h, 1);
  if (!pix) return nullptr;
  const int wpl = pixGetWpl(pix);          // 32-bit words per row in Leptonica
  const int dstStride = wpl * 4;           // bytes per row in Leptonica

  for (int y = 0; y < h; y++) {
    const uchar  *src = reinterpret_cast<const uchar *>(buf.constData()) + y * srcStride;
    l_uint32     *dst = pixGetData(pix) + y * wpl;
    memset(dst, 0, dstStride);
    // Copy full bytes; remaining bits within last partial word are already 0.
    memcpy(dst, src, srcStride);
    // ddjvu gives MSBTOLSB (byte 0 = leftmost 8 pixels, bit7=pixel0).
    // Leptonica wants the same bit order but in big-endian 32-bit words.
    // On little-endian (x86/x64) we need to byte-swap each 32-bit word.
    for (int word = 0; word < wpl; word++) {
      const l_uint32 v = dst[word];
      dst[word] = ((v & 0xFF000000u) >> 24)
                | ((v & 0x00FF0000u) >>  8)
                | ((v & 0x0000FF00u) <<  8)
                | ((v & 0x000000FFu) << 24);
    }
  }
  return pix;
}

// Encode a grayscale QImage as a single-page JBIG2 generic region.
// Returns raw segment data suitable for /JBIG2Decode embedding (no globals).
// full_headers=false → no JBIG2 file header → correct for PDF.
static QByteArray encodeJbig2Generic(const QImage &grayImg)
{
  if (grayImg.isNull()) return {};
  const QImage &gray = (grayImg.format() == QImage::Format_Grayscale8)
                     ? grayImg
                     : grayImg.convertToFormat(QImage::Format_Grayscale8);
  PIX *pix = qImageToPix1bpp(gray);
  if (!pix) return {};
  int       len  = 0;
  uint8_t  *data = jbig2_encode_generic(pix,
                     /*full_headers=*/false,
                     /*xres=*/0, /*yres=*/0,
                     /*dup_line_removal=*/false, &len);
  pixDestroy(&pix);
  if (!data || len <= 0) return {};
  QByteArray result(reinterpret_cast<const char *>(data), len);
  free(data);   // malloced by jbig2enc
  return result;
}

// Encode a DjVu page black stencil as JBIG2 using 1bpp direct render.
// Renders RENDER_BLACK at w×h, produces Leptonica PIX* via renderBlackToPix1bpp(),
// then JBIG2-encodes it. No intermediate grayscale / threshold.
static QByteArray encodeJbig2BlackRender(ddjvu_page_t *page, int w, int h)
{
  PIX *pix = renderBlackToPix1bpp(page, w, h);
  if (!pix) return {};
  int      len  = 0;
  uint8_t *data = jbig2_encode_generic(pix,
                    /*full_headers=*/false,
                    /*xres=*/0, /*yres=*/0,
                    /*dup_line_removal=*/false, &len);
  pixDestroy(&pix);
  if (!data || len <= 0) return {};
  QByteArray result(reinterpret_cast<const char *>(data), len);
  free(data);
  return result;
}

// RawPdfCharset – maps Unicode codepoints to single-byte codes for a
// Type3 font's custom encoding (up to 254 distinct codepoints).

// Local printf-into-QByteArray helper used throughout the PDF writer.
static QByteArray baf(const char *fmt, ...)
{
  char buf[1024];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  return QByteArray(buf);
}

class RawPdfCharset {
public:
  // Encode a codepoint; returns the byte code (allocates on first use).
  // Returns 0xFF if the table is full.
  uint8_t encode(uint32_t u) {
    auto it = u2c_.find(u);
    if (it != u2c_.end()) return it.value();
    if (nextCode_ > 253)  return 0xFF;
    uint8_t code = nextCode_++;
    u2c_[u] = code;
    c2u_.resize(nextCode_, 0);
    c2u_[code] = u;
    return code;
  }

  // Encode every character in s into a byte-string.
  QByteArray encodeString(const QString &s) {
    QByteArray out;
    out.reserve(s.size());
    for (const QChar c : s) {
      uint8_t b = encode((uint32_t)c.unicode());
      out += (char)b;
    }
    return out;
  }

  // List of "code /uniXXXX" pairs for the /Differences array.
  QByteArray buildDifferences() const {
    QByteArray d;
    for (int i = 0; i < (int)c2u_.size(); ++i)
      d += baf(" %d /uni%04X", i, (unsigned)c2u_[i]);
    return d;
  }

  int      size()           const { return (int)c2u_.size(); }
  uint32_t unicode(int idx) const { return c2u_.value(idx, '?'); }

private:
  QMap<uint32_t, uint8_t> u2c_;
  QVector<uint32_t>       c2u_;
  uint8_t nextCode_ = 0;
};

// ---------------------------------------------------------------------------
// RawPdfWriter – minimal PDF 1.4 writer.
//   * JPEG images embedded as DCT streams (true JPEG, respects quality setting)
//   * Invisible text layer via a Type3 font with ToUnicode CMap (Tr=3)
//   * Standard xref/trailer

class RawPdfWriter {
public:
  explicit RawPdfWriter() {}

  bool open(const QString &path, int jpegQuality) {
    jpegQ_ = qBound(1, jpegQuality, 100);
    file_.setFileName(path);
    if (!file_.open(QIODevice::WriteOnly | QIODevice::Truncate))
      return false;
    nextObj_ = 4;  // 1=Catalog 2=Pages 3=Font written in finalize()
    xref_.clear(); pageObjs_.clear(); charset_ = RawPdfCharset();
    write("%PDF-1.4\n%\xE2\xE3\xCF\xD3\n");
    return true;
  }

  bool isOpen() const { return file_.isOpen(); }

  // Add one page.
  // For JPEG pages (isJbig2=false): imgData must be a valid JPEG stream (FF D8).
  // For JBIG2 pages (isJbig2=true):  imgData is a raw JBIG2 generic region
  //   (full_headers=false output from jbig2_encode_generic), 1bpp, DeviceGray.
  bool addPage(double mmW, double mmH,
               const QByteArray &imgData, int imgW, int imgH,
               bool isGray, bool isJbig2,
               const QVector<PdfWordZone> &words)
  {
    if (!file_.isOpen()) return false;
    if (imgData.isEmpty()) return false;
    if (!isJbig2) {
      // Validate JPEG header
      if ((unsigned char)(imgData[0]) != 0xFF
       || (unsigned char)(imgData[1]) != 0xD8)
        return false;
    }

    // Page dimensions in PDF points (1 pt = 1/72 inch).
    const double ptW = mmW / 25.4 * 72.0;
    const double ptH = mmH / 25.4 * 72.0;

    // --- Image XObject ---
    const int imgObj = alloc();
    beginObj(imgObj);
    if (isJbig2) {
      write(baf(
        "<< /Type /XObject /Subtype /Image\n"
        "   /Width %d /Height %d\n"
        "   /ColorSpace /DeviceGray /BitsPerComponent 1\n"
        "   /Filter /JBIG2Decode /Length %d >>\n"
        "stream\n",
        imgW, imgH, (int)imgData.size()));
    } else {
      write(baf(
        "<< /Type /XObject /Subtype /Image\n"
        "   /Width %d /Height %d\n"
        "   /ColorSpace %s /BitsPerComponent 8\n"
        "   /Filter /DCTDecode /Length %d >>\n"
        "stream\n",
        imgW, imgH,
        isGray ? "/DeviceGray" : "/DeviceRGB",
        (int)imgData.size()));
    }
    file_.write(imgData);
    write("\nendstream\n");
    endObj();

    // --- Content stream ---
    // Coordinate note: DjVu text coords are in DjVu-pixel space, origin
    // bottom-left, y increases upward — the same orientation as PDF user space.
    // Scale factor: DjVu pixels → PDF points = ptW/imgW (horizontal),
    //                                           ptH/imgH (vertical).
    const double sx = ptW  / imgW;
    const double sy = ptH  / imgH;

    QByteArray cs;
    cs += baf(
      "q\n%.4f 0 0 %.4f 0 0 cm\n/Im Do\nQ\n", ptW, ptH);

    if (!words.isEmpty()) {
      cs += "BT\n3 Tr\n";   // Tr=3: invisible rendering mode
      for (const PdfWordZone &w : words) {
        if (w.text.isEmpty()) continue;
        const double x1  = w.djvuRect.left()   * sx;
        const double y1  = w.djvuRect.top()     * sy;  // bottom of word
        const double wPt = w.djvuRect.width()   * sx;
        const double hPt = w.djvuRect.height()  * sy;
        if (wPt <= 0.0 || hPt <= 0.0) continue;
        // Horizontal scale: stretch to fill word zone.
        // Approximate Helvetica avg advance = 0.5 × em height.
        const int    nch    = qMax(1, w.text.size());
        const double scaleX = wPt / (0.5 * hPt * nch);
        QByteArray encoded  = charset_.encodeString(w.text);
        // Hex-encode the string: <AABB...>
        QByteArray hex;
        hex.reserve(encoded.size() * 2);
        for (unsigned char byte : encoded)
          hex += baf("%02X", (unsigned)byte);
        // Text matrix [a 0 0 d e f]: a=scaleX*hPt, d=hPt, (e,f)=position.
        cs += baf(
          "/F0 1 Tf\n%.4f 0 0 %.4f %.4f %.4f Tm\n<%s> Tj\n",
          scaleX * hPt, hPt, x1, y1, hex.constData());
      }
      cs += "ET\n";
    }

    const int csObj = alloc();
    beginObj(csObj);
    write(baf("<< /Length %d >>\nstream\n", (int)cs.size()));
    file_.write(cs);
    write("\nendstream\n");
    endObj();

    // --- Page object ---
    const int pageObj = alloc();
    beginObj(pageObj);
    write(baf(
      "<< /Type /Page /Parent 2 0 R\n"
      "   /MediaBox [0 0 %.4f %.4f]\n"
      "   /Contents %d 0 R\n"
      "   /Resources << /XObject << /Im %d 0 R >>\n"
      "                 /Font << /F0 3 0 R >> >> >>\n",
      ptW, ptH, csObj, imgObj));
    endObj();

    pageObjs_.append(pageObj);
    return true;
  }

  // Add a compound (layered) page.
  // fgIsJbig2=false: FG = RENDER_FOREGROUND gray JPEG + Multiply blend.
  //   Gray fills (FGbz) are correctly preserved. Text at JPEG 300dpi quality.
  // fgIsJbig2=true: FG = RENDER_BLACK JBIG2 stencil as /ImageMask.
  //   Text is native-DPI pixel-perfect. Only use when no FGbz gray fills.
  // BG is always RENDER_BACKGROUND JPEG (IW44 layer, no stencil).
  bool addCompoundPage(double mmW, double mmH,
                       const QByteArray &bgData, int bgW, int bgH, bool bgIsGray,
                       const QByteArray &fgData, int fgW, int fgH,
                       bool fgIsJbig2,
                       const QVector<PdfWordZone> &words)
  {
    if (!file_.isOpen()) return false;
    if (bgData.isEmpty() || fgData.isEmpty()) return false;

    const double ptW = mmW / 25.4 * 72.0;
    const double ptH = mmH / 25.4 * 72.0;

    // Background image XObject (JPEG, gray or RGB).
    const int bgObj = alloc();
    beginObj(bgObj);
    write(baf(
      "<< /Type /XObject /Subtype /Image\n"
      "   /Width %d /Height %d\n"
      "   /ColorSpace %s /BitsPerComponent 8\n"
      "   /Filter /DCTDecode /Length %d >>\n"
      "stream\n",
      bgW, bgH, bgIsGray ? "/DeviceGray" : "/DeviceRGB", (int)bgData.size()));
    file_.write(bgData);
    write("\nendstream\n");
    endObj();

    // Foreground XObject: JBIG2 /ImageMask or gray JPEG for Multiply.
    const int fgObj = alloc();
    beginObj(fgObj);
    if (fgIsJbig2) {
      write(baf(
        "<< /Type /XObject /Subtype /Image\n"
        "   /Width %d /Height %d\n"
        "   /ImageMask true /BitsPerComponent 1\n"
        "   /Filter /JBIG2Decode /Length %d >>\n"
        "stream\n",
        fgW, fgH, (int)fgData.size()));
    } else {
      write(baf(
        "<< /Type /XObject /Subtype /Image\n"
        "   /Width %d /Height %d\n"
        "   /ColorSpace /DeviceGray /BitsPerComponent 8\n"
        "   /Filter /DCTDecode /Length %d >>\n"
        "stream\n",
        fgW, fgH, (int)fgData.size()));
    }
    file_.write(fgData);
    write("\nendstream\n");
    endObj();

    // ExtGState for Multiply (only in non-JBIG2 mode).
    int gsObj = -1;
    if (!fgIsJbig2) {
      gsObj = alloc();
      beginObj(gsObj);
      write("<< /Type /ExtGState /BM /Multiply >>\n");
      endObj();
    }

    // Content stream.
    const double sx = ptW / fgW;
    const double sy = ptH / fgH;
    QByteArray cs;
    cs += baf("q\n%.4f 0 0 %.4f 0 0 cm\n/BG Do\nQ\n", ptW, ptH);
    if (fgIsJbig2)
      cs += baf("q\n0 g\n%.4f 0 0 %.4f 0 0 cm\n/FG Do\nQ\n", ptW, ptH);
    else
      cs += baf("q\n/GSmul gs\n%.4f 0 0 %.4f 0 0 cm\n/FG Do\nQ\n", ptW, ptH);
    if (!words.isEmpty()) {
      cs += "BT\n3 Tr\n";
      for (const PdfWordZone &w : words) {
        if (w.text.isEmpty()) continue;
        const double x1  = w.djvuRect.left()   * sx;
        const double y1  = w.djvuRect.top()     * sy;
        const double wPt = w.djvuRect.width()   * sx;
        const double hPt = w.djvuRect.height()  * sy;
        if (wPt <= 0.0 || hPt <= 0.0) continue;
        const int    nch    = qMax(1, w.text.size());
        const double scaleX = wPt / (0.5 * hPt * nch);
        QByteArray encoded  = charset_.encodeString(w.text);
        QByteArray hex;
        hex.reserve(encoded.size() * 2);
        for (unsigned char byte : encoded)
          hex += baf("%02X", (unsigned)byte);
        cs += baf(
          "/F0 1 Tf\n%.4f 0 0 %.4f %.4f %.4f Tm\n<%s> Tj\n",
          scaleX * hPt, hPt, x1, y1, hex.constData());
      }
      cs += "ET\n";
    }

    const int csObj = alloc();
    beginObj(csObj);
    write(baf("<< /Length %d >>\nstream\n", (int)cs.size()));
    file_.write(cs);
    write("\nendstream\n");
    endObj();

    // Page object.
    const int pageObj = alloc();
    beginObj(pageObj);
    if (fgIsJbig2) {
      write(baf(
        "<< /Type /Page /Parent 2 0 R\n"
        "   /MediaBox [0 0 %.4f %.4f]\n"
        "   /Contents %d 0 R\n"
        "   /Resources << /XObject << /BG %d 0 R /FG %d 0 R >>\n"
        "                 /Font << /F0 3 0 R >> >> >>\n",
        ptW, ptH, csObj, bgObj, fgObj));
    } else {
      write(baf(
        "<< /Type /Page /Parent 2 0 R\n"
        "   /MediaBox [0 0 %.4f %.4f]\n"
        "   /Contents %d 0 R\n"
        "   /Resources << /XObject << /BG %d 0 R /FG %d 0 R >>\n"
        "                 /ExtGState << /GSmul %d 0 R >>\n"
        "                 /Font << /F0 3 0 R >> >> >>\n",
        ptW, ptH, csObj, bgObj, fgObj, gsObj));
    }
    endObj();

    pageObjs_.append(pageObj);
    return true;
  }

  // Flush cross-ref table and trailer. Returns false on I/O error.
  bool finalize() {
    if (!file_.isOpen()) return false;

    writeFont();   // obj 3

    // Pages object (obj 2).
    // NOTE: do NOT use baf() here — Kids array can exceed the 1024-byte limit.
    beginObj(2);
    QByteArray kids = "[";
    for (int p : pageObjs_) kids += QByteArray::number(p) + " 0 R ";
    kids += "]";
    QByteArray pagesDict = "<< /Type /Pages /Kids " + kids
                         + " /Count " + QByteArray::number((int)pageObjs_.size())
                         + " >>\n";
    write(pagesDict);
    endObj();

    // Catalog (obj 1).
    beginObj(1);
    write("<< /Type /Catalog /Pages 2 0 R >>\n");
    endObj();

    // Cross-reference table.
    const qint64 xrefPos = file_.pos();
    write(baf("xref\n0 %d\n", nextObj_));
    write("0000000000 65535 f \n");
    for (int i = 1; i < nextObj_; ++i)
      write(baf("%010lld 00000 n \n",
                                 (long long)xref_.value(i, 0)));

    write(baf(
      "trailer\n<< /Size %d /Root 1 0 R >>\nstartxref\n%lld\n%%%%EOF\n",
      nextObj_, (long long)xrefPos));

    file_.close();
    return true;
  }

  void abandon() {
    QString fn = file_.fileName();
    file_.close();
    if (!fn.isEmpty()) QFile::remove(fn);
  }

private:
  QFile             file_;
  QMap<int, qint64> xref_;
  int               nextObj_ = 4;
  QVector<int>      pageObjs_;
  RawPdfCharset     charset_;
  int               jpegQ_   = 75;

  int alloc() { return nextObj_++; }

  void beginObj(int n) {
    xref_[n] = file_.pos();
    file_.write(QByteArray::number(n) + " 0 obj\n");
  }
  void endObj()                     { file_.write("endobj\n"); }
  void write(const QByteArray &b)   { file_.write(b); }
  void write(const char *s)         { file_.write(s); }

  // Write the Type3 font (object 3) with ToUnicode CMap.
  void writeFont() {
    const int sz = charset_.size();

    // Empty glyph proc shared by all characters ("0 0 d0" = zero width, no paint).
    const QByteArray glyphStream("0 0 d0\n");
    const int glyphObj = alloc();
    beginObj(glyphObj);
    write(baf("<< /Length %d >>\nstream\n",
                               (int)glyphStream.size()));
    file_.write(glyphStream);
    write("\nendstream\n");
    endObj();

    // ToUnicode CMap stream.
    QByteArray cmap;
    cmap += "/CIDInit /ProcSet findresource begin\n"
            "12 dict begin\nbegincmap\n"
            "/CMapName /DjView-Text def\n"
            "/CMapType 1 def\n"
            "/CIDSystemInfo << /Registry (DjView) /Ordering (Text)"
            " /Supplement 0 >> def\n";
    cmap += baf(
      "1 begincodespacerange\n<00> <%02X>\nendcodespacerange\n",
      sz > 0 ? sz - 1 : 0);
    for (int base = 0; base < sz; base += 100) {
      const int end = qMin(base + 100, sz);
      cmap += baf("%d beginbfchar\n", end - base);
      for (int i = base; i < end; ++i) {
        uint32_t u = charset_.unicode(i);
        if (u < 0x10000) {
          cmap += baf("<%02X> <%04X>\n", i, u);
        } else {
          u -= 0x10000;
          cmap += baf("<%02X> <%04X%04X>\n", i,
                                       (unsigned)(0xD800 + (u >> 10)),
                                       (unsigned)(0xDC00 + (u & 0x3FF)));
        }
      }
      cmap += "endbfchar\n";
    }
    cmap += "endcmap\nCMapName currentdict /CMap defineresource pop\nend\nend\n";

    const int cmapObj = alloc();
    beginObj(cmapObj);
    write(baf("<< /Length %d >>\nstream\n", (int)cmap.size()));
    file_.write(cmap);
    write("\nendstream\n");
    endObj();

    // CharProcs: every glyph name → same empty glyph object.
    QByteArray charProcs = "<<\n";
    for (int i = 0; i < sz; ++i)
      charProcs += baf("  /uni%04X %d 0 R\n",
                                        (unsigned)charset_.unicode(i), glyphObj);
    charProcs += ">>";

    // Widths: all 1 (unit width in glyph space; actual visual width is 0).
    QByteArray widths = "[";
    for (int i = 0; i < sz; ++i) widths += "1 ";
    widths += "]";

    // Font object (obj 3) — built with QByteArray concatenation to avoid
    // baf()'s 1024-char limit being exceeded by large CharProcs/Widths.
    beginObj(3);
    QByteArray fontDict;
    fontDict += "<< /Type /Font /Subtype /Type3\n";
    fontDict += "   /FontBBox [0 0 1 1]\n";
    fontDict += "   /FontMatrix [0.001 0 0 0.001 0 0]\n";
    fontDict += baf("   /FirstChar 0 /LastChar %d\n", sz > 0 ? sz - 1 : 0);
    fontDict += "   /Widths " + widths + "\n";
    fontDict += "   /CharProcs " + charProcs + "\n";
    fontDict += "   /Encoding << /Type /Encoding /Differences [0";
    fontDict += charset_.buildDifferences();
    fontDict += "] >>\n";
    fontDict += baf("   /ToUnicode %d 0 R >>\n", cmapObj);
    file_.write(fontDict);
    endObj();
  }
};


class QDjViewPdfTextExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  QDjViewPdfTextExporter(QDialog *parent, QDjView *djview, QString name);
  ~QDjViewPdfTextExporter();
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
protected:
  virtual void closeFile();
  virtual void doPage();
private:
  Ui::QDjViewExportPdf ui;
  QPointer<QWidget>    settingsPage;
  QString              outFileName;
  RawPdfWriter        *rawPdf;
  QFile                logFile_;
};


QDjViewExporter*
QDjViewPdfTextExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PDF")
    return new QDjViewPdfTextExporter(parent, djview, name);
  return nullptr;
}


void
QDjViewPdfTextExporter::setup()
{
  addExporterData("PDF", "pdf",
                  tr("PDF Document"),
                  tr("PDF Files (*.pdf)"),
                  QDjViewPdfTextExporter::create);
}


QDjViewPdfTextExporter::QDjViewPdfTextExporter(QDialog *parent, QDjView *djview,
                                               QString  name)
  : QDjViewPageExporter(parent, djview, name),
    rawPdf(nullptr)
{
  settingsPage = new QWidget();
  ui.setupUi(settingsPage);
  settingsPage->setObjectName(tr("PDF Options", "tab caption"));
  resetProperties();
  settingsPage->setWhatsThis(
    tr("<html><b>PDF options.</b><br>"
       "The resolution box limits the maximum output image resolution. "
       "JPEG quality controls image compression "
       "(1&nbsp;=&nbsp;worst, 100&nbsp;=&nbsp;best). "
       "Enabling the text layer embeds the hidden OCR/text stored in "
       "the DjVu file as an invisible overlay, making the PDF "
       "searchable and allowing copy-paste of text."
       "</html>"));
}


QDjViewPdfTextExporter::~QDjViewPdfTextExporter()
{
  closeFile();
  delete settingsPage;
}


void
QDjViewPdfTextExporter::resetProperties()
{
  ui.dpiSpinBox->setValue(300);
  ui.jpegQualitySpinBox->setValue(75);
  ui.textLayerCheckBox->setChecked(true);
}


void
QDjViewPdfTextExporter::loadProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) group = "Export-" + name();
  s.beginGroup(group);
  ui.dpiSpinBox->setValue(s.value("dpi", 300).toInt());
  ui.jpegQualitySpinBox->setValue(s.value("jpegQuality", 75).toInt());
  ui.textLayerCheckBox->setChecked(s.value("textLayer", true).toBool());
}


void
QDjViewPdfTextExporter::saveProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("dpi", ui.dpiSpinBox->value());
  s.setValue("jpegQuality", ui.jpegQualitySpinBox->value());
  s.setValue("textLayer", ui.textLayerCheckBox->isChecked());
}


int
QDjViewPdfTextExporter::propertyPages()
{
  return 1;
}


QWidget*
QDjViewPdfTextExporter::propertyPage(int num)
{
  return (num == 0) ? settingsPage : nullptr;
}


void
QDjViewPdfTextExporter::closeFile()
{
  if (logFile_.isOpen()) {
    if (curStatus > DDJVU_JOB_OK)
      logFile_.write(QByteArray("EXPORT FAILED status=")
                     + QByteArray::number((int)curStatus) + "\n");
    else
      logFile_.write("EXPORT OK\n");
    logFile_.close();
  }
  if (!rawPdf) return;
  if (curStatus > DDJVU_JOB_OK)
    rawPdf->abandon();   // delete partial file on failure
  else
    rawPdf->finalize();
  delete rawPdf;
  rawPdf = nullptr;
}


bool
QDjViewPdfTextExporter::save(QString fname)
{
  if (rawPdf) return false;   // already running
  outFileName = fname;
  const int jpegQ = qBound(1, ui.jpegQualitySpinBox->value(), 100);
  // Open debug log next to the PDF file.
  logFile_.setFileName(fname + ".log");
  (void)logFile_.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text);
  if (logFile_.isOpen())
    logFile_.write(QByteArray("PDF export log: ") + fname.toUtf8() + "\n"
                   "dpi=" + QByteArray::number(ui.dpiSpinBox->value())
                   + " jpegQ=" + QByteArray::number(jpegQ) + "\n");
  rawPdf = new RawPdfWriter();
  if (!rawPdf->open(fname, jpegQ)) {
    delete rawPdf;
    rawPdf = nullptr;
    return false;
  }
  return start();
}


void
QDjViewPdfTextExporter::doPage()
{
  QDjVuPage     *page     = curPage;
  QDjVuDocument *document = djview->getDocument();
  const int pageno  = page->pageNo();
  const int imgdpi  = ddjvu_page_get_resolution(*page);
  const int srcW    = ddjvu_page_get_width(*page);
  const int srcH    = ddjvu_page_get_height(*page);

  // Physical page dimensions derived from DjVu page's own DPI.
  const double mmW = srcW * 25.4 / imgdpi;
  const double mmH = srcH * 25.4 / imgdpi;

  // Render at min(imgdpi, outDpi) to avoid unnecessary upscaling.
  const int outDpi    = qBound(72, ui.dpiSpinBox->value(), 1200);
  const int renderDpi = qMin(imgdpi, outDpi);
  const int renderW   = qMax(1, (srcW * renderDpi + imgdpi / 2) / imgdpi);
  const int renderH   = qMax(1, (srcH * renderDpi + imgdpi / 2) / imgdpi);

  // Determine render mode from viewer's current display setting.
  ddjvu_render_mode_t renderMode = DDJVU_RENDER_COLOR;
  {
    QDjVuWidget::DisplayMode dm = djview->getDjVuWidget()->displayMode();
    if      (dm == QDjVuWidget::DISPLAY_STENCIL) renderMode = DDJVU_RENDER_BLACK;
    else if (dm == QDjVuWidget::DISPLAY_BG)      renderMode = DDJVU_RENDER_BACKGROUND;
    else if (dm == QDjVuWidget::DISPLAY_FG)      renderMode = DDJVU_RENDER_FOREGROUND;
  }

  // Render always into RGB888 first — ddjvu_page_get_type() returns
  // PAGETYPE_UNKNOWN until decoding completes (i.e. until after render).
  ddjvu_rect_t rect; rect.x = rect.y = 0;
  rect.w = (unsigned)renderW;
  rect.h = (unsigned)renderH;
  ddjvu_format_t *fmt = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
  ddjvu_format_set_row_order(fmt, 1);
  ddjvu_format_set_gamma(fmt, 2.2);
  QImage qimg(renderW, renderH, QImage::Format_RGB888);
  qimg.fill(Qt::white);
  const bool renderOk = ddjvu_page_render(
    *page, renderMode, &rect, &rect, fmt,
    qimg.bytesPerLine(), reinterpret_cast<char *>(qimg.bits()));
  ddjvu_format_release(fmt);
  if (!renderOk) qimg.fill(Qt::white);

  // NOW type is known (decode completed during render).
  // For BITONAL pages rendered at reduced DPI: re-render at native DPI so
  // that the hard threshold binarization doesn't destroy fine details
  // (chess diagrams, thin lines, small font strokes etc.).
  // The page is already decoded so re-render is essentially free.
  const bool isBitonalEarly = (ddjvu_page_get_type(*page) == DDJVU_PAGETYPE_BITONAL);
  if (isBitonalEarly && renderDpi < imgdpi) {
    const int nativeW = srcW;
    const int nativeH = srcH;
    ddjvu_rect_t nrect; nrect.x = nrect.y = 0;
    nrect.w = (unsigned)nativeW;
    nrect.h = (unsigned)nativeH;
    ddjvu_format_t *fmt2 = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
    ddjvu_format_set_row_order(fmt2, 1);
    ddjvu_format_set_gamma(fmt2, 2.2);
    QImage nimg(nativeW, nativeH, QImage::Format_RGB888);
    nimg.fill(Qt::white);
    if (ddjvu_page_render(*page, renderMode, &nrect, &nrect, fmt2,
                          nimg.bytesPerLine(), reinterpret_cast<char *>(nimg.bits())))
      qimg = std::move(nimg);   // replace with native-DPI image
    ddjvu_format_release(fmt2);
  }

  // Convert to grayscale if:
  //   a) page is explicitly BITONAL — never has color, or
  //   b) stencil/black render mode, or
  //   c) rendered image has no significant chroma (compound pages that
  //      have only a Sjbz stencil with no IW44 color background also
  //      render as pure B&W even though type=COMPOUND).
  // Quick chroma check: sample every Nth pixel; if no pixel has
  // max(|R-G|,|G-B|,|R-B|) > 16, treat as grayscale.
  bool useGray = (ddjvu_page_get_type(*page) == DDJVU_PAGETYPE_BITONAL)
              || (renderMode == DDJVU_RENDER_BLACK);
  if (!useGray && qimg.format() == QImage::Format_RGB888) {
    const uchar *bits = qimg.constBits();
    const int   total = qimg.width() * qimg.height();
    const int   step  = qMax(1, total / 2000); // sample ~2000 pixels
    bool hasColor = false;
    for (int px = 0; px < total && !hasColor; px += step) {
      const uchar *p = bits + px * 3;
      int diff = qMax(qAbs((int)p[0]-(int)p[1]),
                       qMax(qAbs((int)p[1]-(int)p[2]),
                            qAbs((int)p[0]-(int)p[2])));
      if (diff > 16) hasColor = true;
    }
    useGray = !hasColor;
  }

  // COMPOUND pages: BG = RENDER_BACKGROUND JPEG at renderDpi (IW44 layer only:
  // photo, paper texture — no Sjbz text) + FG = RENDER_BLACK JBIG2 stencil at
  // native DPI drawn as /ImageMask (sharp black text/lines on top).
  // Using RENDER_BACKGROUND avoids double-rendering: the RENDER_COLOR image
  // contains anti-aliased text at 300dpi which would show as a gray halo under
  // the sharp JBIG2 stencil. RENDER_BACKGROUND has only the IW44 background.
  // BITONAL pages: skip this path, go straight to JBIG2-only below.
  bool forceJbig2 = isBitonalEarly;
  const ddjvu_page_type_t earlyType = ddjvu_page_get_type(*page);
  if (earlyType == DDJVU_PAGETYPE_COMPOUND && renderMode == DDJVU_RENDER_COLOR) {
    const int jpegQ = qBound(1, ui.jpegQualitySpinBox->value(), 100);
    // BG: fresh RENDER_BACKGROUND render at renderDpi (IW44 layer, no stencil).
    QByteArray bgData;
    bool bgIsGray = false;
    {
      ddjvu_rect_t brect; brect.x = brect.y = 0;
      brect.w = (unsigned)renderW; brect.h = (unsigned)renderH;
      ddjvu_format_t *fmtBg = ddjvu_format_create(DDJVU_FORMAT_RGB24, 0, nullptr);
      ddjvu_format_set_row_order(fmtBg, 1);
      QImage bgImg(renderW, renderH, QImage::Format_RGB888);
      bgImg.fill(Qt::white);
      const bool bgOk = ddjvu_page_render(*page, DDJVU_RENDER_BACKGROUND,
                           &brect, &brect, fmtBg,
                           bgImg.bytesPerLine(),
                           reinterpret_cast<char *>(bgImg.bits()));
      ddjvu_format_release(fmtBg);
      if (bgOk) {
        // Check chroma on background render to decide gray vs RGB JPEG.
        const uchar *bits = bgImg.constBits();
        const int total = renderW * renderH;
        const int step = qMax(1, total / 2000);
        bool hasColor = false;
        for (int px = 0; px < total && !hasColor; px += step) {
          const uchar *p = bits + px * 3;
          int diff = qMax(qAbs((int)p[0]-(int)p[1]),
                         qMax(qAbs((int)p[1]-(int)p[2]),
                              qAbs((int)p[0]-(int)p[2])));
          if (diff > 16) hasColor = true;
        }
        bgIsGray = !hasColor;
        if (bgIsGray) {
          bgData = encodeJpegGray(bgImg.convertToFormat(QImage::Format_Grayscale8), jpegQ);
        } else {
          QBuffer buf(&bgData); buf.open(QIODevice::WriteOnly);
          QImageWriter writer(&buf, "JPEG"); writer.setQuality(jpegQ);
          writer.write(bgImg);
        }
      }
    }
    // Decide FG mode by checking whether the gray fills are already in
    // RENDER_BACKGROUND (IW44) or only in FGbz (stencil colorization):
    //
    //  - If RENDER_BACKGROUND already has significant dark/gray content
    //    (non-white pixels), those fills are in IW44 → BG JPEG already
    //    carries them → use sharp JBIG2 ImageMask (no Multiply needed).
    //
    //  - If BG is nearly all white but RENDER_FOREGROUND has gray pixels,
    //    the gray fills live only in FGbz → must use Multiply with
    //    RENDER_FOREGROUND gray JPEG to preserve them (text will be 300dpi
    //    JPEG quality in this case).
    //
    bool bgHasGrayContent = false;
    if (!bgData.isEmpty()) {
      // Re-examine the already-rendered bgImg via its JPEG would be lossy;
      // instead re-check the pixel data we still have in bgImg.
      // We need bgImg from the render above — capture it outside the block.
    }
    // Re-render BG into a persistent image for the gray-content probe.
    QImage bgImgProbe;
    bool fgNeedsMultiply = false;
    {
      ddjvu_rect_t brect2; brect2.x = brect2.y = 0;
      brect2.w = (unsigned)renderW; brect2.h = (unsigned)renderH;
      // Use a small subsample for speed: render at renderDpi (already done),
      // just re-use the GREY8 format for the probe.
      ddjvu_format_t *fmtProbe = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, nullptr);
      ddjvu_format_set_row_order(fmtProbe, 1);
      QImage probeImg(renderW, renderH, QImage::Format_Grayscale8);
      probeImg.fill(Qt::white);
      if (ddjvu_page_render(*page, DDJVU_RENDER_BACKGROUND,
                            &brect2, &brect2, fmtProbe,
                            probeImg.bytesPerLine(),
                            reinterpret_cast<char *>(probeImg.bits()))) {
        // Count pixels darker than 210 (covers light grays like table headers).
        const uchar *bits2 = probeImg.constBits();
        const int total2 = renderW * renderH;
        const int step2  = qMax(1, total2 / 2000);
        int darkCount = 0;
        for (int px = 0; px < total2; px += step2)
          if (bits2[px] < 210) ++darkCount;
        // If >2% of sampled pixels are non-white → IW44 has the gray fills.
        bgHasGrayContent = (darkCount > (total2 / step2) / 50);
      }
      ddjvu_format_release(fmtProbe);

      if (!bgHasGrayContent) {
        // BG is nearly white → probe RENDER_FOREGROUND for FGbz gray.
        ddjvu_rect_t frect2; frect2.x = frect2.y = 0;
        frect2.w = (unsigned)renderW; frect2.h = (unsigned)renderH;
        ddjvu_format_t *fmtFg2 = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, nullptr);
        ddjvu_format_set_row_order(fmtFg2, 1);
        QImage fgProbe(renderW, renderH, QImage::Format_Grayscale8);
        fgProbe.fill(Qt::white);
        if (ddjvu_page_render(*page, DDJVU_RENDER_FOREGROUND,
                              &frect2, &frect2, fmtFg2,
                              fgProbe.bytesPerLine(),
                              reinterpret_cast<char *>(fgProbe.bits()))) {
          const uchar *fgBits = fgProbe.constBits();
          const int total3 = renderW * renderH;
          const int step3  = qMax(1, total3 / 2000);
          for (int px = 0; px < total3 && !fgNeedsMultiply; px += step3) {
            const uchar v = fgBits[px];
            if (v > 10 && v < 245) fgNeedsMultiply = true;
          }
          if (fgNeedsMultiply) bgImgProbe = std::move(fgProbe);
        }
        ddjvu_format_release(fmtFg2);
      }
    }
    QByteArray fgData;
    bool fgIsJbig2 = false;
    int  fgW = renderW, fgH = renderH;
    if (fgNeedsMultiply) {
      // True FGbz-only gray fills: Multiply with RENDER_FOREGROUND gray JPEG.
      fgData = encodeJpegGray(bgImgProbe, jpegQ);
    } else {
      // ImageMask path: FG = RENDER_BLACK JBIG2 at native DPI.
      fgData = encodeJbig2BlackRender(*page, srcW, srcH);
      fgIsJbig2 = true;
      fgW = srcW; fgH = srcH;
    }
    if (!bgData.isEmpty() && !fgData.isEmpty()) {
      QVector<PdfWordZone> words;
      if (ui.textLayerCheckBox->isChecked() && document) {
        miniexp_t textExpr = document->getPageText(pageno);
        if (textExpr != miniexp_dummy && textExpr != miniexp_nil)
          pdfCollectWords(textExpr, words);
      }
      if (logFile_.isOpen()) {
        logFile_.write("page=" + QByteArray::number(pageno + 1)
          + " srcW=" + QByteArray::number(srcW) + " srcH=" + QByteArray::number(srcH)
          + " renderW=" + QByteArray::number(renderW) + " renderH=" + QByteArray::number(renderH)
          + " type=3 gray=" + QByteArray(bgIsGray ? "1" : "0")
          + " bgGray=" + QByteArray(bgHasGrayContent ? "1" : "0")
          + " fgMul=" + QByteArray(fgNeedsMultiply ? "1" : "0")
          + " [" + QByteArray(fgIsJbig2 ? "COMPOUND_JBIG2" : "COMPOUND_MULTIPLY") + "]\n"
          + "  bgSize=" + QByteArray::number(bgData.size())
          + " fgSize=" + QByteArray::number(fgData.size()) + "\n");
        logFile_.flush();
      }
      const bool addOk = rawPdf->addCompoundPage(
        mmW, mmH, bgData, renderW, renderH, bgIsGray,
        fgData, fgW, fgH, fgIsJbig2, words);
      if (!addOk)
        error(tr("Failed to write PDF page %1.").arg(pageno + 1), __FILE__, __LINE__);
      return;
    }
    // fallback: fall through to single-image JPEG path
  }

  if (useGray) {
    QImage gray = qimg.convertToFormat(QImage::Format_Grayscale8);
    if (!gray.isNull()) qimg = std::move(gray);
  }

  const int pageType = (int)ddjvu_page_get_type(*page);
  const bool isBitonal = (pageType == (int)DDJVU_PAGETYPE_BITONAL);
  if (logFile_.isOpen()) {
    QByteArray line = "page=" + QByteArray::number(pageno + 1)
      + " srcW=" + QByteArray::number(srcW)
      + " srcH=" + QByteArray::number(srcH)
      + " imgdpi=" + QByteArray::number(imgdpi)
      + " renderW=" + QByteArray::number(renderW)
      + " renderH=" + QByteArray::number(renderH)
      + " imgW=" + QByteArray::number(qimg.width())
      + " imgH=" + QByteArray::number(qimg.height())
      + " type=" + QByteArray::number(pageType)
      + " gray=" + QByteArray(useGray ? "1" : "0")
      + " bitonal=" + QByteArray(isBitonal ? "1" : "0")
      + " jbig2=" + QByteArray(forceJbig2 ? "1" : "0")
      + " renderOk=" + (renderOk ? "1" : "0")
      + "\n";
    logFile_.write(line);
    logFile_.flush();
  }

  // Collect invisible text words (if requested).
  QVector<PdfWordZone> words;
  if (ui.textLayerCheckBox->isChecked() && document) {
    miniexp_t textExpr = document->getPageText(pageno);
    if (textExpr != miniexp_dummy && textExpr != miniexp_nil)
      pdfCollectWords(textExpr, words);
  }

  // Encode the page image.
  // BITONAL + B&W compound (RENDER_BLACK stencil): JBIG2 — pure 1bpp, no anti-aliasing.
  // Grayscale compound with color bg: gray JPEG.
  // Color pages: RGB JPEG.
  const int jpegQ = qBound(1, ui.jpegQualitySpinBox->value(), 100);
  const bool isGrayImg = (qimg.format() == QImage::Format_Grayscale8);
  QByteArray imgData;
  bool isJbig2 = false;

  if (isGrayImg && forceJbig2) {
    // 1bpp direct render — bypasses RGB24->Gray8->threshold pipeline.
    imgData = encodeJbig2BlackRender(*page, qimg.width(), qimg.height());
    if (!imgData.isEmpty()) {
      isJbig2 = true;
    } else {
      // JBIG2 failed — fall back to grayscale JPEG via libjpeg
      if (logFile_.isOpen()) logFile_.write("  encodeJbig2 FAILED, fallback gray JPEG\n");
      imgData = encodeJpegGray(qimg, jpegQ);
      if (imgData.isEmpty()) {
        // libjpeg failed — fall back to RGB QImageWriter
        if (logFile_.isOpen()) logFile_.write("  encodeJpegGray FAILED, fallback RGB\n");
        QImage rgb = qimg.convertToFormat(QImage::Format_RGB888);
        QBuffer b(&imgData); b.open(QIODevice::WriteOnly);
        QImageWriter w(&b, "JPEG"); w.setQuality(jpegQ); w.write(rgb);
      }
    }
  } else if (isGrayImg) {
    // Grayscale compound/photo — anti-aliased render, use gray JPEG (not JBIG2).
    imgData = encodeJpegGray(qimg, jpegQ);
    if (imgData.isEmpty()) {
      if (logFile_.isOpen()) logFile_.write("  encodeJpegGray FAILED, fallback RGB\n");
      QImage rgb = qimg.convertToFormat(QImage::Format_RGB888);
      QBuffer b(&imgData); b.open(QIODevice::WriteOnly);
      QImageWriter w(&b, "JPEG"); w.setQuality(jpegQ); w.write(rgb);
    }
  } else {
    // Color page — RGB JPEG via Qt plugin.
    QBuffer buf(&imgData);
    buf.open(QIODevice::WriteOnly);
    QImageWriter writer(&buf, "JPEG");
    writer.setQuality(jpegQ);
    if (!writer.write(qimg)) {
      if (logFile_.isOpen())
        logFile_.write("  JPEG encode FAILED: " + writer.errorString().toUtf8() + "\n");
      qWarning("DjView PDF export: page %d JPEG encode failed: %s",
               pageno + 1, qPrintable(writer.errorString()));
      imgData.clear();
    }
  }
  if (imgData.isEmpty()) {
    // Last resort: white page in RGB JPEG
    QImage white(qimg.width(), qimg.height(), QImage::Format_RGB888);
    white.fill(Qt::white);
    QBuffer b2(&imgData); b2.open(QIODevice::WriteOnly);
    QImageWriter w2(&b2, "JPEG"); w2.setQuality(75);
    if (!w2.write(white)) {
      if (logFile_.isOpen()) logFile_.write("  fallback white RGB JPEG FAILED\n");
      error(tr("Failed to write PDF page %1: JPEG plugin unavailable.").arg(pageno + 1),
            __FILE__, __LINE__);
      return;
    }
  }

  if (logFile_.isOpen())
    logFile_.write("  " + QByteArray(isJbig2 ? "jbig2Size=" : "jpegSize=")
                   + QByteArray::number(imgData.size()) + "\n");

  const bool addOk = rawPdf->addPage(mmW, mmH, imgData, qimg.width(), qimg.height(),
                                      isGrayImg, isJbig2, words);
  if (logFile_.isOpen())
    logFile_.write("  addPage=" + QByteArray(addOk ? "OK" : "FAILED") + "\n");
  if (!addOk)
    error(tr("Failed to write PDF page %1.").arg(pageno + 1), __FILE__, __LINE__);
}


// ----------------------------------------
// QDJVIEWIMGEXPORTER



class QDjViewImgExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  QDjViewImgExporter(QDialog *parent, QDjView *djview, 
                     QString name, QByteArray format);
  virtual bool exportOnePageOnly();
  virtual bool save(QString fname);
protected:
  virtual void doPage();
  QString fileName;
  QByteArray format;
};


QDjViewExporter*
QDjViewImgExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  QByteArray format = name.toLocal8Bit();
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  
  if (formats.contains(format))
    return new QDjViewImgExporter(parent, djview, name, format);
  return 0;
}


void
QDjViewImgExporter::setup()
{
  QByteArray format;
  QList<QByteArray> formats = QImageWriter::supportedImageFormats();
  foreach(format, formats)
    {
      QString name = QString::fromLocal8Bit(format);
      QString uname = name.toUpper();
      QString suffix = name.toLower();
      addExporterData(name, suffix,
                      tr("%1 Image", "JPG Image").arg(uname),
                      tr("%1 Files (*.%2)", "JPG Files").arg(uname).arg(suffix),
                      QDjViewImgExporter::create);
    }
}


QDjViewImgExporter::QDjViewImgExporter(QDialog *parent, QDjView *djview, 
                                       QString name, QByteArray format)
  : QDjViewPageExporter(parent, djview, name), 
    format(format)
{
}


bool 
QDjViewImgExporter::exportOnePageOnly()
{
  return true;
}


bool 
QDjViewImgExporter::save(QString fname)
{
  fileName = fname;
  return start();
}


void
QDjViewImgExporter::doPage()
{
  QDjVuPage *page = curPage;
  // determine rectangle
  ddjvu_rect_t rect;
  int imgdpi = ddjvu_page_get_resolution(*page);
  int dpi = imgdpi; // (TODO: add property page with resolution!)
  rect.x = rect.y = 0;
  rect.w = ( ddjvu_page_get_width(*page) * dpi + imgdpi/2 ) / imgdpi;
  rect.h = ( ddjvu_page_get_height(*page) * dpi + imgdpi/2 ) / imgdpi;
  // prepare format
  ddjvu_format_t *fmt = 0;
#if DDJVUAPI_VERSION < 18
  unsigned int masks[3] = { 0xff0000, 0xff00, 0xff };
  fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, masks);
#else
  unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
  fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
#endif
  ddjvu_format_set_row_order(fmt, true);
  ddjvu_format_set_gamma(fmt, 2.2);
  // determine mode (TODO: add property page with mode!)
  ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
  QDjVuWidget::DisplayMode displayMode;
  displayMode = djview->getDjVuWidget()->displayMode();
  if (displayMode == QDjVuWidget::DISPLAY_STENCIL)
    mode = DDJVU_RENDER_BLACK;
  else if (displayMode == QDjVuWidget::DISPLAY_BG)
    mode = DDJVU_RENDER_BACKGROUND;
  else if (displayMode == QDjVuWidget::DISPLAY_FG)
    mode = DDJVU_RENDER_FOREGROUND;
  // compute image
  QString message;
  QImage img(rect.w, rect.h, QImage::Format_RGB32);
  if (! ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                          img.bytesPerLine(), (char*)img.bits() ))
    message = tr("Cannot render page.");
  else
    {
      QFile file(fileName);
      QImageWriter writer(&file, format);
      if (! writer.write(img))
        {
          message = file.errorString();
          file.remove();
          if (writer.error() == QImageWriter::UnsupportedFormatError)
            message = tr("Image format %1 not supported.")
              .arg(QString::fromLocal8Bit(format).toUpper());
#if HAVE_STRERROR
          if (file.error() == QFile::OpenError && errno > 0)
            message = systemErrorString(errno);
#endif
        }
    }
  if (! message.isEmpty())
    error(message, __FILE__, __LINE__);
  if (fmt)
    ddjvu_format_release(fmt);
}




// ----------------------------------------
// QDJVIEWPRNEXPORTER



#include "ui_qdjviewexportprn.h"


class QDjViewPrnExporter : public QDjViewPageExporter
{
  Q_OBJECT
public:
  static QDjViewExporter *create(QDialog*, QDjView*, QString);
  static void setup();
public:
  ~QDjViewPrnExporter();
  QDjViewPrnExporter(QDialog *parent, QDjView *djview, QString name);
  virtual void resetProperties();
  virtual void loadProperties(QString group);
  virtual void saveProperties(QString group);
  virtual bool loadPrintSetup(QPrinter *printer, QPrintDialog *dialog);
  virtual bool savePrintSetup(QPrinter *printer);
  virtual int propertyPages();
  virtual QWidget* propertyPage(int num);
  virtual bool save(QString fileName);
  virtual bool print(QPrinter *printer);
protected:
  virtual void closeFile();
  virtual void doPage();
protected:
  Ui::QDjViewExportPrn ui;
  QPointer<QWidget> page;
  QPrinter *printer;
  QPainter *painter;
  QString   fileName;
};


QDjViewExporter*
QDjViewPrnExporter::create(QDialog *parent, QDjView *djview, QString name)
{
  if (name == "PRN")
    return new QDjViewPrnExporter(parent, djview, name);
  return 0;
}


void
QDjViewPrnExporter::setup()
{
  addExporterData("PRN", "prn",
                  tr("Printer data"),
                  tr("PRN Files (*.prn)"),
                  QDjViewPrnExporter::create);
}


QDjViewPrnExporter::~QDjViewPrnExporter()
{
  closeFile();
  delete page;
}


QDjViewPrnExporter::QDjViewPrnExporter(QDialog *parent, QDjView *djview,
                                         QString name)
  : QDjViewPageExporter(parent, djview, name),
    printer(0),
    painter(0)
{
  page = new QWidget();
  ui.setupUi(page);
  page->setObjectName(tr("Printing Options", "tab caption"));
  resetProperties();
  page->setWhatsThis(tr("<html><b>Printing options.</b><br>"
                        "Option <tt>Color</tt> enables color printing. "
                        "Document pages can be decorated with a frame. "
                        "Option <tt>Scale to fit</tt> accommodates "
                        "whatever paper size your printer uses. "
                        "Zoom factor <tt>100%</tt> reproduces the initial "
                        "document size. Orientation <tt>Automatic</tt> "
                        "chooses portrait or landscape on a page per "
                        "page basis.</html>") );
}


void     
QDjViewPrnExporter::resetProperties()
{
  ui.colorButton->setChecked(true);
  ui.frameCheckBox->setChecked(false);
  ui.cropMarksCheckBox->setChecked(false);
  ui.autoOrientCheckBox->setChecked(true);
  ui.scaleToFitButton->setChecked(true);
  ui.zoomSpinBox->setValue(100);
}


void
QDjViewPrnExporter::loadProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  bool color = s.value("color", true).toBool();
  ui.colorButton->setChecked(color);
  ui.grayScaleButton->setChecked(!color);
  ui.frameCheckBox->setChecked(s.value("frame", false).toBool());
  ui.cropMarksCheckBox->setChecked(false);
  ui.autoOrientCheckBox->setChecked(s.value("autoOrient", true).toBool());
  bool landscape = s.value("landscape",false).toBool();
  ui.portraitButton->setChecked(!landscape);
  ui.landscapeButton->setChecked(landscape);
  int zoom = s.value("zoom",0).toInt();
  ui.scaleToFitButton->setChecked(zoom == 0);
  ui.zoomButton->setChecked(zoom!=0);
  ui.zoomSpinBox->setValue(zoom ? qBound(25,zoom,2400) : 100);
}


void
QDjViewPrnExporter::saveProperties(QString group)
{
  QSettings s;
  if (group.isEmpty()) 
    group = "Export-" + name();
  s.beginGroup(group);
  s.setValue("color", ui.colorButton->isChecked());
  s.setValue("frame", ui.frameCheckBox->isChecked());
  s.setValue("autoOrient", ui.autoOrientCheckBox->isChecked());
  s.setValue("landscape", ui.landscapeButton->isChecked());
  int zoom = ui.zoomSpinBox->value();
  s.setValue("zoom", QVariant(ui.zoomButton->isChecked() ? zoom : 0));
}


bool
QDjViewPrnExporter::loadPrintSetup(QPrinter *printer, QPrintDialog *)
{
  bool grayscale = (printer->colorMode() == QPrinter::GrayScale);
  bool landscape = (printer->pageLayout().orientation() == QPageLayout::Landscape);
  ui.grayScaleButton->setChecked(grayscale);  
  ui.colorButton->setChecked(!grayscale);  
  ui.landscapeButton->setChecked(landscape);
  ui.portraitButton->setChecked(!landscape);
  return true;
}


bool
QDjViewPrnExporter::savePrintSetup(QPrinter *printer)
{
  bool grayscale = ui.grayScaleButton->isChecked();
  bool landscape = ui.landscapeButton->isChecked();
  printer->setColorMode(grayscale ? QPrinter::GrayScale : QPrinter::Color);
  printer->setPageOrientation(landscape ? QPageLayout::Landscape : QPageLayout::Portrait);
  return true;
}


int
QDjViewPrnExporter::propertyPages()
{
  return 1;
}


QWidget* 
QDjViewPrnExporter::propertyPage(int num)
{
  if (num == 0)
    return page;
  return 0;
}


bool
QDjViewPrnExporter::save(QString fileName)
{
  closeFile();
  this->fileName = fileName;
  printer = new QPrinter(QPrinter::HighResolution);
  printer->setOutputFileName(fileName);
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) 
  printer->setOutputFormat(QPrinter::PdfFormat);
#endif
  return start();
}


bool 
QDjViewPrnExporter::print(QPrinter *printer)
{
  closeFile();
  this->printer = printer;
  return start();
}


void 
QDjViewPrnExporter::closeFile()
{
  if (painter)
    delete painter;
  painter = 0;
  if (printer && !fileName.isEmpty())
    delete printer;
  printer = 0;
  if (curStatus > DDJVU_JOB_OK && !fileName.isEmpty())
    ::remove(QFile::encodeName(fileName).data());
  fileName = QString();
}


void 
QDjViewPrnExporter::doPage()
{
  QDjVuPage *page = curPage;
  // determine zoom factor and rectangles
  ddjvu_rect_t rect;
  rect.x = rect.y = 0;
  rect.w = ddjvu_page_get_width(*page);
  rect.h = ddjvu_page_get_height(*page);
  QRect pageRect = printer->pageLayout().paintRectPixels(printer->resolution());
  bool landscape = false;
  int prndpi = printer->resolution();
  int imgdpi = ddjvu_page_get_resolution(*page);
  if (ui.autoOrientCheckBox->isChecked())
    landscape = (rect.w > rect.h) ^ (pageRect.width() > pageRect.height());
  else if (ui.landscapeButton->isChecked())
    landscape = true;
  int printW = (landscape) ? pageRect.height() : pageRect.width();
  int printH = (landscape) ? pageRect.width() : pageRect.height();
  int targetW = (rect.w * prndpi + imgdpi/2 ) / imgdpi;
  int targetH = (rect.h * prndpi + imgdpi/2 ) / imgdpi;
  int zoom = 100;
  if (ui.zoomButton->isChecked())
    zoom = qBound(25, ui.zoomSpinBox->value(), 2400);
  else if (ui.scaleToFitButton->isChecked())
    zoom = qMin( printW * 100 / targetW, printH * 100 / targetH);
  targetW = targetW * zoom / 100;
  targetH = targetH * zoom / 100;
  int dpi = qMin(prndpi * zoom / 100, imgdpi);
  rect.w = ( rect.w * dpi + imgdpi/2 ) / imgdpi;
  rect.h = ( rect.h * dpi + imgdpi/2 ) / imgdpi;
  // create image
  QImage::Format imageFormat = QImage::Format_RGB32;
  if (ddjvu_page_get_type(*page) == DDJVU_PAGETYPE_BITONAL && dpi >= imgdpi)
    imageFormat = QImage::Format_Mono;
  else if (ui.grayScaleButton->isChecked())
    imageFormat = QImage::Format_Indexed8;
  QImage img(rect.w, rect.h, imageFormat);
  // render
  bool rendered = false;
  ddjvu_format_t *fmt = 0;
  ddjvu_render_mode_t mode = DDJVU_RENDER_COLOR;
  QDjViewPrefs *prefs = QDjViewPrefs::instance();
  if (imageFormat == QImage::Format_RGB32)
    {
#if DDJVUAPI_VERSION < 18
      unsigned int masks[3] = { 0xff0000, 0xff00, 0xff };
      fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 3, masks);
#else
      unsigned int masks[4] = { 0xff0000, 0xff00, 0xff, 0xff000000 };
      fmt = ddjvu_format_create(DDJVU_FORMAT_RGBMASK32, 4, masks);
#endif
      if (prefs->printerGamma > 0)
        ddjvu_format_set_gamma(fmt, prefs->printerGamma);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
    }
  else if (imageFormat == QImage::Format_Indexed8)
    {
#if QT_VERSION >= 0x40600
      img.setColorCount(256);
#else
      img.setNumColors(256);
#endif
      for (int i=0; i<256; i++)
        img.setColor(i, qRgb(i,i,i));
      fmt = ddjvu_format_create(DDJVU_FORMAT_GREY8, 0, 0);
      if (prefs->printerGamma > 0)
        ddjvu_format_set_gamma(fmt, prefs->printerGamma);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
    }
  else if (imageFormat == QImage::Format_Mono)
    {
      fmt = ddjvu_format_create(DDJVU_FORMAT_MSBTOLSB, 0, 0);
      ddjvu_format_set_row_order(fmt, true);
      if (ddjvu_page_render(*page, mode, &rect, &rect, fmt,
                              img.bytesPerLine(), (char*)img.bits() ))
        rendered = true;
      if (rendered)
        img.invertPixels();
    }
  // prepare painter
  if (! painter)
    painter = new QPainter(printer);
  else
    printer->newPage();
  // prepare settings
  painter->save();
  if (landscape)
    {
      painter->translate(QPoint(pageRect.width(),0));
      painter->rotate(90);
    }
  painter->translate(qMax(0, (printW-targetW)/2), qMax(0, (printH-targetH)/2));

  // print page
  QRect sourceRect(0,0,rect.w,rect.h);
  QRect targetRect(0,0,targetW,targetH);
  if (rendered)
    painter->drawImage(targetRect, img, sourceRect);
  QPen pen(Qt::black, 0, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin);
  painter->setPen(pen);
  painter->setBrush(Qt::NoBrush);
  if (ui.frameCheckBox->isChecked())
    painter->drawRect(targetRect);
  // finish
  painter->restore();
  if (! rendered)
    {
      QString pageName = djview->pageName(curPage->pageNo());
      error(tr("Cannot render page %1.").arg(pageName), __FILE__, __LINE__);
    }
  if (fmt)
    ddjvu_format_release(fmt);
  fmt = 0;
}






// ----------------------------------------
// CREATEEXPORTERDATA


static void 
createExporterData()
{
  QDjViewDjVuExporter::setup();
  QDjViewPdfTextExporter::setup();   // always available – no TIFF required
#if HAVE_TIFF
  QDjViewTiffExporter::setup();
#endif
  QDjViewPSExporter::setup();
  QDjViewImgExporter::setup();
  QDjViewPrnExporter::setup();
}



// ----------------------------------------
// MOC

#include "qdjviewexporters.moc"





/* -------------------------------------------------------------
   Local Variables:
   c++-font-lock-extra-types: ( "\\sw+_t" "[A-Z]\\sw*[a-z]\\sw*" )
   End:
   ------------------------------------------------------------- */
