#include "Utilities/AppStyle.h"

#include <QApplication>
#include <QFile>
#include <QStyleFactory>

void AppStyle::apply(QApplication& app)
{
    app.setStyle(QStyleFactory::create("Fusion"));

    QFile styleFile(":/styles/app.qss");
    if (!styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));
}
