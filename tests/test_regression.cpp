/**
 * This file is part of the KDE project
 *
 * Copyright (C) 2001,2003 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003,2004 Stephan Kulow (coolo@kde.org)
 * Copyright (C) 2004 Dirk Mueller ( mueller@kde.org )
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "test_regression.h"

#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>
#include <unistd.h>

#include <QApplication>
#include <kacceleratormanager.h>
#include <kconfiggroup.h>

#include <QImage>
#include <QtCore/QFile>
#include <QtCore/QEventLoop>
#include <stdio.h>

#include "css/cssstyleselector.h"
#include <dom_string.h>
#include "rendering/render_style.h"
#include "rendering/render_layer.h"
#include "khtmldefaults.h"
#include <QtCore/QProcess>
#include <QCommonStyle>
#include <QStyleOption>

#include <dom/dom_node.h>
#include <dom/dom_element.h>
#include <dom/dom_text.h>
#include <dom/dom_xml.h>

//We don't use the default fonts, though, but traditional testregression ones
#undef HTML_DEFAULT_VIEW_FONT
#undef HTML_DEFAULT_VIEW_FIXED_FONT
#undef HTML_DEFAULT_VIEW_SERIF_FONT
#undef HTML_DEFAULT_VIEW_SANSSERIF_FONT
#undef HTML_DEFAULT_VIEW_CURSIVE_FONT
#undef HTML_DEFAULT_VIEW_FANTASY_FONT
#define HTML_DEFAULT_VIEW_FONT "helvetica"
#define HTML_DEFAULT_VIEW_FIXED_FONT "courier"
#define HTML_DEFAULT_VIEW_SERIF_FONT "times"
#define HTML_DEFAULT_VIEW_SANSSERIF_FONT "helvetica"
#define HTML_DEFAULT_VIEW_CURSIVE_FONT "helvetica"
#define HTML_DEFAULT_VIEW_FANTASY_FONT "helvetica"

#ifdef __GNUC__
#warning "Kill this at some point"
#endif



struct PalInfo
{
    QPalette::ColorRole role;
    quint32            color;
};

PalInfo palInfo[] =
{
    {QPalette::WindowText, 0xff000000},
    {QPalette::Button, 0xffc0c0c0},
    {QPalette::Light, 0xffffffff},
    {QPalette::Midlight, 0xffdfdfdf},
    {QPalette::Dark, 0xff808080},
    {QPalette::Mid, 0xffa0a0a4},
    {QPalette::Text, 0xff000000},
    {QPalette::BrightText, 0xffffffff},
    {QPalette::ButtonText, 0xff000000},
    {QPalette::Base, 0xffffffff},
    {QPalette::Window, 0xffc0c0c0},
    {QPalette::Shadow, 0xff000000},
    {QPalette::Highlight, 0xff000080},
    {QPalette::HighlightedText, 0xffffffff},
    {QPalette::Link, 0xff0000ff},
    {QPalette::LinkVisited, 0xffff00ff},
    {QPalette::LinkVisited, 0}
};

PalInfo disPalInfo[] =
{
    {QPalette::WindowText, 0xff808080},
    {QPalette::Button, 0xffc0c0c0},
    {QPalette::Light, 0xffffffff},
    {QPalette::Midlight, 0xffdfdfdf},
    {QPalette::Dark, 0xff808080},
    {QPalette::Mid, 0xffa0a0a4},
    {QPalette::Text, 0xff808080},
    {QPalette::BrightText, 0xffffffff},
    {QPalette::ButtonText, 0xff808080},
    {QPalette::Base, 0xffc0c0c0},
    {QPalette::Window, 0xffc0c0c0},
    {QPalette::Shadow, 0xff000000},
    {QPalette::Highlight, 0xff000080},
    {QPalette::HighlightedText, 0xffffffff},
    {QPalette::Link, 0xff0000ff},
    {QPalette::LinkVisited, 0xffff00ff},
    {QPalette::LinkVisited, 0}
};



class TestStyle: public QCommonStyle // was QWindowsStyle in Qt4. TODO: Check if this draws everything we need in Qt5...
{
public:
    TestStyle()
    {}

    virtual void drawControl(ControlElement element, const QStyleOption* option, QPainter* painter, const QWidget* widget) const
    {
        switch (element)
        {
        case CE_ScrollBarSubLine:
        case CE_ScrollBarAddLine:
        case CE_ScrollBarSubPage:
        case CE_ScrollBarAddPage:
        case CE_ScrollBarFirst:
        case CE_ScrollBarLast:
        case CE_ScrollBarSlider:
        {
            const QStyleOptionSlider* sbOpt = qstyleoption_cast<const QStyleOptionSlider*>(option);

            if (sbOpt->minimum == sbOpt->maximum)
            {
                const_cast<QStyleOptionSlider*>(sbOpt)->state ^= QStyle::State_Enabled;
                if (element == CE_ScrollBarSlider)
                    element = CE_ScrollBarAddPage;
            }

            if (element == CE_ScrollBarAddPage || element == CE_ScrollBarSubPage)
            {
                //Fun. in Qt4, the brush offset seems to be sensitive to window position??
                painter->setBrushOrigin(0,1);
            }
            break;
        }
        default: //shaddup
            break;
        }

        QCommonStyle::drawControl(element, option, painter, widget);
    }

    virtual QRect subControlRect(ComplexControl control, const QStyleOptionComplex* option,
                                 SubControl subControl, const QWidget* widget) const
    {
        QRect rect = QCommonStyle::subControlRect(control, option, subControl, widget);

        switch (control)
        {
        case CC_ComboBox:
            if (subControl == SC_ComboBoxEditField)
                return rect.translated(3,0);
            else
                return rect;
        default:
            return rect;
        }
    }

    virtual QSize sizeFromContents(ContentsType type, const QStyleOption* option, const QSize& contentsSize, const QWidget* widget) const
    {
        QSize size = QCommonStyle::sizeFromContents(type, option, contentsSize, widget);

        switch (type)
        {
        case CT_PushButton:
            return QSize(size.width(), size.height() - 1);
        case CT_LineEdit:
	    if (widget && widget->parentWidget() && !qstricmp(widget->parentWidget()->metaObject()->className(), "KUrlRequester"))
		return QSize(size.width() + 1, size.height());
            return QSize(size.width() + 2, size.height() + 2);
        case CT_ComboBox:
        {
            const QStyleOptionComboBox* cbOpt = qstyleoption_cast<const QStyleOptionComboBox*>(option);
			Q_UNUSED(cbOpt); // is 'cbOpt' needed at all here?
            return QSize(qMax(43, size.width() + 6), size.height());
        }
        default:
            return size;
        }

    }

    virtual int pixelMetric(PixelMetric metric, const QStyleOption* option, const QWidget* widget) const
    {
        if (metric == PM_ButtonMargin)
            return 7;
        return QCommonStyle::pixelMetric(metric, option, widget);
    }

};

const char* imageMissingIcon =
"iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAYAAAAf8/9hAAAACXBIWXMAAAzrAAAM6wHl1kTSAAAB2ElEQVQ4jZWTMWsiQRTHfxlDCln2iFhIClksRUIQC6t8BkurzTewEBGx2GKrICmsUsg218rBWV2xdQqLRVJYhUQkBCKLeN6iixB5d0WyYOKGcP9mmHm83/u/NzMHvOobcMz/6Tfw5/Btc+z7/oWIoJRCKQWwt0ZSSqHr+vddACLyZck44GFc8OnpiX6/z3Q6RSmFYRhUq1UMw9gDqF2AUorRaES73WYymVAsFsnlcnieR6PRwPO8dy3uAcIwxHEcADRNo1qt0mq16PV6ZDIZut0uYRh+Dri7u2O1WlGr1QCwLIsgCMhkMlQqFebzOePx+P1cdgG+7wNQKpWwbRsRodlsslgsOD8/R0R4fHyMdwBwcnKCiHBzc0M6neby8hIRoV6vMxwOERGy2eznDvL5PJqmMRgMmM1mpFIprq6uEBFs20bXdc7OzkgkEvvXGA2uVqth2zamaVIul9E0jeVyiVKKdrtNMplkvV7HAwDK5TLX19c4jsN4PEZEKBQKmKbJdrvl/v4e13UfgAXAwVueEYbhRdwzTiQSvLy8cHt7y+npKZ1O59myrB8RQH10EKcgCDg6OoqSf/L6keJbiNNms8F13QfLsn69Jf+NYlELGpD+grMAgo+H/wARELhn1VB8lwAAAABJRU5ErkJggg==";
//r 727816 of oxygen's image-missing, base64'd PNG


#include <kio/job.h>
#include <kmainwindow.h>
#include <kconfig.h>

#include <QColor>
#include <QCursor>
#include <QtCore/QDir>
#include <QtCore/QObject>
#include <QPushButton>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>
#include <QStatusBar>

#include "dom/dom2_range.h"
#include "dom/dom_exception.h"
#include "dom/html_document.h"
#include "html/htmltokenizer.h"
#include "khtml_part.h"
#include "khtmlpart_p.h"
#include <kparts/browserextension.h>
#include <qstandardpaths.h>
#include <qcommandlineparser.h>
#include <qcommandlineoption.h>

#include "khtmlview.h"
#include "rendering/render_replaced.h"
#include "xml/dom_docimpl.h"
#include "html/html_baseimpl.h"
#include "dom/dom_doc.h"
#include "misc/loader.h"
#include "ecma/kjs_binding.h"
#include "ecma/kjs_dom.h"
#include "ecma/kjs_window.h"
#include "ecma/kjs_proxy.h"

using namespace khtml;
using namespace DOM;
using namespace KJS;

static bool visual = false;
static pid_t xvfb;

// -------------------------------------------------------------------------

PartMonitor *PartMonitor::sm_highestMonitor = NULL;

PartMonitor::PartMonitor(KHTMLPart *_part)
{
    m_part = _part;
    m_completed = false;
    connect(m_part,SIGNAL(completed()),this,SLOT(partCompleted()));
    m_timer_waits = 200;
    m_timeout_timer = new QTimer(this);
}

PartMonitor::~PartMonitor()
{
   if (this == sm_highestMonitor)
	sm_highestMonitor = 0;
   while (!m_eventLoopStack.isEmpty())
       exitLoop();
}


void PartMonitor::waitForCompletion()
{
    if (!m_completed) {

        if (sm_highestMonitor)
		return;

	sm_highestMonitor = this;

        enterLoop();

        //connect(m_timeout_timer, SIGNAL(timeout()), this, SLOT(timeout()) );
        //m_timeout_timer->stop();
	//m_timeout_timer->start( visual ? 100 : 2, true );
    }
    QTimer::singleShot( 0, this, SLOT(finishTimers()) );
    enterLoop();
}

void PartMonitor::enterLoop()
{
    if (m_eventLoopStack.isEmpty() || m_eventLoopStack.top()->isRunning())
        m_eventLoopStack.push(new QEventLoop());
    m_eventLoopStack.top()->exec();
}

void PartMonitor::exitLoop()
{
    while (!m_eventLoopStack.isEmpty() && !m_eventLoopStack.top()->isRunning())
        delete m_eventLoopStack.pop();
    if (!m_eventLoopStack.isEmpty())
        m_eventLoopStack.top()->exit();
}

void PartMonitor::timeout()
{
    exitLoop();
}

void PartMonitor::finishTimers()
{

    KJS::Window *w = KJS::Window::retrieveWindow( m_part );
    --m_timer_waits;
    if ( m_timer_waits && ((w && w->winq->hasTimers()) || m_part->inProgress())) {
        // wait a bit
        QTimer::singleShot( 10, this, SLOT(finishTimers()) );
        return;
    }
    exitLoop();
}

void PartMonitor::partCompleted()
{
    m_completed = true;
    m_timeout_timer->stop();
    connect(m_timeout_timer, SIGNAL(timeout()),this, SLOT(timeout()) );
	m_timeout_timer->setSingleShot(true);
	m_timeout_timer->start(visual ? 100 : 2);
    disconnect(m_part,SIGNAL(completed()),this,SLOT(partCompleted()));
}

static void signal_handler( int )
{
    printf( "timeout - this should *NOT* happen, it is likely the part's completed() signal was not emitted - FIXME!!\n" );
    if (PartMonitor::sm_highestMonitor)
        PartMonitor::sm_highestMonitor->exitLoop();
    else
        abort();
}
// -------------------------------------------------------------------------

RegTestObject::RegTestObject(ExecState *exec, RegressionTest *_regTest)
{
    m_regTest = _regTest;
    putDirect("print",new RegTestFunction(exec,m_regTest,RegTestFunction::Print,1), DontEnum);
    putDirect("reportResult",new RegTestFunction(exec,m_regTest,RegTestFunction::ReportResult,3), DontEnum);
    putDirect("checkOutput",new RegTestFunction(exec,m_regTest,RegTestFunction::CheckOutput,1), DontEnum);
    // add "quit" for compatibility with the mozilla js shell
    putDirect("quit", new RegTestFunction(exec,m_regTest,RegTestFunction::Quit,1), DontEnum );
}

RegTestFunction::RegTestFunction(ExecState* /*exec*/, RegressionTest *_regTest, int _id, int length)
{
    m_regTest = _regTest;
    id = _id;
    putDirect("length",length);
}

bool RegTestFunction::implementsCall() const
{
    return true;
}

JSValue* RegTestFunction::callAsFunction(ExecState *exec, JSObject* /*thisObj*/, const List &args)
{
    JSValue* result = jsUndefined();
    if ( m_regTest->ignore_errors )
        return result;

    switch (id) {
	case Print: {
	    UString str = args[0]->toString(exec);
            if ( str.qstring().toLower().indexOf( "failed!" ) >= 0 )
                m_regTest->saw_failure = true;
            QString res = str.qstring().replace('\007', "");
            m_regTest->m_currentOutput += res + "\n";	//krazy:exclude=duoblequote_chars DOM demands chars
	    break;
	}
	case ReportResult: {
            bool passed = args[0]->toBoolean(exec);
            QString description = args[1]->toString(exec).qstring();
            if (args[1]->type() == UndefinedType || args[1]->type() == NullType)
                description.clear();
            m_regTest->reportResult(passed,description);
            if ( !passed )
                m_regTest->saw_failure = true;
            break;
        }
	case CheckOutput: {
            DOM::DocumentImpl* docimpl = static_cast<DOM::DocumentImpl*>( m_regTest->m_part->document().handle() );
            if ( docimpl && docimpl->view() && docimpl->renderer() )
            {
                docimpl->updateRendering();
            }
            QString filename = args[0]->toString(exec).qstring();
            filename = RegressionTest::curr->m_currentCategory+"/"+filename;	//krazy:exclude=duoblequote_chars DOM demands chars
            int failures = RegressionTest::NoFailure;
            if ( m_regTest->m_genOutput ) {
                if ( !m_regTest->reportResult( m_regTest->checkOutput(filename+"-dom"),
                                               "Script-generated " + filename + "-dom") )
                    failures |= RegressionTest::DomFailure;
                if ( !m_regTest->reportResult( m_regTest->checkOutput(filename+"-render"),
                                         "Script-generated " + filename + "-render") )
                    failures |= RegressionTest::RenderFailure;
            } else {
                // compare with output file
                if ( !m_regTest->reportResult( m_regTest->checkOutput(filename+"-dom"), "DOM") )
                    failures |= RegressionTest::DomFailure;
                if ( !m_regTest->reportResult( m_regTest->checkOutput(filename+"-render"), "RENDER") )
                    failures |= RegressionTest::RenderFailure;
            }
            RegressionTest::curr->doFailureReport( filename, failures );
            break;
        }
        case Quit:
            m_regTest->reportResult(true,
				    "Called quit" );
            if ( !m_regTest->saw_failure )
                m_regTest->ignore_errors = true;
            break;
    }

    return result;
}

// -------------------------------------------------------------------------

KHTMLPartObject::KHTMLPartObject(ExecState *exec, KHTMLPart *_part)
{
    m_part = _part;
    putDirect("openPage", new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::OpenPage,1), DontEnum);
    putDirect("openPageAsUrl", new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::OpenPageAsUrl,1), DontEnum);
    putDirect("begin",     new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::Begin,1), DontEnum);
    putDirect("write",    new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::Write,1), DontEnum);
    putDirect("end",    new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::End,0), DontEnum);
    putDirect("executeScript", new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::ExecuteScript,0), DontEnum);
    putDirect("processEvents", new KHTMLPartFunction(exec,m_part,KHTMLPartFunction::ProcessEvents,0), DontEnum);
}

KJS::JSValue *KHTMLPartObject::winGetter(KJS::ExecState *, KJS::JSObject*, const KJS::Identifier&, const KJS::PropertySlot& slot)
{
    KHTMLPartObject* thisObj = static_cast<KHTMLPartObject*>(slot.slotBase());
    return KJS::Window::retrieveWindow(thisObj->m_part);
}

KJS::JSValue *KHTMLPartObject::docGetter(KJS::ExecState *exec, KJS::JSObject*, const KJS::Identifier&, const KJS::PropertySlot& slot)
{
    KHTMLPartObject* thisObj = static_cast<KHTMLPartObject*>(slot.slotBase());
    return getDOMNode(exec, thisObj->m_part->document().handle());
}


bool KHTMLPartObject::getOwnPropertySlot(KJS::ExecState *exec, const KJS::Identifier& propertyName, KJS::PropertySlot& slot)
{
    if (propertyName == "document") {
        slot.setCustom(this, docGetter);
        return true;
    }
    else if (propertyName == "window") {
        slot.setCustom(this, winGetter);
        return true;
    }
    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

KHTMLPartFunction::KHTMLPartFunction(ExecState */*exec*/, KHTMLPart *_part, int _id, int length)
{
    m_part = _part;
    id = _id;
    putDirect("length",length);
}

bool KHTMLPartFunction::implementsCall() const
{
    return true;
}

JSValue* KHTMLPartFunction::callAsFunction(ExecState *exec, JSObject*/*thisObj*/, const List &args)
{
    JSValue* result = jsUndefined();

    switch (id) {
        case OpenPage: {
	    if (args[0]->type() == NullType || args[0]->type() == NullType) {
		exec->setException(Error::create(exec, GeneralError,"No filename specified"));
		return jsUndefined();
	    }

            QString filename = args[0]->toString(exec).qstring();
            QString fullFilename = QFileInfo(RegressionTest::curr->m_currentBase+"/"+filename).absoluteFilePath();	//krazy:exclude=duoblequote_chars DOM demands chars
            QUrl url = QUrl::fromLocalFile(fullFilename);
            PartMonitor pm(m_part);
            m_part->openUrl(url);
            pm.waitForCompletion();
	    qApp->processEvents(QEventLoop::AllEvents);
            break;
        }
	case OpenPageAsUrl: {
	    if (args[0]->type() == NullType || args[0]->type() == UndefinedType) {
		exec->setException(Error::create(exec, GeneralError,"No filename specified"));
		return jsUndefined();
	    }
	    if (args[1]->type() == NullType || args[1]->type() == UndefinedType) {
		exec->setException(Error::create(exec, GeneralError,"No url specified"));
		return jsUndefined();
	    }

            QString filename = args[0]->toString(exec).qstring();
            QString url = args[1]->toString(exec).qstring();
            QFile file(RegressionTest::curr->m_currentBase+"/"+filename);	//krazy:exclude=duoblequote_chars DOM demands chars
	    if (!file.open(QIODevice::ReadOnly)) {
		exec->setException(Error::create(exec, GeneralError,
						 qPrintable(QString("Error reading " + filename))));
	    }
	    else {
		QByteArray fileData;
		QDataStream stream(&fileData,QIODevice::WriteOnly);
		char buf[1024];
		int bytesread;
		while (!file.atEnd()) {
		    bytesread = file.read(buf,1024);
		    stream.writeRawData(buf,bytesread);
		}
		file.close();
		QString contents(fileData);
		PartMonitor pm(m_part);
		m_part->begin(QUrl( url ));
		m_part->write(contents);
		m_part->end();
		pm.waitForCompletion();
	    }
	    qApp->processEvents(QEventLoop::AllEvents);
	    break;
	}
	case Begin: {
            QString url = args[0]->toString(exec).qstring();
            m_part->begin(QUrl( url ));
            break;
        }
        case Write: {
            QString str = args[0]->toString(exec).qstring();
            m_part->write(str);
            break;
        }
        case End: {
            m_part->end();
	    qApp->processEvents(QEventLoop::AllEvents);
            break;
        }
	case ExecuteScript: {
	    QString code = args[0]->toString(exec).qstring();
	    Completion comp;
	    KJSProxy *proxy = m_part->jScript();
	    proxy->evaluate("",0,code,0,&comp);
	    if (comp.complType() == Throw)
		exec->setException(comp.value());
	    qApp->processEvents(QEventLoop::AllEvents);
	    break;
	}
	case ProcessEvents: {
	    qApp->processEvents(QEventLoop::AllEvents);
	    break;
	}
    }

    return result;
}

// -------------------------------------------------------------------------

int main(int argc, char *argv[])
{
    // forget about any settings
    passwd* pw = getpwuid( getuid() );
    if (!pw) {
        fprintf(stderr, "dang, I don't even know who I am.\n");
        exit(1);
    }

    QString kh("/var/tmp/%1_non_existant");
    kh = kh.arg( pw->pw_name );
    qputenv( "XDG_CONFIG_HOME", kh.toLatin1());
    qputenv( "XDG_DATA_HOME", kh.toLatin1());
    qputenv( "LC_ALL", "C");
    qputenv( "LANG", "C");

    // We want KIO to be in the slave-forking mode since
    // then it'll ask KProtocolInfo::exec for the binary to run,
    // and we intercept that, limiting the I/O to file://
    // and the magic data://. See Slave::createSlave in KIO's slave.cpp
    qputenv( "KDE_FORK_SLAVES", "true");
    signal( SIGALRM, signal_handler );

    QApplication a(argc, argv);
    // workaround various Qt crashes by always enforcing a TrueColor visual
    QApplication::setColorSpec( QApplication::ManyColor );

    QCommandLineParser parser;
    parser.addHelpOption(QCoreApplication::translate("main", "Regression tester for khtml"));
    parser.addOption(QCommandLineOption(QStringList() << "b" << "base", QCoreApplication::translate("main", "Directory containing tests, basedir and output directories."), "base_dir"));
    parser.addOption(QCommandLineOption(QStringList() << "d" << "debug", QCoreApplication::translate("main", "Do not suppress debug output")));
    parser.addOption(QCommandLineOption(QStringList() << "g" << "genoutput", QCoreApplication::translate("main", "Regenerate baseline (instead of checking)")));
    parser.addOption(QCommandLineOption(QStringList() << "s" << "noshow", QCoreApplication::translate("main", "Do not show the window while running tests")));
    parser.addOption(QCommandLineOption(QStringList() << "t" << "test", QCoreApplication::translate("main", "Only run a single test. Multiple options allowed."), "filename"));
    parser.addOption(QCommandLineOption(QStringList() << "js", QCoreApplication::translate("main", "Only run .js tests")));
    parser.addOption(QCommandLineOption(QStringList() << "html", QCoreApplication::translate("main", "Only run .html tests")));
    parser.addOption(QCommandLineOption(QStringList() << "noxvfb", QCoreApplication::translate("main", "Do not use Xvfb")));
    parser.addOption(QCommandLineOption(QStringList() << "o" << "output", QCoreApplication::translate("main", "Put output in &lt;directory&gt; instead of &lt;base_dir&gt;/output"), "directory"));
    parser.addOption(QCommandLineOption(QStringList() << "r" << "reference", QCoreApplication::translate("main", "Use &lt;directory&gt; as reference instead of &lt;base_dir&gt;/baseline"), "directory"));
    parser.addOption(QCommandLineOption(QStringList() << "+[base_dir]", QCoreApplication::translate("main", "Directory containing tests, basedir and output directories. Only regarded if -b is not specified.")));
    parser.addOption(QCommandLineOption(QStringList() << "+[testcases]", QCoreApplication::translate("main", "Relative path to testcase, or directory of testcases to be run (equivalent to -t).")));
    parser.process(a);


    QString baseDir = parser.value("base");

    if ( parser.remainingArguments().count() < 1 && baseDir.isEmpty() ) {
        parser.showHelp();
        ::exit( 1 );
    }

    int testcase_index = 0;
    if (baseDir.isEmpty())
        baseDir = parser.remainingArguments().at(testcase_index++);

    QFileInfo bdInfo(baseDir);
    // font pathes passed to Xvfb must be absolute
    if (bdInfo.isRelative())
        baseDir = bdInfo.dir().absolutePath();

    const char *subdirs[] = {"tests", "baseline", "output", "resources"};
    for ( int i = 0; i < 3; i++ ) {
        QFileInfo sourceDir(baseDir + QLatin1Char('/') + QLatin1String(subdirs[i]));
        if ( !sourceDir.exists() || !sourceDir.isDir() ) {
            fprintf(stderr,"ERROR: Source directory \"%s\": no such directory.\n", sourceDir.filePath().toLocal8Bit().data());
            exit(1);
        }
    }

    if (parser.isSet("xvfb"))
    {
        QString xvfbPath = QStandardPaths::findExecutable("Xvfb");
        if ( xvfbPath.isEmpty() ) {
            fprintf( stderr, "ERROR: We need Xvfb to be installed for reliable results\n" );
            exit( 1 );
        }

        QByteArray xvfbPath8 = QFile::encodeName(xvfbPath);
        QStringList fpaths;
        fpaths.append(baseDir+"/resources");

        const char* const fontdirs[] = { "75dpi", "misc", "Type1" };
        const char* const fontpaths[] =  {"/usr/share/fonts/", "/usr/X11/lib/X11/fonts/",
            "/usr/lib/X11/fonts/", "/usr/share/fonts/X11/" };

        for (size_t fp=0; fp < sizeof(fontpaths)/sizeof(*fontpaths); ++fp)
            for (size_t fd=0; fd < sizeof(fontdirs)/sizeof(*fontdirs); ++fd)
                if (QFile::exists(QLatin1String(fontpaths[fp])+QLatin1String(fontdirs[fd]))) {
                    if (strcmp(fontdirs[fd] , "Type1"))
                        fpaths.append(QLatin1String(fontpaths[fp])+QLatin1String(fontdirs[fd])+":unscaled");
                    else
                        fpaths.append(QLatin1String(fontpaths[fp])+QLatin1String(fontdirs[fd]));
		}

        xvfb = fork();
        if ( !xvfb ) {
            QByteArray buffer = fpaths.join(",").toLatin1();
            execl( xvfbPath8.data(), xvfbPath8.data(), "-once", "-dpi", "100", "-screen", "0",
                    "1024x768x16", "-ac", "-fp", buffer.data(), ":47", (char*)NULL );
        }

        qputenv( "DISPLAY", ":47");
    }

    a.setStyle( new TestStyle );
    KConfig sc1( "cryptodefaults", KConfig::SimpleConfig );
    KConfigGroup grp = sc1.group("Warnings");
    grp.writeEntry( "OnUnencrypted",  false );
    KSharedConfigPtr config = KSharedConfig::openConfig();
    grp = config->group("Notification Messages" );
    grp.writeEntry( "kjscupguard_alarmhandler", true );
    grp.writeEntry("ReportJSErrors", false);
    KConfig cfg( "khtmlrc" );
    grp = cfg.group("HTML Settings");
    grp.writeEntry( "StandardFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT );
    grp.writeEntry( "FixedFont", HTML_DEFAULT_VIEW_FIXED_FONT );
    grp.writeEntry( "SerifFont", HTML_DEFAULT_VIEW_SERIF_FONT );
    grp.writeEntry( "SansSerifFont", HTML_DEFAULT_VIEW_SANSSERIF_FONT );
    grp.writeEntry( "CursiveFont", HTML_DEFAULT_VIEW_CURSIVE_FONT );
    grp.writeEntry( "FantasyFont", HTML_DEFAULT_VIEW_FANTASY_FONT );
    grp.writeEntry( "MinimumFontSize", HTML_DEFAULT_MIN_FONT_SIZE );
    grp.writeEntry( "MediumFontSize", 10 );
    grp.writeEntry( "Fonts", QStringList() );
    grp.writeEntry( "DefaultEncoding", "" );
    grp = cfg.group("Java/JavaScript Settings");
    grp.writeEntry( "WindowOpenPolicy", (int)KHTMLSettings::KJSWindowOpenAllow);

    cfg.sync();
    grp.sync();

    KJS::ScriptInterpreter::turnOffCPUGuard();

    QPalette pal = a.palette();
    for (int c = 0; palInfo[c].color; ++c)
    {
        pal.setColor(QPalette::Active,   palInfo[c].role, QColor(palInfo[c].color));
        pal.setColor(QPalette::Inactive, palInfo[c].role, QColor(palInfo[c].color));
        pal.setColor(QPalette::Disabled, palInfo[c].role, QColor(disPalInfo[c].color));
    }
    a.setPalette(pal);

    int rv = 1;

    bool outputDebug = parser.isSet( "debug" );

    KConfig dc( "kdebugrc", KConfig::SimpleConfig );
    static int areas[] = { 1000, 6000, 6005, 6010, 6020, 6030,
                            6031, 6035, 6036, 6040, 6041, 6045,
                            6050, 6060, 6061, 7000, 7006, 170,
                            171, 7101, 7002, 7019, 7027, 7014,
                            7011, 6070, 6080, 6090, 0};
    for ( int i = 0; areas[i]; ++i ) {
        grp = dc.group( QString::number( areas[i] ) );
        grp.writeEntry( "InfoOutput", outputDebug ? 2 : 4 );
        grp.sync();
    }
    dc.sync();

    kClearDebugConfig();

    // make sure the missing image icon is independent of the icon theme..
    QByteArray brokenImData = QByteArray::fromBase64(imageMissingIcon);
    QImage brokenIm;
    brokenIm.loadFromData(brokenImData);
    khtml::Cache::brokenPixmap = new QPixmap(QPixmap::fromImage(brokenIm));

    // create widgets
    KMainWindow *toplevel = new KMainWindow();
    KHTMLPart *part = new KHTMLPart( toplevel, 0, KHTMLPart::BrowserViewGUI );

    toplevel->setCentralWidget( part->widget() );
    KAcceleratorManager::setNoAccel ( part->widget() );
    part->setJScriptEnabled(true);

    part->executeScript(DOM::Node(), ""); // force the part to create an interpreter
    part->setJavaEnabled(false);
    part->setPluginsEnabled(false);

    if (parser.isSet("show"))
	visual = true;

    //a.setTopWidget(part->widget());
    if ( visual )
        toplevel->show();

    // we're not interested
    toplevel->statusBar()->hide();

    if (!getenv("KDE_DEBUG")) {
        // set ulimits
        rlimit vmem_limit = { 512*1024*1024, RLIM_INFINITY };	// 512Mb Memory should suffice
        setrlimit(RLIMIT_AS, &vmem_limit);
        rlimit stack_limit = { 8*1024*1024, RLIM_INFINITY };	// 8Mb Memory should suffice
        setrlimit(RLIMIT_STACK, &stack_limit);
    }

    // run the tests
    RegressionTest *regressionTest = new RegressionTest(part,
                                                        baseDir,
                                                        parser.argument("output"),
                                                        parser.argument("reference"),
                                                        parser.isSet("genoutput"),
                                                        !parser.isSet( "html" ),
                                                        !parser.isSet( "js" ));
    QObject::connect(part->browserExtension(), SIGNAL(openUrlRequest(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
		     regressionTest, SLOT(slotOpenURL(QUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
    QObject::connect(part->browserExtension(), SIGNAL(resizeTopLevelWidget(int,int)),
		     regressionTest, SLOT(resizeTopLevelWidget(int,int)));

    bool result = false;
    QStringList tests = parser.arguments("test");
    // merge testcases specified on command line
    for (; testcase_index < parser.remainingArguments().count(); testcase_index++)
        tests << parser.remainingArguments().at(testcase_index);
    if (tests.count() > 0)
        foreach (QString test, tests) {
	    result = regressionTest->runTests(test,true);
            if (!result) break;
        }
    else
	result = regressionTest->runTests();

    if (result) {
	if (parser.isSet("genoutput")) {
	    printf("\nOutput generation completed.\n");
	}
	else {
	    printf("\nTests completed.\n");
            printf("Total:    %d\n",
                   regressionTest->m_passes_work+
                   regressionTest->m_passes_fail+
                   regressionTest->m_failures_work+
                   regressionTest->m_failures_fail+
                   regressionTest->m_errors);
	    printf("Passes:   %d",regressionTest->m_passes_work);
            if ( regressionTest->m_passes_fail )
                printf( " (%d unexpected passes)\n", regressionTest->m_passes_fail );
            else
                printf( "\n" );
	    printf("Failures: %d",regressionTest->m_failures_work);
            if ( regressionTest->m_failures_fail )
                printf( " (%d expected failures)\n", regressionTest->m_failures_fail );
            else
                printf( "\n" );
            if ( regressionTest->m_errors )
                printf("Errors:   %d\n",regressionTest->m_errors);

            QFile list( regressionTest->m_outputDir + "/links.html" );
            list.open( QIODevice::WriteOnly|QIODevice::Append );
            QString link, cl;
            link = QString( "<hr>%1 failures. (%2 expected failures)" )
                   .arg(regressionTest->m_failures_work )
                   .arg( regressionTest->m_failures_fail );
            list.write( link.toLatin1(), link.length() );
            list.close();
	}
    }

    // Only return a 0 exit code if all tests were successful
    if (regressionTest->m_failures_work == 0 && regressionTest->m_errors == 0)
	rv = 0;

    // cleanup
    delete regressionTest;
    delete part;
    delete toplevel;

    khtml::Cache::clear();
    khtml::CSSStyleSelector::clear();
    khtml::RenderStyle::cleanup();

    kill( xvfb, SIGINT );

    return rv;
}

// -------------------------------------------------------------------------

RegressionTest *RegressionTest::curr = 0;

RegressionTest::RegressionTest(KHTMLPart *part, const QString &baseDir, const QString &outputDir, const QString &baselineDir,
			       bool _genOutput, bool runJS, bool runHTML)
  : QObject(part)
{
    m_part = part;

    m_baseDir = baseDir;
    m_baseDir = m_baseDir.replace( "//", "/" );
    if ( m_baseDir.endsWith( "/" ) )	//krazy:exclude=duoblequote_chars DOM demands chars
        m_baseDir = m_baseDir.left( m_baseDir.length() - 1 );
    if (outputDir.isEmpty())
        m_outputDir = m_baseDir + "/output";
    else {
        createMissingDirs(outputDir + "/");	//krazy:exclude=duoblequote_chars DOM demands chars
        m_outputDir = outputDir;
    }
    m_baselineDir = baselineDir;
    m_baselineDir = m_baselineDir.replace( "//", "/" );
    if (m_baselineDir.endsWith( "/" ) )
        m_baselineDir = m_baselineDir.left( m_baselineDir.length() - 1 );
    if (m_baselineDir.isEmpty())
        m_baselineDir = m_baseDir + "/baseline";
    else {
        createMissingDirs(m_baselineDir + "/");
    }
    m_genOutput = _genOutput;
    m_runJS = runJS;
    m_runHTML =  runHTML;
    m_passes_work = m_passes_fail = 0;
    m_failures_work = m_failures_fail = 0;
    m_errors = 0;

    if (!m_genOutput)
        ::unlink( QFile::encodeName( m_outputDir + "/links.html" ) );
    QFile f( m_outputDir + "/empty.html" );
    QString s;
    f.open( QIODevice::WriteOnly | QIODevice::Truncate );
    s = "<html><body>Follow the white rabbit";
    f.write( s.toLatin1(), s.length() );
    f.close();
    f.setFileName( m_outputDir + "/index.html" );
    f.open( QIODevice::WriteOnly | QIODevice::Truncate );
    s = "<html><frameset cols=150,*><frame src=links.html><frame name=content src=empty.html>";
    f.write( s.toLatin1(), s.length() );
    f.close();

    m_paintBuffer = 0;

    curr = this;
    m_part->view()->setFrameStyle(QFrame::StyledPanel | QFrame::Plain);
    resizeTopLevelWidget(800, 598 );
}


static QStringList readListFile( const QString &filename )
{
    // Read ignore file for this directory
    QString ignoreFilename = filename;
    QFileInfo ignoreInfo(ignoreFilename);
    QStringList ignoreFiles;
    if (ignoreInfo.exists()) {
        QFile ignoreFile(ignoreFilename);
        if (!ignoreFile.open(QIODevice::ReadOnly)) {
            fprintf(stderr,"Can't open %s\n",qPrintable(ignoreFilename));
            exit(1);
        }
        QTextStream ignoreStream(&ignoreFile);
        QString line;
        while (!(line = ignoreStream.readLine()).isNull())
            ignoreFiles.append(line);
        ignoreFile.close();
    }
    return ignoreFiles;
}

RegressionTest::~RegressionTest()
{
    delete m_paintBuffer;
}

bool RegressionTest::runTests(QString relPath, bool mustExist, QStringList failureFileList)
{
    m_currentOutput.clear();

    QString fullPath = m_baseDir + "/tests/" + relPath;

    if (!QFile(fullPath).exists()) {
	fprintf(stderr,"%s: No such file or directory\n",qPrintable(relPath));
	return false;
    }

    QFileInfo info(fullPath);

    if (!info.exists() && mustExist) {
	fprintf(stderr,"%s: No such file or directory\n",qPrintable(relPath));
	return false;
    }

    if (!info.isReadable() && mustExist) {
	fprintf(stderr,"%s: Access denied\n",qPrintable(relPath));
	return false;
    }

    if (info.isDir()) {
        QStringList ignoreFiles = readListFile( fullPath + "/ignore" );
        QStringList failureFiles = readListFile( fullPath + "/KNOWN_FAILURES" );

	// Run each test in this directory, recusively
	QDir sourceDir(m_baseDir + "/tests/"+relPath);
	for (uint fileno = 0; fileno < sourceDir.count(); fileno++) {
	    QString filename = sourceDir[fileno];
	    QString relFilename = relPath.isEmpty() ? filename : relPath+"/"+filename;	//krazy:exclude=duoblequote_chars DOM demands chars

	    if (filename == "." || filename == ".." ||  ignoreFiles.contains(filename) )
                continue;

            runTests(relFilename, false, failureFiles);
	}
    }
    else if (info.isFile()) {

        alarm( 12 );

        khtml::Cache::init();

	QString relativeDir = QFileInfo(relPath).path();
	QString filename = info.fileName();
	m_currentBase = m_baseDir + "/tests/"+relativeDir;
	m_currentCategory = relativeDir;
	m_currentTest = filename;

    if (failureFileList.isEmpty() && QFile(info.path() + "/KNOWN_FAILURES").exists()) {
        failureFileList = readListFile( info.path() + "/KNOWN_FAILURES" );
    }

    int known_failure = NoFailure;
    if ( failureFileList.contains( filename ) )
        known_failure |= AllFailure;
    if ( failureFileList.contains ( filename + "-render" ) )
        known_failure |= RenderFailure;
    if ( failureFileList.contains ( filename + "-dump.png" ) )
        known_failure |= PaintFailure;
    if ( failureFileList.contains ( filename + "-dom" ) )
        known_failure |= DomFailure;

        m_known_failures = known_failure;
	if ( filename.endsWith(".html") || filename.endsWith( ".htm" ) || filename.endsWith( ".xhtml" ) || filename.endsWith( ".xml" ) ) {
            if ( relPath.startsWith( "domts/" ) && !m_runJS )
                return true;
	    if ( relPath.startsWith( "ecma/" ) && !m_runJS )
	        return true;
            if ( m_runHTML )
                testStaticFile(relPath);
	}
	else if (filename.endsWith(".js")) {
            if ( m_runJS ) {
                alarm( 120 );
                testJSFile(relPath);
            }
	}
	else if (mustExist) {
	    fprintf(stderr,"%s: Not a valid test file (must be .htm(l) or .js)\n",qPrintable(relPath));
	    return false;
	}
    } else if (mustExist) {
        fprintf(stderr,"%s: Not a regular file\n",qPrintable(relPath));
        return false;
    }

    return true;
}

void RegressionTest::getPartDOMOutput( QTextStream &outputStream, KHTMLPart* part, uint indent )
{
    DOM::Node node = part->document();
    while (!node.isNull()) {
	// process

	for (uint i = 0; i < indent; i++)
	    outputStream << "  ";

	// Make doctype's visually different from elements
	if (node.nodeType() == DOM::Node::DOCUMENT_TYPE_NODE)
		outputStream << "!doctype ";

	outputStream << node.nodeName().string();

	switch (node.nodeType()) {
	    case DOM::Node::ELEMENT_NODE: {
		// Sort strings to ensure consistent output
		QStringList attrNames;
		NamedNodeMap attrs = node.attributes();
		for (uint a = 0; a < attrs.length(); a++)
		    attrNames.append(attrs.item(a).nodeName().string());
		attrNames.sort();

		QStringList::iterator it;
		Element elem(node);
		for (it = attrNames.begin(); it != attrNames.end(); ++it) {
		    QString name = *it;
		    QString value = elem.getAttribute(*it).string();
		    outputStream << " " << name << "=\"" << value << "\"";
		}
		if ( node.handle()->id() == ID_FRAME ) {
			outputStream << endl;
			QString frameName = static_cast<DOM::HTMLFrameElementImpl *>( node.handle() )->name.string();
			KHTMLPart* frame = part->findFrame( frameName );
			if ( frame )
			    getPartDOMOutput( outputStream, frame, indent );
			else
			    outputStream << "(FRAME NOT FOUND)";
		}
		break;
	    }
	    case DOM::Node::ATTRIBUTE_NODE:
		// Should not be present in tree
		assert(false);
		break;
            case DOM::Node::TEXT_NODE:
		outputStream << " \"" << Text(node).data().string() << "\"";
		break;
            case DOM::Node::CDATA_SECTION_NODE:
		outputStream << " \"" << CDATASection(node).data().string() << "\"";
		break;
            case DOM::Node::ENTITY_REFERENCE_NODE:
		break;
            case DOM::Node::ENTITY_NODE:
		break;
            case DOM::Node::PROCESSING_INSTRUCTION_NODE:
		break;
            case DOM::Node::COMMENT_NODE:
		outputStream << " \"" << Comment(node).data().string() << "\"";
		break;
            case DOM::Node::DOCUMENT_NODE:
		break;
            case DOM::Node::DOCUMENT_TYPE_NODE:
		break;
            case DOM::Node::DOCUMENT_FRAGMENT_NODE:
		// Should not be present in tree
		assert(false);
		break;
            case DOM::Node::NOTATION_NODE:
		break;
            default:
		assert(false);
		break;
	}

	outputStream << endl;

	if (!node.firstChild().isNull()) {
	    node = node.firstChild();
	    indent++;
	}
	else if (!node.nextSibling().isNull()) {
	    node = node.nextSibling();
	}
	else {
	    while (!node.isNull() && node.nextSibling().isNull()) {
		node = node.parentNode();
		indent--;
	    }
	    if (!node.isNull())
		node = node.nextSibling();
	}
    }
}

void RegressionTest::dumpRenderTree( QTextStream &outputStream, KHTMLPart* part )
{
    DOM::DocumentImpl* doc = static_cast<DocumentImpl*>( part->document().handle() );
    if ( !doc || !doc->renderer() )
        return;
    doc->renderer()->layer()->dump( outputStream );

    // Dump frames if any
    // Get list of names instead of frames() to sort the list alphabetically
    QStringList names = part->frameNames();
    names.sort();
    for ( QStringList::iterator it = names.begin(); it != names.end(); ++it ) {
        outputStream << "FRAME: " << (*it) << "\n";
	KHTMLPart* frame = part->findFrame( (*it) );
//	Q_ASSERT( frame );
	if ( frame )
            dumpRenderTree( outputStream, frame );
    }
}

QString RegressionTest::getPartOutput( OutputType type)
{
    // dump out the contents of the rendering & DOM trees
    QString dump;
    QTextStream outputStream(&dump, QIODevice::WriteOnly);

    if ( type == RenderTree ) {
        dumpRenderTree( outputStream, m_part );
    } else {
        assert( type == DOMTree );
        getPartDOMOutput( outputStream, m_part, 0 );
    }

    dump.replace( m_baseDir + "/tests", QLatin1String( "REGRESSION_SRCDIR" ) );
    return dump;
}

QImage RegressionTest::renderToImage()
{
    int ew = m_part->view()->contentsWidth();
    int eh = m_part->view()->contentsHeight();

    if (ew * eh > 4000 * 4000) // don't DoS us
        return QImage();

    QImage img( ew, eh, QImage::Format_ARGB32 );
    img.fill( 0xff0000 );
    if (!m_paintBuffer )
        m_paintBuffer = new QPixmap( 512, 128 );

    for ( int py = 0; py < eh; py += 128 ) {
        for ( int px = 0; px < ew; px += 512 ) {
            QPainter* tp = new QPainter;
            tp->begin( m_paintBuffer );
            tp->translate( -px, -py );
            tp->fillRect(px, py, 512, 128, Qt::magenta);
            m_part->document().handle()->renderer()->layer()->paint( tp, QRect( px, py, 512, 128 ) );
            tp->end();
            delete tp;

            // now fill the chunk into our image
            QImage chunk = m_paintBuffer->toImage();
            assert( chunk.depth() == 32 );
            for ( int y = 0; y < 128 && py + y < eh; ++y )
                memcpy( img.scanLine( py+y ) + px*4, chunk.scanLine( y ), qMin( 512, ew-px )*4 );
        }
    }

    assert( img.depth() == 32 );
    return img;
}

bool RegressionTest::imageEqual( const QImage &lhsi, const QImage &rhsi )
{
    if ( lhsi.width() != rhsi.width() || lhsi.height() != rhsi.height() ) {
        // qDebug() << "dimensions different " << lhsi.size() << " " << rhsi.size();
        return false;
    }
    int w = lhsi.width();
    int h = lhsi.height();
    int bytes = lhsi.bytesPerLine();

    const unsigned char* origLs = lhsi.bits();
    const unsigned char* origRs = rhsi.bits();

    for ( int y = 0; y < h; ++y )
    {
        const QRgb* ls = (const QRgb*)(origLs + y * bytes);
        const QRgb* rs = (const QRgb*)(origRs + y * bytes);
        if ( memcmp( ls, rs, bytes ) ) {
            for ( int x = 0; x < w; ++x ) {
                QRgb l = ls[x];
                QRgb r = rs[x];
                if ( ( abs( qRed( l ) - qRed(r ) ) < 20 ) &&
                     ( abs( qGreen( l ) - qGreen(r ) ) < 20 ) &&
                     ( abs( qBlue( l ) - qBlue(r ) ) < 20 ) )
                    continue;
                 // qDebug() << "pixel (" << x << ", " << y << ") is different " << QColor(  lhsi.pixel (  x, y ) ) << " " << QColor(  rhsi.pixel (  x, y ) );
                return false;
            }
        }
    }

    return true;
}

void RegressionTest::createLink( const QString& test, int failures )
{
    createMissingDirs( m_outputDir + "/" + test + "-compare.html" );	//krazy:exclude=duoblequote_chars DOM demands chars

    QFile list( m_outputDir + "/links.html" );
    list.open( QIODevice::WriteOnly|QIODevice::Append );
    QString link;
    link = QString( "<a href=\"%1\" target=\"content\" title=\"%2\">" )
           .arg( test + "-compare.html" )
           .arg( test );
    link += m_currentTest;
    link += "</a> [";
    if ( failures & DomFailure )
        link += "D";	//krazy:exclude=duoblequote_chars DOM demands chars
    if ( failures & RenderFailure )
        link += "R";	//krazy:exclude=duoblequote_chars DOM demands chars
    if ( failures & PaintFailure )
        link += "P";	//krazy:exclude=duoblequote_chars DOM demands chars
    link += "]<br>\n";
    list.write( link.toLatin1(), link.length() );
    list.close();
}

void RegressionTest::doJavascriptReport( const QString &test )
{
    QFile compare( m_outputDir + "/" + test + "-compare.html" );	//krazy:exclude=duoblequote_chars DOM demands chars
    if ( !compare.open( QIODevice::WriteOnly|QIODevice::Truncate ) )
        qDebug() << "failed to open " << m_outputDir + "/" + test + "-compare.html";	//krazy:exclude=duoblequote_chars DOM demands chars
    QString cl;
    cl = QString( "<html><head><title>%1</title>" ).arg( test );
    cl += "<body><tt>";
    QString text = "\n" + m_currentOutput;	//krazy:exclude=duoblequote_chars DOM demands chars
    text.replace( '<', "&lt;" );
    text.replace( '>', "&gt;" );
    text.replace( QRegExp( "\nFAILED" ), "\n<span style='color: red'>FAILED</span>" );
    text.replace( QRegExp( "\nFAIL" ), "\n<span style='color: red'>FAIL</span>" );
    text.replace( QRegExp( "\nPASSED" ), "\n<span style='color: green'>PASSED</span>" );
    text.replace( QRegExp( "\nPASS" ), "\n<span style='color: green'>PASS</span>" );
    if ( text.at( 0 ) == '\n' )
        text = text.mid( 1, text.length() );
    text.replace( '\n', "<br>\n" );
    cl += text;
    cl += "</tt></body></html>";
    compare.write( cl.toLatin1(), cl.length() );
    compare.close();
}

/** returns the path in a way that is relatively reachable from base.
 * @param base base directory (must not include trailing slash)
 * @param path directory/file to be relatively reached by base
 * @return path with all elements replaced by .. and concerning path elements
 *	to be relatively reachable from base.
 */
static QString makeRelativePath(const QString &base, const QString &path)
{
    QString absBase = QFileInfo(base).absoluteFilePath();
    QString absPath = QFileInfo(path).absoluteFilePath();
//     qDebug() << "absPath: \"" << absPath << "\"";
//     qDebug() << "absBase: \"" << absBase << "\"";

    // walk up to common ancestor directory
    int pos = 0;
    do {
        pos++;
        int newpos = absBase.indexOf('/', pos);
        if (newpos == -1) newpos = absBase.length();
        QString cmpPathComp(absPath.unicode() + pos, newpos - pos);
        QString cmpBaseComp(absBase.unicode() + pos, newpos - pos);
//         qDebug() << "cmpPathComp: \"" << cmpPathComp.string() << "\"";
//         qDebug() << "cmpBaseComp: \"" << cmpBaseComp.string() << "\"";
//         qDebug() << "pos: " << pos << " newpos: " << newpos;
        if (cmpPathComp != cmpBaseComp) { pos--; break; }
        pos = newpos;
    } while (pos < (int)absBase.length() && pos < (int)absPath.length());
    int basepos = pos < (int)absBase.length() ? pos + 1 : pos;
    int pathpos = pos < (int)absPath.length() ? pos + 1 : pos;

//     qDebug() << "basepos " << basepos << " pathpos " << pathpos;

    QString rel;
    {
        QString relBase(absBase.unicode() + basepos, absBase.length() - basepos);
        QString relPath(absPath.unicode() + pathpos, absPath.length() - pathpos);
        // generate as many .. as there are path elements in relBase
        if (relBase.length() > 0) {
            for (int i = relBase.count('/'); i > 0; --i)
                rel += "../";
            rel += "..";
            if (relPath.length() > 0) rel += "/";	//krazy:exclude=duoblequote_chars DOM demands chars
        }
        rel += relPath;
    }
    return rel;
}

static QString getDiff(QString cmdLine)
{
    QProcess p;
    p.start(cmdLine, QIODevice::ReadOnly);
    p.waitForFinished();
    QString text = QString::fromLocal8Bit(p.readAllStandardOutput());
    text = text.replace( '<', "&lt;" );
    text = text.replace( '>', "&gt;" );
    return text;
}

void RegressionTest::doFailureReport( const QString& test, int failures )
{
    if ( failures == NoFailure ) {
        ::unlink( QFile::encodeName( m_outputDir + "/" + test + "-compare.html" ) );	//krazy:exclude=duoblequote_chars DOM demands chars
        return;
    }

    createLink( test, failures );

    if ( failures & JSFailure ) {
        doJavascriptReport( test );
        return; // no support for both kind
    }

    QFile compare( m_outputDir + "/" + test + "-compare.html" );	//krazy:exclude=duoblequote_chars DOM demands chars

    QString testFile = QFileInfo(test).fileName();

    QString renderDiff;
    QString domDiff;

    QString relOutputDir = makeRelativePath(m_baseDir, m_outputDir);
    QString relBaselineDir = makeRelativePath(m_baseDir, m_baselineDir);

    // are blocking reads possible with K3Process?
    QString pwd = QDir::currentPath();
    chdir( QFile::encodeName( m_baseDir ) );

    if ( failures & RenderFailure ) {
        renderDiff += "<pre>";
        renderDiff += getDiff( QString::fromLatin1( "diff -u %4/%1-render %3/%2-render" )
                            .arg ( test, test, relOutputDir, relBaselineDir ) );
        renderDiff += "</pre>";
    }

    if ( failures & DomFailure ) {
        domDiff += "<pre>";
        domDiff += getDiff( QString::fromLatin1( "diff -u %4/%1-dom %3/%2-dom" )
                            .arg ( test, test, relOutputDir, relBaselineDir ) );
        domDiff += "</pre>";
    }

    chdir( QFile::encodeName( pwd ) );

    // create a relative path so that it works via web as well. ugly
    QString relpath = makeRelativePath(m_outputDir + "/"	//krazy:exclude=duoblequote_chars DOM demands chars
        + QFileInfo(test).path(), m_baseDir);

    compare.open( QIODevice::WriteOnly|QIODevice::Truncate );
    QString cl;
    cl = QString( "<html><head><title>%1</title>" ).arg( test );
    cl += QString( "<script>\n"
                  "var pics = new Array();\n"
                  "pics[0]=new Image();\n"
                  "pics[0].src = '%1';\n"
                  "pics[1]=new Image();\n"
                  "pics[1].src = '%2';\n"
                  "var doflicker = 1;\n"
                  "var t = 1;\n"
                  "var lastb=0;\n" )
          .arg( relpath+"/"+relBaselineDir+"/"+test+"-dump.png" )
          .arg( testFile+"-dump.png" );
    cl += QString( "function toggleVisible(visible) {\n"
                  "     document.getElementById('render').style.visibility= visible == 'render' ? 'visible' : 'hidden';\n"
                  "     document.getElementById('image').style.visibility= visible == 'image' ? 'visible' : 'hidden';\n"
                  "     document.getElementById('dom').style.visibility= visible == 'dom' ? 'visible' : 'hidden';\n"
                  "}\n"
                  "function show() { document.getElementById('image').src = pics[t].src; "
                  "document.getElementById('image').style.borderColor = t && !doflicker ? 'red' : 'gray';\n"
                  "toggleVisible('image');\n"
                   "}" );
    cl += QString ( "function runSlideShow(){\n"
                  "   document.getElementById('image').src = pics[t].src;\n"
                  "   if (doflicker)\n"
                  "       t = 1 - t;\n"
                  "   setTimeout('runSlideShow()', 200);\n"
                  "}\n"
                  "function m(b) { if (b == lastb) return; document.getElementById('b'+b).className='buttondown';\n"
                  "                var e = document.getElementById('b'+lastb);\n"
                  "                 if(e) e.className='button';\n"
                  "                 lastb = b;\n"
                  "}\n"
                  "function showRender() { doflicker=0;toggleVisible('render')\n"
                  "}\n"
                  "function showDom() { doflicker=0;toggleVisible('dom')\n"
                  "}\n"
                   "</script>\n");

    cl += QString ("<style>\n"
                   ".buttondown { cursor: pointer; padding: 0px 20px; color: white; background-color: blue; border: inset blue 2px;}\n"
                   ".button { cursor: pointer; padding: 0px 20px; color: black; background-color: white; border: outset blue 2px;}\n"
                   ".diff { position: absolute; left: 10px; top: 100px; visibility: hidden; border: 1px black solid; background-color: white; color: black; /* width: 800; height: 600; overflow: scroll; */ }\n"
                   "</style>\n" );

    if ( failures & PaintFailure )
        cl += QString( "<body onload=\"m(1); show(); runSlideShow();\"" );
    else if ( failures & RenderFailure )
        cl += QString( "<body onload=\"m(4); toggleVisible('render');\"" );
    else
        cl += QString( "<body onload=\"m(5); toggleVisible('dom');\"" );
    cl += QString(" text=black bgcolor=gray>\n<h1>%3</h1>\n" ).arg( test );
    if ( failures & PaintFailure )
        cl += QString ( "<span id='b1' class='buttondown' onclick=\"doflicker=1;show();m(1)\">FLICKER</span>&nbsp;\n"
                        "<span id='b2' class='button' onclick=\"doflicker=0;t=0;show();m(2)\">BASE</span>&nbsp;\n"
                        "<span id='b3' class='button' onclick=\"doflicker=0;t=1;show();m(3)\">OUT</span>&nbsp;\n" );
    if ( renderDiff.length() )
        cl += "<span id='b4' class='button' onclick='showRender();m(4)'>R-DIFF</span>&nbsp;\n";
    if ( domDiff.length() )
        cl += "<span id='b5' class='button' onclick='showDom();m(5);'>D-DIFF</span>&nbsp;\n";
    // The test file always exists - except for checkOutput called from *.js files
    if ( QFile::exists( m_baseDir + "/tests/"+ test ) )
        cl += QString( "<a class=button href=\"%1\">HTML</a>&nbsp;" )
              .arg( relpath+"/tests/"+test );

    cl += QString( "<hr>"
                   "<img style='border: solid 5px gray' src=\"%1\" id='image'>" )
          .arg( relpath+"/"+relBaselineDir+"/"+test+"-dump.png" );

    cl += "<div id='render' class='diff'>" + renderDiff + "</div>";
    cl += "<div id='dom' class='diff'>" + domDiff + "</div>";

    cl += "</body></html>";
    compare.write( cl.toLatin1(), cl.length() );
    compare.close();
}

void RegressionTest::testStaticFile(const QString & filename)
{
    qDebug("TESTING:%s", filename.toLatin1().data());
    resizeTopLevelWidget( 800, 598 ); // restore size

    // Set arguments
    KParts::OpenUrlArguments args;
    if (filename.endsWith(".html") || filename.endsWith(".htm")) args.setMimeType("text/html");
    else if (filename.endsWith(".xhtml")) args.setMimeType("application/xhtml+xml");
    else if (filename.endsWith(".xml")) args.setMimeType("text/xml");
    m_part->setArguments(args);
    // load page
    QUrl url = QUrl::fromLocalFile(QFileInfo(m_baseDir + "/tests/"+filename).absoluteFilePath());
    PartMonitor pm(m_part);
    m_part->openUrl(url);
    pm.waitForCompletion();
    m_part->closeUrl();

    if ( filename.startsWith( "domts/" ) ) {
        QString functionname;

        KJS::Completion comp = m_part->jScriptInterpreter()->evaluate(filename, 0, "exposeTestFunctionNames();");
        /*
         *  Error handling
         */
        KJS::ExecState *exec = m_part->jScriptInterpreter()->globalExec();
        if ( comp.complType() == ReturnValue || comp.complType() == Normal )
        {
            if (comp.value() && comp.value()->type() == ObjectType &&
               comp.value()->toObject(exec)->className() == "Array" )
            {
                JSObject* argArrayObj = comp.value()->toObject(exec);
                unsigned int length = argArrayObj->
                                      get(exec, "length")->
                                      toUInt32(exec);
                if ( length == 1 )
                    functionname = argArrayObj->get(exec, 0)->toString(exec).qstring();
            }
        }
        if ( functionname.isNull() ) {
            // qDebug() << "DOM " << filename << " doesn't expose 1 function name - ignoring";
            return;
        }

        KJS::Completion comp2 = m_part->jScriptInterpreter()->evaluate(filename, 0, QString("setUpPage(); " + functionname + "();") );
        bool success = ( comp2.complType() == ReturnValue || comp2.complType() == Normal );
        QString description = "DOMTS";
        if ( comp2.complType() == Throw ) {
            KJS::JSValue*  val = comp2.value();
            KJS::JSObject* obj = val->toObject(exec);
            if ( obj && obj->hasProperty( exec, "jsUnitMessage" ) )
                description = obj->get( exec, "jsUnitMessage" )->toString( exec ).qstring();
            else
                description = comp2.value()->toString( exec ).qstring();
        }
        reportResult( success,  description );

        if (!success && !m_known_failures)
            doFailureReport( filename, JSFailure );
        return;
    }

    int back_known_failures = m_known_failures;

    if ( m_genOutput ) {
        if ( m_known_failures & DomFailure)
            m_known_failures = AllFailure;
        reportResult( checkOutput(filename+"-dom"), "DOM" );
        if ( m_known_failures & RenderFailure )
            m_known_failures = AllFailure;
        reportResult( checkOutput(filename+"-render"), "RENDER" );
        if ( m_known_failures & PaintFailure )
            m_known_failures = AllFailure;
        renderToImage().save(m_baselineDir + "/" + filename + "-dump.png","PNG", 60);
        printf("Generated %s\n", qPrintable(QString( m_baselineDir + "/" + filename + "-dump.png" )) );
        reportResult( true, "PAINT" );
    } else {
        int failures = NoFailure;

        // compare with output file
        if ( m_known_failures & DomFailure)
            m_known_failures = AllFailure;
        if ( !reportResult( checkOutput(filename+"-dom"), "DOM" ) )
            failures |= DomFailure;

        if ( m_known_failures & RenderFailure )
            m_known_failures = AllFailure;
        if ( !reportResult( checkOutput(filename+"-render"), "RENDER" ) )
            failures |= RenderFailure;

        if ( m_known_failures & PaintFailure )
            m_known_failures = AllFailure;
        if (!reportResult( checkPaintdump(filename), "PAINT") )
            failures |= PaintFailure;

        doFailureReport(filename, failures );
    }

    m_known_failures = back_known_failures;
}

void RegressionTest::evalJS( ScriptInterpreter &interp, const QString &filename, bool report_result )
{
    QString fullSourceName = filename;
    QFile sourceFile(fullSourceName);

    if (!sourceFile.open(QIODevice::ReadOnly)) {
        fprintf(stderr,"Error reading file %s\n",qPrintable(fullSourceName));
        exit(1);
    }

    QTextStream stream ( &sourceFile );
	stream.setCodec( "UTF-8" );
    QString code = stream.readAll();
    sourceFile.close();

    saw_failure = false;
    ignore_errors = false;
    Completion c = interp.evaluate(filename, 0, UString( code ) );

    if ( report_result && !ignore_errors) {
        bool expected_failure = filename.endsWith( "-n.js" );
        if (c.complType() == Throw) {
            ExecState* exec = interp.globalExec();
            QString errmsg = c.value()->toString(exec).qstring();
            if ( !expected_failure ) {
                int line = -1;
                JSObject* obj = c.value()->toObject(exec);
                if (obj)
                    line = obj->get(exec, "line")->toUInt32(exec);
                printf( "ERROR: %s (%s) at line:%d\n",qPrintable(filename), qPrintable(errmsg), line);
                doFailureReport( m_currentCategory + "/" + m_currentTest, JSFailure );	//krazy:exclude=duoblequote_chars DOM demands chars
                m_errors++;
            } else {
                reportResult( true, QString( "Expected Failure: %1" ).arg( errmsg ) );
            }
        } else if ( saw_failure ) {
            if ( !expected_failure )
                doFailureReport( m_currentCategory + "/" + m_currentTest, JSFailure );	//krazy:exclude=duoblequote_chars DOM demands chars
            reportResult( expected_failure, "saw 'failed!'" );
        } else {
            reportResult( !expected_failure, "passed" );
        }
    }
}

class GlobalImp : public JSGlobalObject {
public:
  virtual UString className() const { return "global"; }
};

void RegressionTest::testJSFile(const QString & filename )
{
    qDebug("TEST JS:%s", filename.toLatin1().data());
    resizeTopLevelWidget( 800, 598 ); // restore size

    // create interpreter
    // note: this is different from the interpreter used by the part,
    // it contains regression test-specific objects & functions
    ProtectedPtr<JSGlobalObject> global(new GlobalImp());
    khtml::ChildFrame frame;
    frame.m_part = m_part;
    ScriptInterpreter interp(global,&frame);
    ExecState *exec = interp.globalExec();

    global->put(exec, "part", new KHTMLPartObject(exec,m_part));
    global->put(exec, "regtest", new RegTestObject(exec,this));
    global->put(exec, "debug", new RegTestFunction(exec,this,RegTestFunction::Print,1) );
    global->put(exec, "print", new RegTestFunction(exec,this,RegTestFunction::Print,1) );

    QStringList dirs = filename.split( '/' );
    // NOTE: the basename is of little interest here, but the last basedir change
    // isn't taken in account
    QString basedir =  m_baseDir + "/tests/";
    foreach(const QString &it, dirs) {
        if ( ! ::access( QFile::encodeName( basedir + "shell.js" ), R_OK ) )
            evalJS( interp, basedir + "shell.js", false );
        basedir += it + "/";	//krazy:exclude=duoblequote_chars DOM demands chars
    }
    evalJS( interp, m_baseDir + "/tests/"+ filename, true );
}

RegressionTest::CheckResult RegressionTest::checkPaintdump(const QString &filename)
{
    QString againstFilename( filename + "-dump.png" );
    QString absFilename = QFileInfo(m_baselineDir + "/" + againstFilename).absoluteFilePath();
    if ( svnIgnored( absFilename ) ) {
        m_known_failures = NoFailure;
        return Ignored;
    }
    CheckResult result = Failure;

    QImage baseline;
    baseline.load( absFilename, "PNG");
    QImage output = renderToImage();
    if ( !imageEqual( baseline, output ) ) {	//krazy:exclude=duoblequote_chars DOM demands chars
        QString outputFilename = m_outputDir + "/" + againstFilename;
        createMissingDirs(outputFilename );

        bool kf = false;
        if ( m_known_failures & AllFailure )
            kf = true;
        else if ( m_known_failures & PaintFailure )
            kf = true;
        if ( kf )
            outputFilename += "-KF";

        output.save(outputFilename, "PNG", 60);
    }
    else {
        ::unlink( QFile::encodeName( m_outputDir + "/" + againstFilename ) );	//krazy:exclude=duoblequote_chars DOM demands chars
        result = Success;
    }
    return result;
}

RegressionTest::CheckResult RegressionTest::checkOutput(const QString &againstFilename)
{
    QString absFilename = QFileInfo(m_baselineDir + "/" + againstFilename).absoluteFilePath();
    if ( svnIgnored( absFilename ) ) {
        m_known_failures = NoFailure;
        return Ignored;
    }

    bool domOut = againstFilename.endsWith( "-dom" );
    QString data = getPartOutput( domOut ? DOMTree : RenderTree );
    data.remove( char( 13 ) );

    CheckResult result = Success;

    // compare result to existing file
    QString outputFilename = QFileInfo(m_outputDir + "/" + againstFilename).absoluteFilePath();	//krazy:exclude=duoblequote_chars DOM demands chars
    bool kf = false;
    if ( m_known_failures & AllFailure )
        kf = true;
    else if ( domOut && ( m_known_failures & DomFailure ) )
        kf = true;
    else if ( !domOut && ( m_known_failures & RenderFailure ) )
        kf = true;
    if ( kf )
        outputFilename += "-KF";

    if ( m_genOutput )
        outputFilename = absFilename;

    QFile file(absFilename);
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream ( &file );
		stream.setCodec( "UTF-8" );

        QString fileData = stream.readAll();

        result = ( fileData == data ) ? Success : Failure;
        if ( !m_genOutput && result == Success ) {
            ::unlink( QFile::encodeName( outputFilename ) );
            return Success;
        }
    }

    // generate result file
    createMissingDirs( outputFilename );
    QFile file2(outputFilename);
    if (!file2.open(QIODevice::WriteOnly)) {
        fprintf(stderr,"Error writing to file %s\n",qPrintable(outputFilename));
        exit(1);
    }

    QTextStream stream2(&file2);
	stream2.setCodec( "UTF-8" );
    stream2 << data;
    if ( m_genOutput )
        printf("Generated %s\n", qPrintable(outputFilename));

    return result;
}

bool RegressionTest::reportResult(CheckResult result, const QString & description)
{
    if ( result == Ignored ) {
        //printf("IGNORED: ");
        //printDescription( description );
        return true; // no error
    } else
        return reportResult( result == Success, description );
}

bool RegressionTest::reportResult(bool passed, const QString & description)
{
    if (m_genOutput)
	return true;

   if (passed) {
        if ( m_known_failures & AllFailure ) {
            printf("PASS (unexpected!): ");
            m_passes_fail++;
        } else {
            printf("PASS: ");
            m_passes_work++;
        }
    }
    else {
        if ( m_known_failures & AllFailure ) {
            printf("FAIL (known): ");
            m_failures_fail++;
            passed = true; // we knew about it
        } else {
            printf("FAIL: ");
            m_failures_work++;
        }
    }

    printDescription( description );
    return passed;
}

void RegressionTest::printDescription(const QString& description)
{
    if (!m_currentCategory.isEmpty())
	printf("%s/", qPrintable(m_currentCategory));

    printf("%s", qPrintable(m_currentTest));

    if (!description.isEmpty()) {
        QString desc = description;
        desc.replace( '\n', ' ' );
	printf(" [%s]", qPrintable(desc));
    }

    printf("\n");
    fflush(stdout);
}

void RegressionTest::createMissingDirs(const QString & filename)
{
    QFileInfo dif(filename);
    QFileInfo dirInfo( dif.path() );
    if (dirInfo.exists())
	return;

    QStringList pathComponents;
    QFileInfo parentDir = dirInfo;
    pathComponents.prepend(parentDir.absoluteFilePath());
    while (!parentDir.exists()) {
	QString parentPath = parentDir.absoluteFilePath();
	int slashPos = parentPath.lastIndexOf('/');
	if (slashPos < 0)
	    break;
	parentPath = parentPath.left(slashPos);
	pathComponents.prepend(parentPath);
	parentDir = QFileInfo(parentPath);
    }
    for (int pathno = 1; pathno < pathComponents.count(); pathno++) {
	if (!QFileInfo(pathComponents[pathno]).exists() &&
	    !QDir(pathComponents[pathno-1]).mkdir(pathComponents[pathno])) {
	    fprintf(stderr,"Error creating directory %s\n",qPrintable(pathComponents[pathno]));
	    exit(1);
	}
    }
}

void RegressionTest::slotOpenURL(const QUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs)
{
    m_part->setArguments(args);
    m_part->browserExtension()->setBrowserArguments(browserArgs);

    PartMonitor pm(m_part);
    m_part->openUrl(url);
    pm.waitForCompletion();
}

bool RegressionTest::svnIgnored( const QString &filename )
{
    QFileInfo fi( filename );
    QString ignoreFilename = fi.path() + "/svnignore";
    QFile ignoreFile(ignoreFilename);
    if (!ignoreFile.open(QIODevice::ReadOnly))
        return false;

    QTextStream ignoreStream(&ignoreFile);
    QString line;
    while (!(line = ignoreStream.readLine()).isNull()) {
        if ( line == fi.fileName() )
            return true;
    }
    ignoreFile.close();
    return false;
}

void RegressionTest::resizeTopLevelWidget( int w, int h )
{
    m_part->widget()->parentWidget()->resize( w, h );
    m_part->widget()->resize( w, h );
    // Since we're not visible, this doesn't have an immediate effect, QWidget posts the event
    QApplication::sendPostedEvents( 0, QEvent::Resize );
}

