/** -*- mode: c++ ; c-basic-offset: 3 -*-
 * @file   MainWindow.cpp
 * @author Sebastien Fourey
 * @date   July 2010
 * @brief  Declaration of the class MainWindow
 * 
 * This file is part of the ZArt software's source code.
 * 
 * Copyright Sebastien Fourey / GREYC Ensicaen (2010-...) 
 * 
 *                    http://www.greyc.ensicaen.fr/~seb/
 * 
 * This software is a computer program whose purpose is to demonstrate
 * the possibilities of the GMIC image processing language by offering the
 * choice of several manipulations on a video stream aquired from a webcam. In
 * other words, ZArt is a GUI for G'MIC real-time manipulations on the output
 * of a webcam.
 * 
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use, 
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info". See also the directory "Licence" which comes
 * with this source code for the full text of the CeCILL licence. 
 * 
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability. 
 * 
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or 
 * data to be ensured and,  more generally, to use and operate it in the 
 * same conditions as regards security. 
 * 
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms. 
 */

#include <QtXml>
#include <QFileDialog>
#include <QImageWriter>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>
#include <QDesktopServices>
#include <QShortcut>

#include "DialogAbout.h"
#include "DialogLicence.h"
#include "ImageConverter.h"
#include "ImageView.h"
#include "MainWindow.h"
#include "WebcamGrabber.h"
#include "Settings.h"
#include "FilterThread.h"

MainWindow::MainWindow( QWidget * parent )
   : QMainWindow( parent )
{
   setupUi(this);
   setWindowTitle( QString("%1 %2")
		   .arg(QApplication::applicationName())
		   .arg(QApplication::applicationVersion()) );

#if QT_VERSION > 0x040600
   _tbPlay->setIcon( QIcon::fromTheme("media-playback-start",QIcon(":/images/media-playback-start.png")) );
   _tbZoomOriginal->setIcon( QIcon::fromTheme("zoom-original", QIcon(":/images/zoom-original.png") ));
   _tbZoomFit->setIcon( QIcon::fromTheme("zoom-fit-best", QIcon(":/images/zoom-fit-best.png") ));
#else
   _tbPlay->setIcon( QIcon(":/images/media-playback-start.png") );
   _tbZoomOriginal->setIcon( QIcon(":/images/zoom-original.png") );
   _tbZoomFit->setIcon( QIcon(":/images/zoom-fit-best.png") );
#endif

   // Menu and actions
   QMenu * menu;
   menu = menuBar()->addMenu( tr("&File") );

   QAction * action = new QAction( tr("&Save presets..."), this );
   action->setShortcut( QKeySequence::SaveAs );
#if QT_VERSION > 0x040600
   action->setIcon( QIcon::fromTheme( "document-save-as" ) );
#endif
   menu->addAction( action );
   connect( action, SIGNAL( triggered() ),
  	   this, SLOT( savePresetsFile() ) );

   action = new QAction( tr("&Quit"), this );
   action->setShortcut( QKeySequence::Quit );
#if QT_VERSION > 0x040600
   action->setIcon( QIcon::fromTheme( "application-exit", QIcon(":/images/application-exit.png") ) );
#else
   action->setIcon( QIcon(":/images/application-exit.png") );
#endif
   connect( action, SIGNAL( triggered() ),
  	   qApp, SLOT( closeAllWindows() ) );
   menu->addAction( action );

   QMenu * webcamMenu;
   QStringList args = qApp->arguments();
   if ( args.size() == 3 && args[1] == "--cam" ) {
      int index = args[2].toInt();
      _webcam.setCameraIndex( index );
   } else {
      // Find available cameras, and setup the default one
      QList<int> cameras = WebcamGrabber::getWebcamList();
      if ( cameras.size() > 1 ) {
         webcamMenu = menuBar()->addMenu( tr("&Webcam") );
         QActionGroup * actionGroup = new QActionGroup(this);
         actionGroup->setExclusive(true);
         QAction * action;
         QList<int>::iterator it = cameras.begin();
         while (it != cameras.end()) {
            action = new QAction( QString("Webcam %1").arg(*it), this );
            action->setData( QVariant( *it ) );
            action->setCheckable( true );
            webcamMenu->addAction( action );
            actionGroup->addAction( action );
            ++it;
         }
         actionGroup->actions()[0]->setChecked(true);
         _webcam.setCameraIndex( actionGroup->actions()[0]->data().toInt() );
         connect( actionGroup, SIGNAL( triggered( QAction * ) ),
                 this, SLOT( onWebcamSelected( QAction * ) ) );
      } else if ( cameras.size() == 1 ) {
         _webcam.setCameraIndex( cameras[0] );
      }
   }

   QSize cameraSize( _webcam.width(), _webcam.height() );
   _imageView->resize( cameraSize );

   // Network manager
   _networkManager = new QNetworkAccessManager(this);
   connect( _networkManager, SIGNAL(finished(QNetworkReply*)),
	   this, SLOT(networkReplyFinished(QNetworkReply*)) );

   // Options menu
   menu = menuBar()->addMenu( tr("&Options") );

   action = new QAction(tr("Show right panel"),this);
   action->setCheckable(true);
   action->setChecked(true);
   action->setShortcut(QKeySequence("Ctrl+F"));
   connect( action, SIGNAL(triggered(bool)),
           this, SLOT(onRightPanel(bool)));
   menu->addAction(action);

   menu->addSeparator();
   action = new QAction( tr("&Get online presets"), this );
   action->setCheckable( true );
   connect( action, SIGNAL( toggled(bool ) ),
	   this, SLOT( onGetOnlinePresets(bool ) ) );
   menu->addAction( action );

   // Get online presets if configured.
   onGetOnlinePresets( globalSettings.value( "GetOnlinePresets", false ).toBool() );

   action = new QAction( tr("&Set preset file..."), this );
#if QT_VERSION > 0x040600
   action->setIcon( QIcon::fromTheme( "document-open", QIcon(":/images/document-open.png") ) );
#else
   action->setIcon( QIcon(":/images/document-open.png") );
#endif
   connect( action, SIGNAL( triggered() ),
	   this, SLOT( setPresetsFile() ) );
   menu->addAction( action );

   // Load presets file if configured.
   if ( !globalSettings.value( "GetOnlinePresets", false ).toBool()
         && !globalSettings.value( "PresetsFile", QString() ).toString().isEmpty() ) {
      QFile presetsTreeFile( globalSettings.value( "PresetsFile", QString() ).toString() );
      QString error;
      presetsTreeFile.open( QIODevice::ReadOnly );
      _presets.setContent( &presetsTreeFile, false, &error );
      presetsTreeFile.close();
      _treeGPresets->clear();
      addPresets( _presets.elementsByTagName("document").at(0).toElement(), 0 );
   }

   action = new QAction( tr("&Use built-in presets"), this );
   connect( action, SIGNAL( triggered() ),
	   this, SLOT( onUseBuiltinPresets() ) );
   menu->addAction( action );
   if ( ! globalSettings.value("GetOnlinePresets", false ).toBool()
         && globalSettings.value( "PresetsFile", QString() ).toString().isEmpty() )
      onUseBuiltinPresets();

   menu = menuBar()->addMenu( tr("&Help") );

   action = new QAction( tr("&Visit G'MIC website"), this );
   connect( action, SIGNAL( triggered() ),
           this, SLOT( visitGMIC() ) );
   menu->addAction( action );

   action = new QAction( tr("&Licence..."), this );
   connect( action, SIGNAL( triggered() ),
           this, SLOT( licence() ) );
   menu->addAction( action );

   action = new QAction( tr("&About..."), this );
   connect( action, SIGNAL( triggered() ),
           this, SLOT( about() ) );
   menu->addAction( action );

   _imageView->QWidget::resize( _webcam.width(), _webcam.height() );

   _bgFilterChoice = new QButtonGroup(this);
   _bgFilterChoice->addButton( _rbGMIC );
   _bgFilterChoice->addButton( _rbOpenCV );
   _rbGMIC->setChecked( true );


   _bgZoom = new QButtonGroup(this);
   _bgZoom->setExclusive(true);
   _tbZoomFit->setCheckable(true);
   _tbZoomFit->setChecked(true);
   _tbZoomOriginal->setCheckable(true);
   _bgZoom->addButton(_tbZoomOriginal);
   _bgZoom->addButton(_tbZoomFit);

   _cbPreviewMode->addItem("Full",FilterThread::Full);
   _cbPreviewMode->addItem("Top",FilterThread::TopHalf);
   _cbPreviewMode->addItem("Left",FilterThread::LeftHalf);
   _cbPreviewMode->addItem("Bottom",FilterThread::BottomHalf);
   _cbPreviewMode->addItem("Right",FilterThread::RightHalf);
   _cbPreviewMode->addItem("Camera",FilterThread::Camera);

   connect( _cbPreviewMode, SIGNAL(activated(int)),
            this, SLOT(onPreviewModeChanged(int)));

   QShortcut * sc = new QShortcut(QKeySequence("Ctrl+P"),this);
   _tbPlay->setToolTip(tr("Launch processing (Ctrl+P)"));
   connect( sc, SIGNAL(activated()),
           this, SLOT(onPlay()) );
   connect( _tbPlay, SIGNAL(clicked()),
           this, SLOT(onPlay()));

   connect( _tbZoomOriginal, SIGNAL( clicked() ),
           _imageView, SLOT( zoomOriginal() ) );

   connect( _tbZoomFit, SIGNAL( clicked() ),
           _imageView, SLOT( zoomFitBest() ) );

   connect( _pbApply, SIGNAL( clicked() ),
	   this, SLOT( commandModified() ) );

   connect( _tbCamera, SIGNAL( clicked() ),
	   this, SLOT( snapshot() ) );

   _imageView->setMouseTracking( true );

   connect( _imageView, SIGNAL( mousePress( QMouseEvent * ) ),
           this, SLOT( imageViewMouseEvent( QMouseEvent * ) ) );

   connect( _imageView, SIGNAL( mouseMove( QMouseEvent * ) ),
           this, SLOT( imageViewMouseEvent( QMouseEvent * ) ) );

   connect( _rbGMIC, SIGNAL( toggled(bool ) ),
           this, SLOT( onGMICFilterModeChoice(bool) ) );

   connect( _treeGPresets, SIGNAL( itemClicked( QTreeWidgetItem *, int )),
	   this, SLOT( presetClicked( QTreeWidgetItem *, int ) ) );

   connect( _treeGPresets, SIGNAL( itemDoubleClicked( QTreeWidgetItem *, int )),
	   this, SLOT( presetDoubleClicked( QTreeWidgetItem *, int ) ) );

   connect( _commandEditor, SIGNAL( commandModified() ),
	   this, SLOT( commandModified() ) );

   _sliderSkipFrames->setRange(0,10);
   connect( _sliderSkipFrames, SIGNAL( valueChanged(int) ),
	   this, SLOT( setFrameSkip(int ) ) );

   // Image filter for the "Save as..." dialog
   QList<QByteArray> formats = QImageWriter::supportedImageFormats();
   QList<QByteArray>::iterator it = formats.begin();
   QList<QByteArray>::iterator end = formats.end();
   _imageFilters = "Image file (";
   while ( it != end ) {
      _imageFilters += QString("*.") + QString(*it).toLower() + " ";
      ++it;
   }
   _imageFilters.chop(1);
   _imageFilters += ")";

   // Cascade files combobox
   QStringList files;
   _filtersPath = "/usr/share/zart";
   files = QDir(_filtersPath,"haarcascade_*.xml").entryList();
   if ( ! files.size() ) {
      _filtersPath = QFileInfo(qApp->arguments()[0]).absolutePath();
      files = QDir(_filtersPath,"haarcascade_*.xml").entryList();
   }
   if ( files.size() ) {
      QStringList::iterator itFile = files.begin();
      while ( itFile != files.end() ) {
         _cbCascades->addItem( (*itFile++).replace(".xml","").replace("haarcascade_","") );
      }
      connect( _cbCascades, SIGNAL( currentIndexChanged( const QString & ) ),
              this, SLOT( onCascadeChanged( const QString & ) ) );
   } else {
      _rbOpenCV->setEnabled(false);
      _cbCascades->addItem(tr("No xml files found"));
      _cbCascades->setEnabled(false);
   }

   _filterThread = 0;

   readSettings();
}

MainWindow::~MainWindow()
{ 
   _filterThread->stop();
   _filterThread->wait();
   delete _filterThread;
}

void
MainWindow::readSettings()
{
   QSettings settings;
   settings.beginGroup("snapshot");
   _currentDir = settings.value("path", QDir::homePath()).toString();
   settings.endGroup();
}

void
MainWindow::writeSettings()
{
   QSettings settings;
   settings.beginGroup("snapshot");
   settings.setValue("path", _currentDir);
   settings.endGroup();
}

void
MainWindow::closeEvent( QCloseEvent *event )
{
   writeSettings();
   event->accept();
}

void
MainWindow::addPresets( const QDomElement & domE, QTreeWidgetItem * parent )
{
   for( QDomNode n = domE.firstChild(); !n.isNull(); n = n.nextSibling() ) {
      QString name = n.attributes().namedItem( "name" ).nodeValue();
      if ( n.nodeName() == QString("preset") ) {
         QString text = n.firstChild().toText().data();
         QStringList strList;
         strList << name;
         if ( ! parent )
            _treeGPresets->addTopLevelItem( new QTreeWidgetItem( strList ) );
         else
            new QTreeWidgetItem( parent, strList );
      } else if ( n.nodeName() == QString("preset_group") ) {
         QStringList strList;
         strList << name;
         QTreeWidgetItem * parent;
         _treeGPresets->addTopLevelItem( parent = new QTreeWidgetItem( strList ) );
         addPresets( n.toElement(), parent );
      }
   }
}

void
MainWindow::about()
{
   DialogAbout * d = new DialogAbout( this );
   d->exec();
   delete d;
}

void
MainWindow::visitGMIC()
{
   QDesktopServices::openUrl(QUrl("http://gmic.sourceforge.net/"));
}

void
MainWindow::licence()
{
   DialogLicence * d = new DialogLicence( this );
   d->exec();
   delete d;
}

QString
MainWindow::getPreset( const QString & name )
{
   QDomNodeList list =  _presets.elementsByTagName("preset");
   for ( int i = 0; i < list.count(); ++i ) {
      QDomNode n = list.at(i);
      if ( n.attributes().namedItem( "name" ).nodeValue() == name ) {
         return n.firstChild().toText().data().trimmed();
      }
   }
   return QString();
}

void
MainWindow::onImageAvailable()
{
   _imageView->checkSize();
   _imageView->repaint();
}

void
MainWindow::play()
{
   int pm = _cbPreviewMode->itemData(_cbPreviewMode->currentIndex()).toInt();
   FilterThread::PreviewMode  previewMode = static_cast<FilterThread::PreviewMode>(pm);

   _filterThread = new FilterThread( _webcam,
                                    _commandEditor->toPlainText(),
                                    &_imageView->image(),
                                    &_imageView->imageMutex(),
                                    _rbGMIC->isChecked() ? FilterThread::GMIC_Mode : FilterThread::FaceDetection_Mode,
                                    previewMode,
                                    QString("%1%2haarcascade_%3.xml").arg(_filtersPath).arg(QDir::separator()).arg( _cbCascades->currentText() ),
                                    _sliderSkipFrames->value() );

   connect( _filterThread, SIGNAL( imageAvailable() ),
           this, SLOT( onImageAvailable() ) );

   _filterThread->start();
}

void
MainWindow::stop()
{ 
   if ( _filterThread ) {
      _filterThread->stop();
      _filterThread->wait();
      delete _filterThread;
      _filterThread = 0;
   }
}

void
MainWindow::onPlay()
{
  if ( _filterThread ) {
     stop();
#if QT_VERSION > 0x040600
   _tbPlay->setIcon( QIcon::fromTheme("media-playback-start",QIcon(":/images/media-playback-start.png")) );
#else
   _tbPlay->setIcon( QIcon(":/images/media-playback-start.png") );
#endif
   _tbPlay->setToolTip(tr("Launch processing (Ctrl+P)"));
  } else {
     play();
#if QT_VERSION > 0x040600
   _tbPlay->setIcon( QIcon::fromTheme("media-playback-stop",QIcon(":/images/media-playback-stop.png")) );
#else
   _tbPlay->setIcon( QIcon(":/images/media-playback-stop.png") );
#endif
   _tbPlay->setToolTip(tr("Stop processing (Ctrl+P)"));
  }
}

void
MainWindow::imageViewMouseEvent( QMouseEvent * e )
{
   int buttons = 0;
   if ( e->buttons() & Qt::LeftButton ) buttons |= 1;
   if ( e->buttons() & Qt::RightButton ) buttons |= 2;
   if ( e->buttons() & Qt::MidButton ) buttons |= 4;
   if (_filterThread)
      _filterThread->setMousePosition( e->x(), e->y(), buttons );
}

void
MainWindow::commandModified()
{
   if ( _filterThread && _filterThread->isRunning() && _rbGMIC->isChecked()) {
      stop();
      play();
   }
}

void
MainWindow::presetClicked( QTreeWidgetItem * item, int  )
{
   if ( item->childCount() ) return;
   _commandEditor->setPlainText( getPreset( item->text(0) ) );
}

void
MainWindow::presetDoubleClicked( QTreeWidgetItem * item, int  )
{
   if ( item->childCount() ) return;
   _commandEditor->setPlainText( getPreset( item->text(0) ) );
   if ( ! _rbGMIC->isChecked() )
      _rbGMIC->setChecked( true );
   if ( _cbPreviewMode->currentText().startsWith("Camera") ) {
      _cbPreviewMode->setCurrentIndex(0);
      onPreviewModeChanged(0);
   }
   commandModified();
}

void
MainWindow::snapshot()
{
   if ( _filterThread )
      _tbPlay->click();
   QString filename = QFileDialog::getSaveFileName( this,
						    tr("Save image as..."),
						    _currentDir,
						    _imageFilters,
						    0,
						    0 );
   if ( ! filename.isEmpty() ) {
      QFileInfo info( filename );
      _currentDir = info.filePath();
      QImageWriter writer( filename );
      _imageView->imageMutex().lock();
      writer.write( _imageView->image() );
      _imageView->imageMutex().unlock();
   }
}

void
MainWindow::setFrameSkip(int i)
{
   _labelSkipFrames->setText( QString(tr("Frame skip (%1)")).arg(i) );
   if ( _filterThread )
      _filterThread->setFrameSkip( i );
}

void
MainWindow::onWebcamSelected( QAction * action )
{
   int index = action->data().toInt();
   if ( action->isChecked() ) {
      if ( _filterThread && _filterThread->isRunning() ) {
         stop();
         _webcam.setCameraIndex( index );
         play();
      } else {
         _webcam.setCameraIndex( index );
      }
   }
}

void 
MainWindow::onGetOnlinePresets( bool on )
{
   // globalSettings.setValue( "GetOnlinePresets", on );
   if ( on ) {
      QNetworkRequest request( QUrl("http://www.greyc.ensicaen.fr/~seb/zart_presets.xml" ) );
      _networkManager->get( request );
   } else {
      globalSettings.setValue( "GetOnlinePresets", false );
   }
}

void
MainWindow::networkReplyFinished( QNetworkReply* reply )
{
   if ( reply->error() != QNetworkReply::NoError ) {
      QMessageBox::critical( this,
			     tr("Network Error"),
			     tr("Could not retreive the preset file from"
				" the Web. Maybe a problem with your network"
				" connection.") );
      return;
   }
   globalSettings.setValue( "GetOnlinePresets", true );
   QString error;
   _presets.setContent( reply, false, &error );
   _treeGPresets->clear();
   addPresets( _presets.elementsByTagName("document").at(0).toElement(), 0 );
}

void
MainWindow::setPresetsFile()
{
   QString filename = QFileDialog::getOpenFileName( this,
						    tr("Open a presets file"),
						    QDir::currentPath(),
						    tr("Preset files (*.xml)") );
   if ( ! filename.isEmpty() ) {
      globalSettings.setValue( "PresetsFile", filename );
      QFile presetsTreeFile( filename );
      QString error;
      presetsTreeFile.open( QIODevice::ReadOnly );
      _presets.setContent( &presetsTreeFile, false, &error );
      presetsTreeFile.close();
      _treeGPresets->clear();
      addPresets( _presets.elementsByTagName("document").at(0).toElement(),
                 0 );
      globalSettings.setValue("GetOnlinePresets", false );
   }
}

void
MainWindow::savePresetsFile()
{
   QString filename = QFileDialog::getSaveFileName( this,
						    tr("Save presets file"),
						    QDir::currentPath(),
						    tr("Preset files (*.xml)") );
   if ( ! filename.isEmpty() ) {
      QFile presetsFile( filename );
      presetsFile.open( QIODevice::WriteOnly );
      presetsFile.write( _presets.toByteArray() );
      presetsFile.close();
      globalSettings.setValue("GetOnlinePresets", false );
   }
}

void 
MainWindow::onUseBuiltinPresets()
{
   QFile presetsTreeFile( ":/presets.xml" );
   QString error;
   presetsTreeFile.open( QIODevice::ReadOnly );
   _presets.setContent( &presetsTreeFile, false, &error );
   presetsTreeFile.close();
   _treeGPresets->clear();
   addPresets( _presets.elementsByTagName("document").at(0).toElement(),
	      0 );
   globalSettings.setValue( "PresetsFile", QString() );
   globalSettings.setValue( "GetOnlinePresets", false );
}

void
MainWindow::onGMICFilterModeChoice( bool )
{
   if ( _filterThread && _filterThread->isRunning() ) {
      stop();
      play();
   }
}

void
MainWindow::onCascadeChanged( const QString &  )
{
   if ( _filterThread && _filterThread->isRunning() ) {
      stop();
      play();
   }
}

void
MainWindow::onPreviewModeChanged( int index )
{
   int mode = _cbPreviewMode->itemData(index).toInt();
   if ( _filterThread )
      _filterThread->setPreviewMode(static_cast<FilterThread::PreviewMode>(mode));
}

void
MainWindow::onRightPanel( bool on )
{
   if ( on && !_rightPanel->isVisible()) {
      _rightPanel->show();
      return;
   }
   if ( !on && _rightPanel->isVisible()) {
      _rightPanel->hide();
      return;
   }
}

