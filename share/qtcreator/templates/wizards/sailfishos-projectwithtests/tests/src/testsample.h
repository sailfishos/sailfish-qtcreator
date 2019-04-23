#ifndef TESTSAMPLE_H
#define TESTSAMPLE_H

#include <QObject>
#include <QtTest/QtTest>

class TestSample : public QObject
{
    Q_OBJECT
private slots:
    void simpleTest();
};

#endif // TESTSAMPLE_H
