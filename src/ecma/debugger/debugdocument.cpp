#include "debugdocument.h"
#include "debugwindow.h"

#include <QHash>
#include <QVector>
#include <QStringList>
#include <QApplication>

#include "kjs_binding.h"
#include "khtml_part.h"

#include <ktexteditor/document.h>
#include <ktexteditor/view.h>
#include <ktexteditor/editor.h>
#include <ktexteditor/configinterface.h>
#include <ktexteditor/markinterface.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <qcryptographichash.h>

using namespace KJS;
using namespace KJSDebugger;

DebugDocument::DebugDocument(KJS::Interpreter* intp, const QString& url,
                             int sourceId, int baseLine, const QString &source)
{
    m_interpreter = intp;
    m_url   = url;
    
    m_firstLine = baseLine;
    m_sourceId  = sourceId;
    m_sourceLines = source.split('\n');

    QUrl kurl(url);
    m_name = kurl.fileName();

    // Might have to fall back in case of query-like things;
    // ad scripts tend to do that
    while (m_name.contains("=") || m_name.contains("&")) {
        kurl = kurl.upUrl();
        m_name = kurl.fileName();
    }

    if (m_name.isEmpty())
        m_name = kurl.host();

    if (m_name.isEmpty())
        m_name = "????"; //Probably better than un-i18n'd 'undefined'...

    m_kteDoc  = 0;
    m_kteView = 0;
    m_rebuilding    = false;
    m_reload        = false;
    m_hasFunctions  = false;
}

DebugDocument::~DebugDocument()
{
    emit documentDestroyed(this);

    // View has an another parent for UI purposes, so we have to clean it up
    delete m_kteView;
}

KJS::Interpreter* DebugDocument::interpreter()
{
    return m_interpreter;
}

bool DebugDocument::hasFunctions()
{
    return m_hasFunctions;
}

void DebugDocument::setHasFunctions()
{
    m_hasFunctions = true;
}

QString DebugDocument::name() const
{
    return m_name;
}

int DebugDocument::sid() const
{
    return m_sourceId;
}

int DebugDocument::baseLine() const
{
    return m_firstLine;
}

int DebugDocument::length() const
{
    return m_sourceLines.size();
}

QString DebugDocument::url() const
{
    return m_url;
}

void DebugDocument::reloaded(int sourceId, const QString &source)
{
    assert(m_reload);
    m_reload = false;

    m_sourceLines = source.split('\n');
    m_sourceId    = sourceId;
    m_md5.clear();
    
    if (m_kteDoc) // Update docu if needed
        rebuildViewerDocument();
}

void DebugDocument::markReload()
{
    m_reload = true;
}

bool DebugDocument::isMarkedReload() const
{
    return m_reload;
}

void DebugDocument::setBreakpoint(int lineNumber)
{
    if (m_rebuilding) return;

    breakpoints().append(lineNumber);
}

void DebugDocument::removeBreakpoint(int lineNumber)
{
    if (m_rebuilding) return;

    QVector<int>& br = breakpoints();
    int idx = breakpoints().indexOf(lineNumber);
    if (idx != -1)
    {
        br.remove(idx);
        if (br.isEmpty() && !m_url.isEmpty())
        {
            // We just removed the last breakpoint per URL,
            // so we can kill the entire list
            s_perUrlBreakPoints->remove(url());
        }
    }
}

bool DebugDocument::hasBreakpoint(int lineNumber)
{
    return breakpoints().contains(lineNumber);
}

QHash<QString, QVector<int> >* DebugDocument::s_perUrlBreakPoints = 0;
QHash<QString, QVector<int> >* DebugDocument::s_perHashBreakPoints = 0;

QVector<int>& DebugDocument::breakpoints()
{
    if (m_url.isEmpty())
    {
        if (!s_perHashBreakPoints)
            s_perHashBreakPoints = new QHash<QString, QVector<int> >;

        if (m_md5.isEmpty())
        {
            QCryptographicHash hash(QCryptographicHash::Md5);
            hash.addData(m_sourceLines.join("\n").toUtf8());
            m_md5 = QString::fromLatin1(hash.result().toHex());
        }

        return (*s_perHashBreakPoints)[m_md5];
    }
    else
    {
        if (!s_perUrlBreakPoints)
            s_perUrlBreakPoints = new QHash<QString, QVector<int> >;

        return (*s_perUrlBreakPoints)[m_url];
    }
}

KTextEditor::Document* DebugDocument::viewerDocument()
{
    if (!m_kteDoc)
        rebuildViewerDocument();
    return m_kteDoc;
}

KTextEditor::Editor* DebugDocument::s_kate = 0;

KTextEditor::Editor* DebugDocument::kate()
{
    if (!s_kate)
        s_kate = KTextEditor::editor("katepart");

    if (!s_kate)
    {
        KMessageBox::error(DebugWindow::window(), i18n("Unable to find the Kate editor component;\n"
                                      "please check your KDE installation."));
        qApp->exit(1);
    }

    return s_kate;
}


void DebugDocument::rebuildViewerDocument()
{
    m_rebuilding = true;

    if (!m_kteDoc)
    {
        m_kteDoc = kate()->createDocument(this);
        setupViewerDocument();
    }

    KTextEditor::Cursor oldPos;
    if (m_kteView)
        oldPos = m_kteView->cursorPosition();

    m_kteDoc->setReadWrite(true);    
    m_kteDoc->setText(m_sourceLines.join("\n"));

    // Restore cursor pos, if there is a view
    if (m_kteView)
        m_kteView->setCursorPosition(oldPos);

    // Check off the pending/URL-based breakpoints. We have to do even
    // when the document is being updated as they may be on later lines
    // Note that we have to fiddle with them based on our base line, as 
    // views will always start at 0, even for later fragments
    KTextEditor::MarkInterface* imark = qobject_cast<KTextEditor::MarkInterface*>(m_kteDoc);
    if (imark)
    {
        QVector<int>& bps = breakpoints();
        foreach (int bpLine, bps) {
            int relLine = bpLine - m_firstLine;
            if (0 <= relLine && relLine < length())
                imark->addMark(relLine, KTextEditor::MarkInterface::BreakpointActive);
        }
    }

    m_kteDoc->setReadWrite(false);
    m_rebuilding = false;
}

void DebugDocument::setupViewerDocument()
{
    // Highlight as JS..
    m_kteDoc->setMode("JavaScript");

    // Configure all the breakpoint/execution point marker stuff.
    // ### there is an odd split of mark use between here and DebugWindow.
    // Perhaps we should just emit a single and let it do it, and
    // limit ourselves to ownership?
    KTextEditor::MarkInterface* imark = qobject_cast<KTextEditor::MarkInterface*>(m_kteDoc);
    assert(imark);

    imark->setEditableMarks(KTextEditor::MarkInterface::BreakpointActive);
    connect(m_kteDoc, SIGNAL(markChanged(KTextEditor::Document*,KTextEditor::Mark,KTextEditor::MarkInterface::MarkChangeAction)),
                DebugWindow::window(), SLOT(markSet(KTextEditor::Document*,KTextEditor::Mark,KTextEditor::MarkInterface::MarkChangeAction)));

    imark->setMarkDescription(KTextEditor::MarkInterface::BreakpointActive,
                                          i18n("Breakpoint"));
    imark->setMarkPixmap(KTextEditor::MarkInterface::BreakpointActive,
                                     SmallIcon("flag-red"));
    imark->setMarkPixmap(KTextEditor::MarkInterface::Execution,
                                     SmallIcon("arrow-right"));
}

KTextEditor::View* DebugDocument::viewerView()
{
    if (m_kteView)
        return m_kteView;

    // Ensure document is created
    viewerDocument();

    m_kteView = m_kteDoc->createView(DebugWindow::window());
    KTextEditor::ConfigInterface* iconf = qobject_cast<KTextEditor::ConfigInterface*>(m_kteView);
    assert(iconf);
    if (iconf->configKeys().contains("line-numbers"))
        iconf->setConfigValue("line-numbers", false);
    if (iconf->configKeys().contains("icon-bar"))
        iconf->setConfigValue("icon-bar", true);
    if (iconf->configKeys().contains("dynamic-word-wrap"))
        iconf->setConfigValue("dynamic-word-wrap", true);

    return m_kteView;
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
