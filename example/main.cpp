#include "example.h"
#include <QApplication>

int main(int argc, char** argv){


    QApplication app( argc, argv );

    Example *example = new Example();


    example->testODT();
    //example->testDOCX();


    exit(0);

    return app.exec();

}
