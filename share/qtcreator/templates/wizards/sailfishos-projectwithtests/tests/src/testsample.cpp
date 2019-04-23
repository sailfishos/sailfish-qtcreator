#include "testsample.h"

void TestSample::simpleTest() {
    QString str = "Hello";
    QCOMPARE(str.toUpper(), QString("HELLO"));
}
