#ifndef SIGNALSRECEIVER_H
#define SIGNALSRECEIVER_H

#include <QObject>
#include "../templater.h"

class Example : public QObject
{
    Q_OBJECT
public:
    explicit Example(QObject *parent = 0);

    //void run();

public slots:
    void showInfo( QString );
    void showWarning( QString );
    void showError( QString );

private:
    Templater *templater;


};

#endif // SIGNALSRECEIVER_H
