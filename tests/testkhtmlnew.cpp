// program to test the new khtml implementation

#include "testkhtmlnew.h"

#include <QAction>
#include <QCursor>
#include <QBoxLayout>
#include <QFrame>
#include <QToolButton>
#include <QComboBox>
#include <QLineEdit>
#include <QMovie>
#include <QLabel>
#include <QImage>
#include <QtXml/QDomDocument>
#include <QApplication>

#include <kxmlguifactory.h>
#include <kxmlguiwindow.h>
#include <ktoggleaction.h>
#include <kstandardaction.h>
#include <kactioncollection.h>

#include "css/cssstyleselector.h"

#include "dom/dom_string.h"
#include "dom/dom2_range.h"
#include "dom/html_document.h"
#include "dom/dom_exception.h"

#include "html/htmltokenizer.h"
#include "html/html_imageimpl.h"

#include "misc/loader.h"

#include "rendering/render_style.h"

#include "khtml_part.h"
#include "khtml_global.h"

TestKHTML::TestKHTML()
    : KXmlGuiWindow()
{
#ifndef __KDE_HAVE_GCC_VISIBILITY
    m_factory = new KHTMLGlobal();
#endif

    m_movie = new QMovie(":/pics/progress.gif");
    m_indicator = new QLabel;
    m_indicator->setFixedSize(QSize(24, 24));
    m_indicator->setMovie(m_movie);

    m_part = new KHTMLPart( this, this, KHTMLPart::BrowserViewGUI );
    connect( m_part->browserExtension(), SIGNAL(openUrlRequest(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
             this, SLOT(openUrl(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));

    m_combo = new QComboBox;
    m_combo->setEditable(true);
    connect(m_combo, SIGNAL(returnPressed(QString)), this, SLOT(openUrl(QString)));

    m_goButton = new QToolButton;
    m_goButton->setText("Go");
    connect(m_goButton, SIGNAL(clicked()), this, SLOT(openUrl()));

    m_reloadButton = new QToolButton;
    m_reloadButton->setIcon(QIcon::fromTheme("view-refresh"));
    connect(m_reloadButton, SIGNAL(clicked()), this, SLOT(reload()));

    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->addWidget(m_reloadButton);
    topLayout->addWidget(m_combo);
    topLayout->addWidget(m_goButton);
    topLayout->addWidget(m_indicator);
    m_indicator->show();

    QFrame *mainWidget = new QFrame;
    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->addLayout(topLayout);
    mainLayout->addWidget(m_part->widget());

    setCentralWidget(mainWidget);
    resize(800, 600);
    setupActions();

    m_part->setJScriptEnabled(true);
    m_part->setJavaEnabled(true);
    m_part->setPluginsEnabled(true);
    m_part->setURLCursor(QCursor(Qt::PointingHandCursor));

    connect(m_part, SIGNAL(setWindowCaption(QString)),
            m_part->widget()->topLevelWidget(), SLOT(setCaption(QString)));

    connect(m_part, SIGNAL(started(KIO::Job*)), this, SLOT(startLoading()));
    connect(m_part, SIGNAL(completed()), this, SLOT(finishedLoading()));
}

void TestKHTML::startLoading()
{
    m_movie->start();
}

void TestKHTML::finishedLoading()
{
    m_movie->stop();
}

TestKHTML::~TestKHTML()
{
#ifndef __KDE_HAVE_GCC_VISIBILITY
    delete m_factory;
#endif
}

void TestKHTML::setupActions()
{
    QDomDocument document = m_part->domDocument();
    QDomElement fileMenu = document.documentElement().firstChild().childNodes().item( 0 ).toElement();

    QDomElement quitElement = document.createElement("action");
    quitElement.setAttribute("name",
                             KStandardAction::name(KStandardAction::Quit));
    fileMenu.appendChild(quitElement);

    QDomElement viewMenu = document.documentElement().firstChild().childNodes().item( 2 ).toElement();

    QDomElement element = document.createElement("action");
    element.setAttribute("name", "debugRenderTree");
    viewMenu.appendChild(element);

    element = document.createElement("action");
    element.setAttribute("name", "debugDOMTree");
    viewMenu.appendChild(element);

    QDomElement toolBar = document.documentElement().firstChild().nextSibling().toElement();
    element = document.createElement("action");
    element.setAttribute("name", "editable");
    toolBar.insertBefore(element, toolBar.firstChild());

    element = document.createElement("action");
    element.setAttribute("name", "navigable");
    toolBar.insertBefore(element, toolBar.firstChild());

    element = document.createElement("action");
    element.setAttribute("name", "reload");
    toolBar.insertBefore(element, toolBar.firstChild());

    element = document.createElement("action");
    element.setAttribute("name", "print");
    toolBar.insertBefore(element, toolBar.firstChild());

    QAction *action = new QAction(QIcon::fromTheme("view-refresh"), "Reload", this );
    m_part->actionCollection()->addAction( "reload", action );
    connect(action, SIGNAL(triggered(bool)), this, SLOT(reload()));
    action->setShortcut(Qt::Key_F5);

    QAction *kprint = new QAction(QIcon::fromTheme("document-print"), "Print", this );
    m_part->actionCollection()->addAction( "print", kprint );
    connect(kprint, SIGNAL(triggered(bool)), m_part->browserExtension(), SLOT(print()));
    kprint->setEnabled(true);

    KToggleAction *ta = new KToggleAction( QIcon::fromTheme("edit-rename"), "Navigable", this );
    actionCollection()->addAction( "navigable", ta );
    ta->setShortcuts( QList<QKeySequence>() );
    ta->setChecked(m_part->isCaretMode());
    connect(ta, SIGNAL(toggled(bool)), this, SLOT(toggleNavigable(bool)));

    ta = new KToggleAction( QIcon::fromTheme("document-properties"), "Editable", this );
    actionCollection()->addAction( "editable", ta );
    ta->setShortcuts( QList<QKeySequence>() );
    ta->setChecked(m_part->isEditable());
    connect(ta, SIGNAL(toggled(bool)), this, SLOT(toggleEditable(bool)));

    KStandardAction::quit( qApp, SLOT(quit()), m_part->actionCollection() );

    guiFactory()->addClient(m_part);
}

KHTMLPart *TestKHTML::doc() const
{
    if (m_part)
        return m_part;

    return 0;
}

void TestKHTML::openUrl( const QUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs )
{
    m_part->setArguments(args);
    m_part->browserExtension()->setBrowserArguments(browserArgs);
    m_part->openUrl( url );
}

void TestKHTML::openUrl(const QUrl &url)
{
    m_part->openUrl(url);
}

void TestKHTML::openUrl(const QString &url)
{
    qDebug() << "url: " << url;
    QUrl parsedUrl(url);
    m_part->openUrl(parsedUrl);
}

void TestKHTML::openUrl()
{
    QLineEdit *edit = m_combo->lineEdit();
    if (!edit)
        return;

    QString url = edit->text();
    QUrl parsedUrl(url);
    m_part->openUrl(parsedUrl);
}

void TestKHTML::reload()
{
    KParts::OpenUrlArguments args;
    args.setReload( true );
    m_part->setArguments( args );
    m_part->openUrl( m_part->url() );
}

void TestKHTML::toggleNavigable(bool s)
{
    m_part->setCaretMode(s);
}

void TestKHTML::toggleEditable(bool s)
{
    qDebug() << "editable: " << s;
    m_part->setEditable(s);
}

// a basic web browser using the KHTML library
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (app.arguments().count() == 0) {
        qWarning() << "Argument expected: url to open";
        return 1;
    }

    TestKHTML *test = new TestKHTML;
    QUrl url = QUrl::fromUserInput(app.arguments().at(0)); // TODO support for relative paths
    if (url.path().right(4).toLower() == ".xml")
    {
        KParts::OpenUrlArguments args(test->doc()->arguments());
        args.setMimeType("text/xml");
        test->doc()->setArguments(args);
    }

    test->openUrl(url);
    test->show();

    return app.exec();
}











