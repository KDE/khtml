// program to test the new khtml implementation

#include "testkhtml.h"

#include <stdlib.h>
#include <QApplication>
#include "khtmlview.h"
#include "html_document.h"
#include "htmltokenizer.h"
// to be able to delete a static protected member pointer in kbrowser...
// just for memory debugging
#define protected public
#include "khtml_part.h"
#undef protected
#include "misc/loader.h"
#include <QAction>
#include <QCursor>
#include <QDomDocument>
#include <QFileDialog>
#include <dom_string.h>
#include "dom/dom2_range.h"
#include "dom/html_document.h"
#include "dom/dom_exception.h"
#include <stdio.h>
#include "css/cssstyleselector.h"
#include "html/html_imageimpl.h"
#include "rendering/render_style.h"
#include "khtml_global.h"
#include <kxmlguiwindow.h>

#include <ktoggleaction.h>
#include <kactioncollection.h>
#include "kxmlguifactory.h"
#include <ksharedconfig.h>
#include <kconfiggroup.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if ( a.arguments().count() == 0 ) {
        qWarning() << "Argument expected: url to open";
        return 1;
    }

    new KHTMLGlobal;

    KXmlGuiWindow *toplevel = new KXmlGuiWindow();
    KHTMLPart *doc = new KHTMLPart( toplevel, toplevel, KHTMLPart::BrowserViewGUI );

    Dummy *dummy = new Dummy( doc );
    QObject::connect( doc->browserExtension(), SIGNAL(openUrlRequest(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
		      dummy, SLOT(slotOpenURL(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)) );

    QObject::connect( doc, SIGNAL(completed()), dummy, SLOT(handleDone()) );

    QUrl url = QUrl::fromUserInput(a.arguments().at(0)); // TODO support for relative paths
    if (url.path().right(4).toLower() == ".xml") {
        KParts::OpenUrlArguments args(doc->arguments());
        args.setMimeType("text/xml");
        doc->setArguments(args);
    }

    doc->openUrl(url);

    toplevel->setCentralWidget( doc->widget() );
    toplevel->resize( 800, 600);

    QDomDocument d = doc->domDocument();
    QDomElement viewMenu = d.documentElement().firstChild().childNodes().item( 2 ).toElement();
    QDomElement e = d.createElement( "action" );
    e.setAttribute( "name", "debugRenderTree" );
    viewMenu.appendChild( e );
    e = d.createElement( "action" );
    e.setAttribute( "name", "debugDOMTree" );
    viewMenu.appendChild( e );
    e = d.createElement( "action" );
    e.setAttribute( "name", "debugDoBenchmark" );
    viewMenu.appendChild( e );

    QDomElement toolBar = d.documentElement().firstChild().nextSibling().toElement();
    e = d.createElement( "action" );
    e.setAttribute( "name", "editable" );
    toolBar.insertBefore( e, toolBar.firstChild() );
    e = d.createElement( "action" );
    e.setAttribute( "name", "navigable" );
    toolBar.insertBefore( e, toolBar.firstChild() );
    e = d.createElement( "action" );
    e.setAttribute( "name", "reload" );
    toolBar.insertBefore( e, toolBar.firstChild() );
    e = d.createElement( "action" );
    e.setAttribute( "name", "print" );
    toolBar.insertBefore( e, toolBar.firstChild() );

    QAction *action = new QAction(QIcon::fromTheme("view-refresh"),  "Reload", doc );
    doc->actionCollection()->addAction( "reload", action );
    QObject::connect(action, SIGNAL(triggered(bool)), dummy, SLOT(reload()));
    action->setShortcut(Qt::Key_F5);

    QAction *bench = new QAction( QIcon(), "Benchmark...", doc );
    doc->actionCollection()->addAction( "debugDoBenchmark", bench );
    QObject::connect(bench, SIGNAL(triggered(bool)), dummy, SLOT(doBenchmark()));

    QAction *kprint = new QAction(QIcon::fromTheme("document-print"),  "Print", doc );
    doc->actionCollection()->addAction( "print", kprint );
    QObject::connect(kprint, SIGNAL(triggered(bool)), doc->browserExtension(), SLOT(print()));
    kprint->setEnabled(true);
    KToggleAction *ta = new KToggleAction( QIcon::fromTheme("edit-rename"), "Navigable", doc );
    doc->actionCollection()->addAction( "navigable", ta );
    ta->setShortcuts( QList<QKeySequence>() );
    ta->setChecked(doc->isCaretMode());
    QWidget::connect(ta, SIGNAL(toggled(bool)), dummy, SLOT(toggleNavigable(bool)));
    ta = new KToggleAction( QIcon::fromTheme("document-properties"), "Editable", doc );
    doc->actionCollection()->addAction( "editable", ta );
    ta->setShortcuts( QList<QKeySequence>() );
    ta->setChecked(doc->isEditable());
    QWidget::connect(ta, SIGNAL(toggled(bool)), dummy, SLOT(toggleEditable(bool)));
    toplevel->guiFactory()->addClient( doc );

    doc->setJScriptEnabled(true);
    doc->setJavaEnabled(true);
    doc->setPluginsEnabled( true );
    doc->setURLCursor(QCursor(Qt::PointingHandCursor));
    a.setActiveWindow(doc->widget());
    QWidget::connect(doc, SIGNAL(setWindowCaption(QString)),
		     doc->widget()->topLevelWidget(), SLOT(setCaption(QString)));
    doc->widget()->show();
    toplevel->show();
    doc->view()->viewport()->show();
    doc->view()->widget()->show();


    int ret = a.exec();
    return ret;
}

void Dummy::doBenchmark()
{
    KConfigGroup settings(KSharedConfig::openConfig(), "bench");
    results.clear();

    const QString startDir = settings.readPathEntry("path", QString());
    QString directory = QFileDialog::getExistingDirectory(m_part->view(),
            QString::fromLatin1("Please select directory with tests"),
            startDir);

    if (!directory.isEmpty()) {
        settings.writePathEntry("path", directory);
        KSharedConfig::openConfig()->sync();

        QDir dirListing(directory, "*.html");
        for (unsigned i = 0; i < dirListing.count(); ++i) {
            filesToBenchmark.append(dirListing.absoluteFilePath(dirListing[i]));
        }
    }

    benchmarkRun = 0;

    if (!filesToBenchmark.isEmpty())
        nextRun();
}

const int COLD_RUNS = 2;
const int HOT_RUNS  = 6;

void Dummy::nextRun()
{
    if (benchmarkRun == (COLD_RUNS + HOT_RUNS)) {
        filesToBenchmark.removeFirst();
        benchmarkRun = 0;
    }

    if (!filesToBenchmark.isEmpty()) {
        loadTimer.start();
        m_part->openUrl(QUrl::fromLocalFile(filesToBenchmark[0]));
    } else {
        //Generate HTML for report.
        m_part->begin();
        m_part->write("<html><body><table border=1>");

        for (QMap<QString, QList<int> >::iterator i = results.begin(); i != results.end(); ++i) {
            m_part->write("<tr><td>" + i.key() + "</td>");
            QList<int> timings = i.value();
            int total = 0;
            for (int pos = 0; pos < timings.size(); ++pos) {
                int t = timings[pos];
                if (pos < COLD_RUNS)
                    m_part->write(QString::fromLatin1("<td>(Cold):") + QString::number(t) + "</td>");
                else {
                    total += t;
                    m_part->write(QString::fromLatin1("<td><i>") + QString::number(t) + "</i></td>");
                }
            }

            m_part->write(QString::fromLatin1("<td>Average:<b>") + QString::number(double(total) / HOT_RUNS) + "</b></td>");

            m_part->write("</tr>");
        }

        m_part->end();
    }
}

void Dummy::handleDone()
{
    if (filesToBenchmark.isEmpty()) return;

    results[filesToBenchmark[0]].append(loadTimer.elapsed());
    ++benchmarkRun;
    QTimer::singleShot(100, this, SLOT(nextRun()));
}
