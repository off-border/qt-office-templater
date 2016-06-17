#ifndef TEMPLATER_H
#define TEMPLATER_H

#include <QObject>
#include <QScriptEngine>
#include <QScriptEngineDebugger>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

class Templater : public QObject
{
    Q_OBJECT
public:
    explicit Templater( bool debug_mode = false, QObject *parent = 0);

    void setTemplateFile( QString path );
    void setTmpDir( QString path );

    void    setOutputFile( QString path );
    QString getOutputFile();

    QString getTemplate();

    void setVariable( QString name, QString value );

    void setVariable(QString name, QStringList value);

    bool parseTemplate();

    Q_INVOKABLE QScriptValue requireJS( QString filename );

    Q_INVOKABLE QScriptValue replace_image_file( QScriptValue old_file, QScriptValue new_file );
    Q_INVOKABLE QScriptValue get_new_img_height( QScriptValue img_path, QScriptValue old_height );

    Q_INVOKABLE QString  readTxtFile( QString path );
    Q_INVOKABLE void     writeTxtFile( QString path, QString data );

    Q_INVOKABLE QString  mergeOdtFile( QString filename );

signals:
    void sig_info( QString );
    void sig_warning( QString );
    void sig_error( QString );

public slots:

private:
    bool     removeDir(QString dirName);
    void     cleanDirs();

    QString  unzipFile(QString filename , QString outputDir = "" );
    void     prepareUnpacked( QString path );

    QString  updateRefs2(QString xml , QString prefix);
    void     finishMerging(QString result_path );

    bool     readTemplate();
    QString  cleanTemplate( QString tmpl );
    QString  updateRefs( QString tmpl );
    QString  fixInclusions( QString tmpl );
    void     loadJSEngine();
    void     writeResult();

    QString  getMediaPath( QString upackedPath, QString reference );

    QScriptEngine           engine;
    QScriptEngineDebugger   *debugger;
    QScriptValue            locals;
    QString templatePath;
    QString templateText;
    QString resultPath;
    QString resultText;
    QString tmp_path;

    QMap<QString, QString>  unzippedFiles;
    QMap<QString, QString>  prefixes;

    bool debug;

    enum TemplateTypes { typeOdt, typeDocx, typeUnknown };
    TemplateTypes templateType;

};

#endif // TEMPLATER_H
