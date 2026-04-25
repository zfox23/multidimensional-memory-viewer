#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include "ui/AppController.h"
#include "ui/MdmImageProvider.h"

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Multidimensional Memory Viewer"));
    app.setOrganizationName(QStringLiteral("Zach Fox"));
    app.setOrganizationDomain(QStringLiteral("zachfox.com"));

    AppController controller;

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("mdmthumbnail"), new MdmThumbnailProvider());
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);

    const QUrl url(QStringLiteral("qrc:/qt/qml/MMV/qml/Main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
