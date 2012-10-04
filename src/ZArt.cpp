/** -*- mode: c++ ; c-basic-offset: 3 -*-
 * @file   ZArt.cpp
 * @author Sebastien Fourey
 * @date   July 2010
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

#include <QApplication>
#include <QMessageBox>
#include <QTextStream>
#include "WebcamGrabber.h"
#include "Common.h"
#include "MainWindow.h"

int main( int argc, char *argv[] )
{
  QApplication app( argc, argv );
  QApplication::setWindowIcon( QIcon(":images/icon.png") );
  QApplication::setApplicationName( ZART_APPLICATION_NAME );
  QApplication::setApplicationVersion( ZART_APPLICATION_VERSION );

  // Parse command line arguments
  QStringList args = app.arguments();
  bool helpFlag = false;;
  bool versionFlag = false;
  if (args.contains("-h") || args.contains("--help"))
    helpFlag = true;
  else if (args.contains("--version"))
    versionFlag = true;

  if (helpFlag) {
     QTextStream out(stdout);
     out << "Usage: " << QApplication::applicationName() << "[OPTION]" << endl
	 << "Options:" << endl
	 << "    " << "-h, --help : print this help." << endl
	 << "    " << "--version  : print the version of the application." << endl
	 << "    " << "--cam N    : disable camera detection and force selection of" << endl
	 << "                 camera with index N." << endl
	 << " " << QApplication::applicationVersion()
	 << endl;
     return 0;
  }
  else if (versionFlag) {
     QTextStream out(stdout);
     out << QApplication::applicationName()
	 << " " << QApplication::applicationVersion()
	 << endl;
     return 0;
  }

  if ( ! WebcamGrabber::getWebcamList().count() ) {
     QMessageBox::critical(0, QString("%1 %2").arg(QApplication::applicationVersion()).arg(QApplication::applicationVersion()),
			   "No webcam found.<br/><br/>(ZArt is useless without a webcam!)");
     exit(EXIT_FAILURE);
  }

  // Localization
  QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8")) ;
  QDir translationDirectory;
  QString translationFilename = QString("zart_%1.qm").arg(QLocale::system().name().split('_').first());
  QString directory;

  if (translationDirectory.exists())
     directory = translationDirectory.absoluteFilePath("lang");
  else
     directory = QDir::current().absoluteFilePath("lang");

  QTranslator translator;
  translator.load(translationFilename, directory);
  app.installTranslator(&translator);

  MainWindow *mainWindow = new MainWindow;
  mainWindow->show();
  return app.exec();
}
