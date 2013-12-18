#ifndef DEBUGDOCUMENT_H
#define DEBUGDOCUMENT_H

#include <QHash>
#include <QObject>
#include <QVector>
#include <QStringList>

#include "misc/shared.h"

namespace KJS {
    class Interpreter;
}

namespace KTextEditor {
    class Document;
    class View;
    class Editor;
}

namespace KJSDebugger {

class DebugDocument : public QObject, public khtml::Shared<DebugDocument>
{
    Q_OBJECT
public:
    typedef SharedPtr<DebugDocument> Ptr;

    DebugDocument(KJS::Interpreter* interp, const QString& url, 
                  int sourceId, int baseLine, const QString &source);
    ~DebugDocument();

    QString name() const;
    QString url() const;
    int     sid() const;

    KTextEditor::Document* viewerDocument();
    KTextEditor::View*     viewerView();

    // Marks the document as being discarded for reload, so that new data should be set here.
    void markReload();
    bool isMarkedReload() const;
    
    void reloaded(int sourceId, const QString &source);

    void setBreakpoint(int lineNumber);
    void removeBreakpoint(int lineNumber);
    bool hasBreakpoint(int lineNumber);
    
    // We keep track of whether documents have functions, since we can't discard
    // eval contexts that do
    bool hasFunctions();
    void setHasFunctions();
    
    KJS::Interpreter* interpreter(); 
    
    int baseLine() const;
    int length()   const;

Q_SIGNALS:
    void documentDestroyed(KJSDebugger::DebugDocument*);
private:
    QString m_url;
    QString m_name;
    KJS::Interpreter* m_interpreter;
    
    bool m_hasFunctions;

    // This is set to true when we are rebuilding the document.
    // in that case, the UI might get undesired mark add/remove events,
    // and update the breakpoint set accordingly --- such as removing all of them
    // on clear. 
    bool m_rebuilding;

    bool m_reload;
    
    int m_firstLine;
    int m_sourceId;
    QStringList m_sourceLines;
    QString     m_md5; // empty if invalid/not computed.

    void rebuildViewerDocument();
    void setupViewerDocument();

    // We store breakpoints differently for scopes with URL
    // and without it. Those that have it are stored globally,
    // so that breakpoints persist across multiple visits of the
    // document. For eval scopes, we store them based on the MD5
    // of the source code.
    static QHash<QString, QVector<int> >* s_perUrlBreakPoints;
    static QHash<QString, QVector<int> >* s_perHashBreakPoints;

    QVector<int>& breakpoints();

    KTextEditor::Document* m_kteDoc;
    KTextEditor::View*     m_kteView;
    static KTextEditor::Editor* s_kate;
    static KTextEditor::Editor* kate();
};

}


#endif
