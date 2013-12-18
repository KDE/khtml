#include <QtCore/QDebug>
#include <QApplication>
#include <klocalizedstring.h>

#include <QtCore/QString>
#include <QUrl>

#include "java/kjavaappletserver.h"
#include "java/kjavaapplet.h"
#include "java/kjavaappletcontext.h"
#include "java/kjavaappletwidget.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    if (app.arguments().isEmpty()) {
      qWarning() << "you need to specify a path to your kdelibs source dir, see \"--help\"";
      return -1;
    }
    const QString path = app.arguments().first();

    KJavaAppletContext *context = new KJavaAppletContext;
    KJavaAppletWidget *a = new KJavaAppletWidget;
    a->applet()->setAppletContext(context);

    a->show();

//    c->registerApplet(a->applet());

    a->applet()->setBaseURL(QUrl::fromLocalFile(path).toString());
    a->applet()->setAppletName( "Lake" );
    a->applet()->setAppletClass( "lake.class" );
    a->applet()->setParameter( "image", "konqi.gif" );

    a->showApplet();
    a->applet()->start();

    app.exec();
}
