#include "templater.h"
#include <quazip/JlCompress.h>
#include <QDebug>
#include <QtGui/QPixmap>
#include <QtXml>
#include <QMainWindow>


Templater::Templater(bool debug_mode, QObject *parent) :
    QObject(parent), debug(debug_mode)
{

    if( debug ){
        debugger = new QScriptEngineDebugger();
        debugger->attachTo( &engine );
        debugger->setAutoShowStandardWindow( true );
        //debugger->standardWindow()->show();
    }


    loadJSEngine();

    locals = engine.newObject();

}

void Templater::setTemplateFile(QString path) {
    templatePath = path;
}

void Templater::setOutputFile(QString path) {
    resultPath = path;
}

QString Templater::getOutputFile(){
    return resultPath;
}

void Templater::setTmpDir(QString path) {
    tmp_path = path;
}

QString Templater::getTemplate() {
    return templateText;
}


void Templater::parseTemplate()
{

    cleanDirs();

    QDir().mkpath( tmp_path );

    readTemplate();

    engine.globalObject().setProperty( "tmpl", templateText );

    QScriptValue parms = engine.newObject();
    parms.setProperty ( "locals", locals );
    engine.globalObject().setProperty("parms", parms);

    QScriptValue parsed = engine.evaluate("swig.render( tmpl, parms )" );

    resultText = parsed.toString();

    resultText = fixInclusions( resultText );

    writeTxtFile( tmp_path + "/content.xml", resultText );

    finishMerging( tmp_path );

    writeResult();
}


void Templater::cleanDirs()
{
    foreach( QString dir, unzippedFiles.values() ){
        if( dir.length() > 3 ) //to avoid deleting root directories
            removeDir( dir );
        unzippedFiles.clear();
        prefixes.clear();
    }
}




QString Templater::readTxtFile(QString path)
{
    QFile file( path );
    if( !file.exists() )
        emit sig_error( "readTxtFile: File '" + path + "' not found" );
    file.open( QFile::ReadOnly );
    QTextStream stream( &file );
    stream.setCodec("UTF-8");
    QString text = stream.readAll();
    file.close();
    return text;
}

void Templater::writeTxtFile(QString path, QString data)
{
    QFile file( path );
    file.open( QFile::WriteOnly );
    QTextStream stream( &file );
    stream << data;
    stream.flush();
    file.close();
}


void Templater::readTemplate()
{
    QString unzipped = unzipFile( templatePath, tmp_path );
    prepareUnpacked( unzipped );
    templateText = readTxtFile( unzipped + "/content.xml" );
}


QString Templater::cleanTemplate(QString tmpl)
{
    QRegExp varOpen ("\\{<[^{]+>\\{");
    QRegExp varClose("([^}^%])\\}[^}]+\\}");
    QRegExp varExpr ("\\{\\{\\s*(<[^>]+>\\s*)*(\\w+\\s*)+(<[^>]+>)+(\\s*)");  //

    tmpl = tmpl.replace( varOpen, "{{" );
    tmpl = tmpl.replace( varClose, "\\1}}" );
    while( varExpr.indexIn(tmpl) != -1 )
        tmpl = tmpl.replace( varExpr, "{{\\2\\4");

    QRegExp tagOpen("\\{<[^{]+>%");
    QRegExp tagClose("([^}^{^\\d])%<[^}]+\\}");
    QRegExp tagExpr ("\\{\\%\\s*(<[^>]+>\\s*)*(([^<^\\}])*)(<[^>]+>)+(\\s*)");      // \{\%\s*(<[^>]+>\s*)*(([^<^\}])*)(<[^>]+>)+(\s*)

    tmpl = tmpl.replace( tagOpen, "{% " );
    tmpl = tmpl.replace( tagClose, "\\1 %}" );
    while( tagExpr.indexIn(tmpl) != -1 )
        tmpl = tmpl.replace( tagExpr, "{%\\2\\5");

    tmpl = tmpl.replace("&apos;", "'");

   //fix double xml tags closing
    tmpl = tmpl.replace( QRegExp("(</text:span>\\s*<text:span[^>]+>)([^<]+)(</text:span>)\\s*</text:span>"), "\\1\\2\\3" );

    tmpl = tmpl.replace("{{", "@_{{");

    return tmpl;
}


QString Templater::updateRefs2(QString xml, QString prefix)
{
    xml = xml.replace("#doc_", "_doc_");

    QRegExp reg;
    reg = QRegExp("(ref-name)=\"([^\"^#]+)\"");
    while ( reg.indexIn( xml ) != -1 )
        xml = xml.replace( reg, QString("\\1=\"\\2#%1\"").arg(prefix) );

    reg = QRegExp("(style:name)=\"([^\"^#]+)\"");
    while ( reg.indexIn( xml ) != -1 )
        xml = xml.replace( reg, QString("\\1=\"\\2#%1\"").arg(prefix) );

    reg = QRegExp("(style-name)=\"([^\"^#]+)\"");
    while ( reg.indexIn( xml ) != -1 )
        xml = xml.replace( reg, QString("\\1=\"\\2#%1\"").arg(prefix) );

    reg = QRegExp("(style:page-layout-name)=\"([^\"^#]+)\"");
    while ( reg.indexIn( xml ) != -1 )
        xml = xml.replace( reg, QString("\\1=\"\\2#%1\"").arg(prefix) );

    return xml;
}


//QString Templater::updateRefs(QString tmpl)
//{
//    emit sig_info( " updating refs " );

//    QRegExp ref("\"(_Ref\\d+)\"");

//    qDebug() << "qtime: " << std::rand() << " " << std::rand();

//    while ( ref.indexIn( tmpl ) != -1 ){
//        qDebug() << "refs captured: " << ref.captureCount() << ref.capturedTexts();
//        QString old_ref = ref.capturedTexts().at(1);
//        QString new_ref = "_Ref___";
//        while( new_ref.length() < 18 )
//            new_ref += QString::number(std::rand());
//        new_ref = new_ref.left(17);
//        tmpl = tmpl.replace( old_ref, new_ref );
//    }
//    return tmpl;
//}



QString Templater::fixInclusions(QString tmpl)
{
    QDomDocument doc;
    doc.setContent( tmpl );
    QDomNodeList incs = doc.elementsByTagName("inclusion");

    for( int i = 0; i < incs.length(); i++ ){

        while( incs.at(i).parentNode().nodeName() != "office:text" )
            incs.at(i).parentNode().parentNode().appendChild( incs.at(i) );

        for( int j = 0; j < incs.at(i).childNodes().length(); j++ ){
            incs.at(i).parentNode().appendChild( incs.at(i).childNodes().at(i) );
            j--;
        }

        incs.at(i).parentNode().removeChild( incs.at(i) );

        i--;
    }
    return doc.toString();

}



void Templater::loadJSEngine() {
    engine.globalObject().setProperty( "reporter", engine.newQObject( this ) );

    requireJS("://js/init.js");
}


QScriptValue Templater::requireJS(QString filename) {
    return engine.evaluate( readTxtFile(filename), filename );
}


void Templater::setVariable(QString name, QString value) {
    locals.setProperty( name, value );
}

void Templater::setVariable(QString name, QStringList value) {
    QScriptValue val = qScriptValueFromSequence( &engine, value );
    locals.setProperty( name, val );
}


QScriptValue Templater::replace_image_file( QScriptValue old_file, QScriptValue new_file ) {

    QString of( tmp_path+"/"+old_file.toString() );
    QString nf( new_file.toString().replace("file:///", "") );


    if( !QFileInfo( of ).isFile() ){
        emit sig_error( "cannot find file: " + of );
        return "";
    }
    if( !QFileInfo( nf ).isFile() ){
        emit sig_error( "cannot find file: " + nf );
        return "";
    }

    QFile( of ).remove();
    if( !QFile::copy( nf, of ) ){
        emit sig_error( "cannot replace file " + of + " with file " + of);
    }

    return "";
}


QScriptValue Templater::get_new_img_height(QScriptValue img_path, QScriptValue to_width ) {

    QString file = img_path.toString().replace("file:///","");
    if( !QFile(file).exists() ){
        emit sig_warning( "get_new_img_height: file " + file + " not found" );
        return 0;
    }

    QPixmap img( file );

    if( img.width() == 0 ){
        emit sig_error( "get_new_img_height: image width must be > 0" );
        return 0;
    }

    return static_cast<double>( img.size().height() ) / img.size().width() * to_width.toString().toDouble() ;
}


bool Templater::removeDir(QString dirName) {
    bool result = true;
        QDir dir(dirName);
        if (dir.exists(dirName)) {
            Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
                if (info.isDir())
                    result = removeDir(info.absoluteFilePath());
                else
                    result = QFile::remove(info.absoluteFilePath());
                if (!result)
                    return result;
            }
            result = dir.rmdir(dirName);
        }
        return result;
}



QString Templater::unzipFile(QString filename, QString outputDir ) {
    if( !QFile(filename).exists()){
        emit sig_error("file not found: " + filename);
        return "";
    }


    if( outputDir.length() == 0 )
        outputDir = tmp_path + "_unzipped" + QString::number(unzippedFiles.count());
    if( unzippedFiles.keys().contains(filename) )
        return unzippedFiles.value(filename);

    removeDir( outputDir );

    prefixes.insert( outputDir, "doc_" + QString::number(prefixes.count()) );

    JlCompress::getFileList( filename );

    QStringList extracted = JlCompress::extractDir( filename, outputDir );


    if( extracted.empty() ){
        emit sig_error("unzipFIle: cannot extract archive: " + filename);
        return outputDir;
    }

    unzippedFiles.insert( filename, outputDir );
    return outputDir;
}



QString Templater::mergeOdtFile(QString filename) {
    emit sig_info("merging odt file: " + filename);

    if( !QFile(filename).exists() ){
        emit sig_error("including file not found: " + filename);
        return "FILE_NOT_FOUND";
    }

    QString unzipped = unzipFile( filename );
    prepareUnpacked( unzipped );

   //collect pictures
    QList<QString> pics;
    QDomDocument manifest;
    manifest.setContent( readTxtFile(unzipped + "/META-INF/manifest.xml") );
    QDomNodeList files    = manifest.elementsByTagName("manifest:file-entry");
    for( int i = 0; i < files.length(); i++ ){
        QString pic = files.at(i).toElement().attribute("manifest:full-path");
        if( pic.indexOf("Pictures/") != -1 )
            pics.append( pic );
    }

   //copy pictures
    if( pics.length() > 0 ){
        QDir().mkdir( tmp_path + "/Pictures" );
        foreach( QString pic, pics ){
            QFile::copy( unzipped + "/" + pic, tmp_path + "/" + pic );
        }
    }

    QString content = cleanTemplate( readTxtFile( unzipped + "/content.xml" ) );
    QDomDocument doc;
    if( !doc.setContent(content) )
        emit sig_error("imported template is corrupted");

    QDomNodeList items = doc.elementsByTagName("office:text").at(0).toElement().childNodes();

    QDomElement body_el = doc.createElement("inclusion");
    while( items.length() > 0 )
        body_el.appendChild( items.at(0) );

    QString bodyText;
    QTextStream stream(&bodyText);
    body_el.save( stream, QDomNode::CDATASectionNode );

    return bodyText;
}



void Templater::prepareUnpacked(QString path)
{
    QString text = readTxtFile( path + "/content.xml" );
    text = cleanTemplate( text );
    text = updateRefs2( text, prefixes.value(path) );
    writeTxtFile( path + "/content.xml", text );

    text = readTxtFile( path + "/styles.xml" );
    text = updateRefs2( text, prefixes.value(path) );
    writeTxtFile( path + "/styles.xml", text );

    //qDebug() << " --- preparing: " << path << "\n--- prefix: " << prefixes.value(path);
}



void mergeNodes( QString nodeName, QDomDocument dst, QDomDocument src, QStringList excludeNodes = QStringList() ){
    QDomNode dstNode = dst.elementsByTagName( nodeName ).at(0);
    QDomNode srcNode = src.elementsByTagName( nodeName ).at(0);
    for( int i = 0; i < srcNode.childNodes().length(); i++ ){
        if( !excludeNodes.contains( src.childNodes().at(i).nodeName() ) ){
            dstNode.appendChild( srcNode.childNodes().at(i) );
            i--;
        }
    }
}



void Templater::finishMerging(QString result_path)
{

    foreach( QString source_path, unzippedFiles.values() ){

        if( source_path == result_path )
            continue;

        QDomDocument src, dst; QString file;

        file = "/content.xml";
        dst.setContent( readTxtFile(result_path + file) );
        src.setContent( readTxtFile(source_path + file) );
        mergeNodes( "office:font-face-decls", dst, src );
        mergeNodes( "office:automatic-styles", dst, src );
        writeTxtFile( result_path + file, dst.toString(4) );

        file = "/styles.xml";
        dst.setContent( readTxtFile(result_path + file) );
        src.setContent( readTxtFile(source_path + file) );
        mergeNodes( "office:font-face-decls", dst, src );
        mergeNodes( "office:styles", dst, src, QStringList() << "style:default-style" );
        mergeNodes( "office:automatic-styles", dst, src );
        mergeNodes( "office:master-styles", dst, src );
        writeTxtFile( result_path + file, dst.toString(4) );

        file = "/META-INF/manifest.xml";
        dst.setContent( readTxtFile(result_path + file) );
        src.setContent( readTxtFile(source_path + file) );
        QDomNode     manifest = dst.elementsByTagName("manifest:manifest").at(0);
        QDomNodeList files    = src.elementsByTagName("manifest:file-entry");
        for( int i = 0; i < files.length(); i++ ){
            QString pic = files.at(i).toElement().attribute("manifest:full-path");
            if( pic.indexOf("Pictures/") != -1 ){
                manifest.appendChild( files.at(i) );
                i--;
            }
        }
        writeTxtFile( result_path + file, dst.toString(4) );
    }
}

void Templater::writeResult() {
    JlCompress().compressDir( resultPath, tmp_path );
}


