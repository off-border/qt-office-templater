#include "templater.h"
#include <quazip/JlCompress.h>
#include <QDebug>
#include <QtGui/QPixmap>
#include <QtXml>
#include <QMainWindow>
#include <QDirIterator>



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
    templateType = typeUnknown;
    if( templatePath.endsWith(".odt") )
        templateType = typeOdt;
    if( templatePath.endsWith(".docx") )
        templateType = typeDocx;
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


bool Templater::parseTemplate()
{

    cleanDirs();

    QDir().mkpath( tmp_path );




    if( !readTemplate() )
        return false;

    //qDebug() << "read template: " << templateText;

    engine.globalObject().setProperty( "tmpl", templateText );

    QScriptValue parms = engine.newObject();
    parms.setProperty( "locals", locals );
    engine.globalObject().setProperty("parms", parms);

    QScriptValue parsed = engine.evaluate("swig.render( tmpl, parms )" );

    resultText = parsed.toString();

    //qDebug() << "parsed: " << resultText;


    resultText = fixInclusions( resultText );

    if( templateType == typeOdt )
        writeTxtFile( tmp_path + "/content.xml", resultText );
    if( templateType == typeDocx )
        writeTxtFile( tmp_path + "/word/document.xml", resultText );


    finishMerging( tmp_path );

    writeResult();

    return true;
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


bool Templater::readTemplate()
{
    if( !QFile(templatePath).exists() ){
        emit sig_error("template file not found: " + templatePath );
        return false;
    }

    QString unzipped;
    if( templateType == typeOdt || templateType == typeDocx ){
        unzipped = unzipFile( templatePath, tmp_path );
        prepareUnpacked( unzipped );
    }

    if( templateType == typeOdt )
        templateText = readTxtFile( unzipped + "/content.xml" );
    if( templateType == typeDocx )
        templateText = readTxtFile( unzipped + "/word/document.xml" );
    if( templateType == typeUnknown )
        templateText = readTxtFile( templatePath );

    return true;
}



QString cleanTags( QString str, QString begin, QString end ){



    QRegExp xmlTags("(<[^<]+>\\s*)+");
    QRegExp preservedSpace("(<[^>]+)xml:space=\"preserve\"(/?>)\\s?");

    QString tmp( str );

    QStringList toClean;
    int from = 0, to = 0;
    while( true ){
        from = tmp.indexOf(begin,to);
        if( from < 0 )
            break;
        to   = tmp.indexOf(end, from) + end.length();
        if( to < 0 )
            break;
        QString tag = tmp.mid(from, to-from);
        if( xmlTags.indexIn(tag) != -1 )
        toClean << tag;
    }


    foreach( QString tag, toClean ){

        QString withSpaces(tag);
        while( preservedSpace.indexIn(withSpaces) != -1 ){
            withSpaces.replace( preservedSpace.cap(), preservedSpace.cap(1) + preservedSpace.cap(2) + "$SPACE" );
        }
        withSpaces.replace(xmlTags, "");
        withSpaces.replace("$SPACE", " ");

        qDebug() << "withSpaces: " << withSpaces;
        tmp.replace( tag, withSpaces );
    };
    return tmp;
}


QString Templater::cleanTemplate(QString tmpl)
{
    QRegExp varOpen ("\\{(<[^<]+>\\s*)+\\{");
    QRegExp varClose("\\}(<[^<]+>\\s*)+\\}");

    tmpl.replace( varOpen, "{{" );
    tmpl.replace( varClose, "}}" );
    tmpl = cleanTags( tmpl, "{{", "}}");

    QRegExp tagOpen("\\{<[^<]+>\\s*)+\\%");
    QRegExp tagClose("\\%(<[^<]+>\\s*)+\\}");

    tmpl.replace( tagOpen, "{% " );
    tmpl.replace( tagClose, "%}" );
    tmpl = cleanTags( tmpl, "{%", "%}" );

    tmpl.replace("&apos;", "'");
    tmpl.replace("`","'");

   //fix double xml tags closing
    tmpl.replace( QRegExp("(</text:span>\\s*<text:span[^>]+>)([^<]+)(</text:span>)\\s*</text:span>"), "\\1\\2\\3" );

    tmpl.replace("{{", "@_{{");

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

    QString of, nf;

    if( templateType == typeUnknown ){
        emit sig_error("cannot replace image for unknown template type");
        return "";
    }

    of = getMediaPath( tmp_path, old_file.toString() );

    nf = new_file.toString().replace("file:///", "");

    if( !QFileInfo( of ).isFile() ){
        emit sig_error( "cannot find old image file: " + of );
        return "";
    }
    if( !QFileInfo( nf ).isFile() ){
        emit sig_error( "cannot find new image file: " + nf );
        return "";
    }

    QFile( of ).remove();
    if( !QFile::copy( nf, of ) ){
        emit sig_error( "cannot replace file " + of + " with file " + nf);
    }



    return "";
}


QString Templater::getMediaPath( QString upackedPath, QString reference){
    if( templateType == typeOdt ){
        return upackedPath + "/" + reference;
    }

    if( templateType == typeDocx ){
        QDomDocument doc;
        doc.setContent( readTxtFile( upackedPath + "/word/_rels/document.xml.rels" ) );
        QDomNodeList rels = doc.elementsByTagName("Relationship");
        for( int i = 0; i < rels.length(); i++ )
            if( rels.at(i).toElement().attribute("Id") == reference )
                return upackedPath + "/word/" + rels.at(i).toElement().attribute("Target");
    }

    return upackedPath + "/" + reference;
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

    double height = static_cast<double>( img.size().height() ) / img.size().width() * to_width.toString().toDouble() ;

    if( templateType == typeOdt )
        return height;

    if( templateType == typeDocx )
        return (int)round( height );

    return height;
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
    if( templateType == typeOdt )
    {
        QString text = readTxtFile( path + "/content.xml" );
        text = cleanTemplate( text );
        text = updateRefs2( text, prefixes.value(path) );
        writeTxtFile( path + "/content.xml", text );

        text = readTxtFile( path + "/styles.xml" );
        text = updateRefs2( text, prefixes.value(path) );
        writeTxtFile( path + "/styles.xml", text );
    }

    if( templateType == typeDocx )
    {
        QString text = readTxtFile( path + "/word/document.xml" );
        text = cleanTemplate( text );
        //text = updateRefs2( text, prefixes.value(path) );
        writeTxtFile( path + "/word/document.xml", text );

    }
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

        if( templateType == typeOdt ) {
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

        if( templateType == typeDocx ) {
            emit sig_warning("finishMerging: not implemented for .docx");
        }
    }
}



void Templater::writeResult() {
    JlCompress().compressDir( resultPath, tmp_path, true );

   /* append word/_rels/.rels to archive.
    * JICompress dont want to work correctly with filters
    * to add hidden files */
    if( templateType == typeDocx ){

        QuaZip zip( resultPath );
        QuaZipFile outZipFile(&zip);

        QFile inFile( tmp_path + "/_rels/.rels" );

        if( !zip.open( QuaZip::mdAdd ) ){
            emit sig_error( "cannot open file: " + resultPath );
            return;
        }

        if( !inFile.open(QIODevice::ReadOnly) ){
            emit sig_error( "cannot open file: " + tmp_path + "/_rels/.rels" );
            return;
        }

        if( !outZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo("_rels/.rels", tmp_path + "/_rels/.rels")) ){
            emit sig_error("cannot open outZipFile to append .rels");
            return;
        }

        QByteArray buffer;
        buffer = inFile.read( 256 );
        while( !buffer.isEmpty() ){
            outZipFile.write( buffer );
            buffer = inFile.read( 256 );
        }

        outZipFile.close();

        zip.close();
    }





}


