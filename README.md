# qt-office-templater
## An ODT files templating engine for QT

The swig.js and quazip based office documents templating engine for QT(4-5). In current version it supports an ODT files only. But some DOCX basic support will come soon (without subtemplates inclusion, I think)


### What works: 

##### Insertion of simple text variable:
```
there will be a {{ variable }} inserted.
```

#####Conditions:

```
 {% if variable %}
   insert something 
 {% else %}
   insert another
 {% endif %}
```

##### Loops:

```
{% for item_1 in array_variable %}
   so some output with the {{ item_1 }}
{% endfor %}
```

##### Including of subtemplates

```
  Somwhere in document we can insert an another document: 
  {% include path_to_document_variable %}
  or
  {% include '/path/to/document.odt' %} 
```
but i haven't tested the simple .txt files including.

Note: The path to included document must be absolute. I used the browser version of Swig.js, so it doesn't support relative paths for including. Maybe I'll fix it later.


### Insertion of images (replacement, actually)

There are no ways to insert images in ODT XML file via Swig.js's basic functionality, because of format complexity and need of change some other files in the document's archive. So i've found the decision with replacement the real image file in the 'Pictures' folder in archive, and with changing the image size in content.xml to keep the width of an original inserted image and with fitting the height to keep aspect ratio of the new image.

You can do this replacement using tag {% img img_file_path_variable %} ... {% endimg %}:

```
some document text
{% img path_to_new_image %}
  [there are must be an inserted image which is to be replaced with the new one]
{% endimg %}
document continues
```


### Example

```C++


    Templater *templater = new Templater( true );  //'true' is for enabling js debug on errors

    templater->setTmpDir( QDir().temp().absolutePath() + QDir::separator() + "templater_tmp_dir" );
    templater->setTemplateFile( "../example/template.odt" );
    templater->setOutputFile( QDir::current().absoluteFilePath("./result.odt") );



    QString     simpleVariable = "This Is A Simple Variable";
    QStringList array; array << "This" << "Is" << "An" << "Array";
    QString     includeFile = QFileInfo("../example/template_to_include.odt").absoluteFilePath(); /* swig.js doesn't support relative paths */


   /* setting template variables */
    templater->setVariable( "simple_variable", simpleVariable );
    templater->setVariable( "array", array );
    templater->setVariable( "file_path", includeFile );
    templater->setVariable( "image_file_path","../example/test_image.jpg");

    templater->parseTemplate();
    ```
    
