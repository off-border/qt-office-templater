#include "example.h"
#include <QDebug>
#include <QDir>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>

Example::Example(QObject *parent) :
    QObject(parent)
{
    templater = new Templater( true );


   /* because templater displays no error to console output. It uses signals to inform us about things */
    connect( templater, SIGNAL(sig_info(QString)),    this, SLOT(showInfo(QString)) );
    connect( templater, SIGNAL(sig_warning(QString)), this, SLOT(showWarning(QString)) );
    connect( templater, SIGNAL(sig_error(QString)),   this, SLOT(showError(QString)) );


    templater->setTmpDir( QDir().temp().absolutePath() + QDir::separator() + "templater_tmp_dir" );


    QString     simpleVariable = "This Is A Simple Variable";
    QStringList array; array << "This" << "Is" << "An" << "Array";
    QString     includeFile = QFileInfo("../example/template_to_include.odt").absoluteFilePath(); /* swig.js doesn't support relative paths */


   /* setting template variables */
    templater->setVariable( "simple_variable", simpleVariable );
    templater->setVariable( "array", array );
    templater->setVariable( "file_path", includeFile );
    templater->setVariable( "image_file_path","../example/test_image.jpg");


}



void Example::testODT()
{
    templater->setTemplateFile( "../example/template.odt" );
    templater->setOutputFile( QDir::current().absoluteFilePath("./result.odt") );
    templater->parseTemplate();
    QDesktopServices::openUrl(QUrl::fromLocalFile( QFileInfo("./result.odt").absoluteFilePath() ));
}



void Example::testDOCX()
{
    templater->setTemplateFile( "../example/template.docx" );
    templater->setOutputFile( QDir::current().absoluteFilePath("./result.docx") );
    templater->parseTemplate();
    QDesktopServices::openUrl(QUrl::fromLocalFile( QFileInfo("./result.docx").absoluteFilePath() ));
}


void Example::showInfo(QString msg){
    qDebug() << msg;
}

void Example::showWarning(QString msg){
    qDebug() << "Waring: " << msg;
}

void Example::showError(QString msg){
    qDebug() << "ERROR: " << msg;
}


