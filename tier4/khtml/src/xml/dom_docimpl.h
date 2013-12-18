/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2002-2003 Apple Computer, Inc.
 *           (C) 2006 Allan Sandfeld Jensen(kde@carewolf.com)
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

#ifndef _DOM_DocumentImpl_h_
#define _DOM_DocumentImpl_h_

#include "xml/dom_elementimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "xml/dom_textimpl.h"
#include "xml/dom2_traversalimpl.h"
#include "xml/security_origin.h"
#include "misc/shared.h"
#include "misc/loader.h"
#include "misc/seed.h"

#include <QtCore/QStringList>
#include <QtCore/QObject>
#include <QtCore/QList>
#include <QtCore/QHash>
#include <QtCore/QMap>
#include <QUrl>

//Added by qt3to4:
#include <QTimerEvent>

class QPaintDevice;
class QTextCodec;
class KHTMLView;
class QEventLoop;
class KEncodingDetector;

namespace khtml {
    class Tokenizer;
    class CSSStyleSelector;
    class DocLoader;
    class RenderArena;
    class EditCommand;
    class RenderObject;
    class CounterNode;
    class CachedObject;
    class CachedCSSStyleSheet;
    class DynamicDomRestyler;

    class XPathResultImpl;
    class XPathExpressionImpl;
    class XPathNSResolverImpl;
}

namespace KJS {
    class Window;
}

namespace WebCore
{
    class SVGDocumentExtensions;
    class SVGDocument;
} // namespace WebCore

namespace DOM {

    class AbstractViewImpl;
    class AttrImpl;
    class CDATASectionImpl;
    class CSSStyleSheetImpl;
    class CommentImpl;
    class DocumentFragmentImpl;
    class DocumentImpl;
    class XMLDocumentImpl;
    class DocumentType;
    class DocumentTypeImpl;
    class EditingTextImpl;
    class ElementImpl;
    class EntityReferenceImpl;
    class EventImpl;
    class EventListener;
    class HTMLDocumentImpl;
    class HTMLElementImpl;
    class HTMLPartContainerElementImpl;
    class HTMLImageElementImpl;
    class JSEditor;
    class NodeFilter;
    class NodeFilterImpl;
    class NodeIteratorImpl;
    class NodeListImpl;
    class ProcessingInstructionImpl;
    class RangeImpl;
    class StyleSheetImpl;
    class StyleSheetListImpl;
    class TextImpl;
    class TreeWalkerImpl;
    class WindowEventTargetImpl;    

class DOMImplementationImpl : public khtml::Shared<DOMImplementationImpl>
{
public:
    DOMImplementationImpl();
    ~DOMImplementationImpl();

    // DOM methods & attributes for DOMImplementation.
    static bool hasFeature ( const DOMString &feature, const DOMString &version );
    DocumentTypeImpl *createDocumentType( const DOMString &qualifiedName, const DOMString &publicId,
                                          const DOMString &systemId, int &exceptioncode );
    static DocumentImpl *createDocument( const DOMString &namespaceURI, const DOMString &qualifiedName,
                                  DocumentTypeImpl* dtype,
                                  KHTMLView* v, int &exceptioncode );

    // From the DOMImplementationCSS interface
    static CSSStyleSheetImpl *createCSSStyleSheet(DOMStringImpl *title, DOMStringImpl *media, int &exceptioncode);

    // From the HTMLDOMImplementation interface
    static HTMLDocumentImpl* createHTMLDocument( const DOMString& title);

    // Other methods (not part of DOM)
    static DocumentImpl *createDocument( KHTMLView *v = 0 );
    static XMLDocumentImpl *createXMLDocument( KHTMLView *v = 0 );
    static HTMLDocumentImpl *createHTMLDocument( KHTMLView *v = 0 );
    static WebCore::SVGDocument *createSVGDocument( KHTMLView *v = 0 );
};

/**
 * @internal A cache of element name (or id) to pointer
 * ### KDE4, QHash: better to store values here
 */
class ElementMappingCache
{
public:
    /**
     For each name, we hold a reference count, and a
     pointer. If the item is in the table, which implies
     reference count is > 1, the name is a valid key.
     If the pointer is non-null, it points to the appropriate
     mapping
    */
    struct ItemInfo
    {
        int       ref;
        ElementImpl* nd;
    };

    ElementMappingCache();
    ~ElementMappingCache();

    /**
     Add a pointer as just one of candidates, not neccesserily the proper one
    */
    void add(const DOMString& id, ElementImpl* nd);

    /**
     Set the pointer as the definite mapping; it must have already been added
    */
    void set(const DOMString& id, ElementImpl* nd);

    /**
     Remove the item; it must have already been added.
    */
    void remove(const DOMString& id, ElementImpl* nd);

    /**
     Returns true if the item exists
    */
    bool contains(const DOMString& id);

    /**
     Returns the information for the given ID
    */
    ItemInfo* get(const DOMString& id);
private:
    QHash<DOMString,ItemInfo*> m_dict;
};

/**
 * @internal
 */
class DocumentImpl : public QObject, private khtml::CachedObjectClient, public NodeBaseImpl
{
    Q_OBJECT
public:
    DocumentImpl(KHTMLView *v);
    ~DocumentImpl();

    // DOM methods & attributes for Document

    DocumentTypeImpl *doctype() const;

    DOMImplementationImpl *implementation() const;
    ElementImpl *documentElement() const;
    virtual void childrenChanged();
    virtual ElementImpl *createElement ( const DOMString &tagName, int* pExceptioncode = 0 );
    virtual AttrImpl *createAttribute( const DOMString &tagName, int* pExceptioncode = 0 );
    DocumentFragmentImpl *createDocumentFragment ();
    TextImpl *createTextNode ( DOMStringImpl* data ) { return new TextImpl( docPtr(), data); }
    TextImpl *createTextNode ( const QString& data )
        { return createTextNode(new DOMStringImpl(data.unicode(), data.length())); }
    TextImpl *createTextNode ( const DOMString &data ) { return createTextNode(data.implementation()); }
    TextImpl *createTextNode ( const char *latin1 ) { return createTextNode(DOMString(latin1)); }
    CommentImpl *createComment ( DOMStringImpl* data );
    CDATASectionImpl *createCDATASection ( DOMStringImpl* data );
    ProcessingInstructionImpl *createProcessingInstruction ( const DOMString &target, DOMStringImpl* data );
    EntityReferenceImpl *createEntityReference ( const DOMString &name );
    NodeImpl *importNode( NodeImpl *importedNode, bool deep, int &exceptioncode );
    virtual ElementImpl *createElementNS ( const DOMString &_namespaceURI, const DOMString &_qualifiedName,
                                           int* pExceptioncode = 0 );
    virtual AttrImpl *createAttributeNS( const DOMString &_namespaceURI, const DOMString &_qualifiedName,
                                           int* pExceptioncode = 0 );
    ElementImpl *getElementById ( const DOMString &elementId ) const;

    // DOM3 XPath, from XPathEvaluator interface
    khtml::XPathExpressionImpl *createExpression( DOM::DOMString& expression,
                                                  khtml::XPathNSResolverImpl *resolver,
                                                  int &exceptioncode );
    khtml::XPathNSResolverImpl *createNSResolver( NodeImpl *nodeResolver );
    khtml::XPathResultImpl *evaluate( DOM::DOMString& expression,
                                      NodeImpl *contextNode,
                                      khtml::XPathNSResolverImpl *resolver,
                                      unsigned short type,
                                      khtml::XPathResultImpl *result,
                                      int &exceptioncode );

    // Actually part of HTMLDocument, but used for giving XML documents a window title as well
    DOMString title() const { return m_title; }
    void setTitle(const DOMString& _title);

    // DOM methods overridden from  parent classes

    virtual DOMString nodeName() const;
    virtual unsigned short nodeType() const;

    // Other methods (not part of DOM)
    virtual bool isDocumentNode() const { return true; }
    virtual bool isHTMLDocument() const { return false; }
    virtual bool isSVGDocument() const;

    virtual ElementImpl *createHTMLElement ( const DOMString &tagName, bool caseInsensitive = true );
    // SVG
    virtual ElementImpl *createSVGElement(const QualifiedName& name);

    khtml::CSSStyleSelector *styleSelector() { return m_styleSelector; }

     /**
     * Updates the pending sheet count and then calls updateStyleSelector.
     */
    void styleSheetLoaded();

    /**
     * This method returns true if all top-level stylesheets have loaded (including
     * any \@imports that they may be loading).
     */
    bool haveStylesheetsLoaded() const { return m_pendingStylesheets <= 0 || m_ignorePendingStylesheets; }

    /**
     * Increments the number of pending sheets.  The \<link\> elements
     * invoke this to add themselves to the loading list.
     */
    void addPendingSheet();

    /**
     * Returns true if the document has pending stylesheets
     * loading.
     */
    bool hasPendingSheets() const { return m_pendingStylesheets; }

    /**
     * Called when one or more stylesheets in the document may have been added, removed or changed.
     *
     * Creates a new style selector and assign it to this document. This is done by iterating through all nodes in
     * document (or those before \<BODY\> in a HTML document), searching for stylesheets. Stylesheets can be contained in
     * \<LINK\>, \<STYLE\> or \<BODY\> elements, as well as processing instructions (XML documents only). A list is
     * constructed from these which is used to create the a new style selector which collates all of the stylesheets
     * found and is used to calculate the derived styles for all rendering objects.
     *
     * @param shallow If the stylesheet list for the document is unchanged, with only added or removed rules
     * in existing sheets, then set this argument to true for efficiency.
     */
    void updateStyleSelector(bool shallow=false);
    
    void ensureStyleSheetListUpToDate() { if (m_styleSheetListDirty) rebuildStyleSheetList(true); }

    bool readyForLayout() const;

    // DOM representation of the JS Window object, for event handling
    WindowEventTargetImpl* windowEventTarget() const { return m_windowEventTarget; }
private:    
    void rebuildStyleSheetList(bool force = false);
    void rebuildStyleSelector ();
    bool m_styleSheetListDirty;
public:

    // Tries to restore the elements value from the doc state,
    // if it seems like the same thing
    void attemptRestoreState(NodeImpl* e);

    // Query all registered elements for their state
    QStringList docState();
    bool unsubmittedFormChanges();
    void registerMaintainsState(NodeImpl* e) { m_maintainsState.append(e); }
    void deregisterMaintainsState(NodeImpl* e) { int i; if ((i = m_maintainsState.indexOf(e)) != -1) m_maintainsState.removeAt(i); }

    // Set the state the document should restore to
    void setRestoreState( const QStringList &s);

    KHTMLView *view() const;
    KHTMLPart* part() const;

    RangeImpl *createRange();

    NodeIteratorImpl *createNodeIterator(NodeImpl *root, unsigned long whatToShow,
                                    NodeFilterImpl* filter, bool entityReferenceExpansion, int &exceptioncode);

    TreeWalkerImpl *createTreeWalker(NodeImpl *root, unsigned long whatToShow, NodeFilterImpl *filter,
                            bool entityReferenceExpansion, int &exceptioncode);

    EditingTextImpl *createEditingTextNode(const DOMString &text);

    virtual void recalcStyle( StyleChange = NoChange );
    virtual void updateRendering();
    void updateLayout();
    static void updateDocumentsRendering();
    khtml::DocLoader *docLoader() { return m_docLoader; }

    virtual void attach();
    virtual void detach();

    khtml::RenderArena* renderArena() { return m_renderArena.get(); }

    // to get visually ordered hebrew and arabic pages right
    void setVisuallyOrdered();
    // to get URL decoding right
    //void setDecoderCodec(const QTextCodec *codec);

    // ### elide the two after designMode merge
    void setSelection(NodeImpl* s, int sp, NodeImpl* e, int ep);
    void clearSelection();
    void updateSelection();

    void open ( bool clearEventListeners = true );
    virtual void close (  );
    virtual void contentLoaded() {}
    void write ( const DOMString &text );
    void write ( const QString &text );
    void writeln ( const DOMString &text );
    void finishParsing (  );

    QUrl URL() const { return m_url; }
    void setURL(const QString& url) { m_url = QUrl(url); }

    QUrl baseURL() const { return m_baseURL.isEmpty() ? m_url : m_baseURL; }
    void setBaseURL(const QUrl& baseURL);

    QString baseTarget() const { return m_baseTarget; }
    void setBaseTarget(const QString& baseTarget) { m_baseTarget = baseTarget; }

    QString completeURL(const QString& url) const;
    DOMString canonURL(const DOMString& url) const { return url.isEmpty() ? url : completeURL(url.string()); }

    void setUserStyleSheet(const QString& sheet);
    QString userStyleSheet() const { return m_usersheet; }
    void setPrintStyleSheet(const QString& sheet) { m_printSheet = sheet; }
    QString printStyleSheet() const { return m_printSheet; }

    CSSStyleSheetImpl* elementSheet();
    virtual khtml::Tokenizer *createTokenizer();
    khtml::Tokenizer *tokenizer() { return m_tokenizer; }
    KEncodingDetector* decoder() { return m_decoder; }
    void setDecoder(KEncodingDetector* enc) { m_decoder = enc; }

    void setPaintDevice(QPaintDevice *dev){m_paintDevice = dev;}
    QPaintDevice *paintDevice() const {return m_paintDevice;}
    int logicalDpiY();

    enum HTMLMode {
        Html3 = 0,
        Html4 = 1,
        XHtml = 2
    };

    enum ParseMode {
        Unknown,
        Compat,
        Transitional,
        Strict
    };
    virtual void determineParseMode();
    void setParseMode( ParseMode m ) { pMode = m; }
    ParseMode parseMode() const { return pMode; }

    bool inCompatMode() const { return pMode == Compat; }
    bool inTransitionalMode() const { return pMode == Transitional; }
    bool inStrictMode() const { return pMode == Strict; }

    //void setHTMLMode( HTMLMode m ) { hMode = m; }
    HTMLMode htmlMode() const { return hMode; }

    void setParsing(bool b) { m_bParsing = b; }
    bool parsing() const { return m_bParsing; }

    void setHasVariableLength(bool b=true) { m_bVariableLength = b; }
    bool hasVariableLength() const { return m_bVariableLength; }

    void setTextColor( QColor color ) { m_textColor = color; }
    QColor textColor() const { return m_textColor; }

    void setDesignMode(bool b);
    bool designMode() const;

    // internal
    bool prepareMouseEvent( bool readonly, int x, int y, MouseEvent *ev );

    virtual bool childTypeAllowed( unsigned short nodeType );
    virtual WTF::PassRefPtr<NodeImpl> cloneNode ( bool deep );

    StyleSheetListImpl* styleSheets() { return m_styleSheets; }

    DOMString preferredStylesheetSet() const { return m_preferredStylesheetSet; }
    DOMString selectedStylesheetSet() const;
    void setSelectedStylesheetSet(const DOMString&);
    void setPreferredStylesheetSet(const DOMString& s) { m_preferredStylesheetSet = s; }

    void addStyleSheet(StyleSheetImpl *, int *exceptioncode = 0);
    void removeStyleSheet(StyleSheetImpl *, int *exceptioncode = 0);

    QStringList availableStyleSheets() const { return m_availableSheets; }

    NodeImpl* hoverNode() const { return m_hoverNode; }
    void setHoverNode(NodeImpl *newHoverNode);
    NodeImpl *focusNode() const { return m_focusNode; }
    void quietResetFocus(); // Removes focus from active node without attempting to emit any events
    void setFocusNode(NodeImpl *newFocusNode);
    NodeImpl* activeNode() const { return m_activeNode; }
    void setActiveNode(NodeImpl *newActiveNode);

    // Updates for :target (CSS3 selector).
    void setCSSTarget(NodeImpl* n);
    NodeImpl* getCSSTarget() { return m_cssTarget; }

    bool isDocumentChanged()	{ return m_docChanged; }
    virtual void setDocumentChanged(bool = true);
    void attachNodeIterator(NodeIteratorImpl *ni);
    void detachNodeIterator(NodeIteratorImpl *ni);
    void notifyBeforeNodeRemoval(NodeImpl *n);
    AbstractViewImpl *defaultView() const { return m_defaultView; }
    EventImpl *createEvent(const DOMString &eventType, int &exceptioncode);

    // keep track of what types of event listeners are registered, so we don't
    // dispatch events unnecessarily
    enum ListenerType {
        DOMSUBTREEMODIFIED_LISTENER          = 0x01,
        DOMNODEINSERTED_LISTENER             = 0x02,
        DOMNODEREMOVED_LISTENER              = 0x04,
        DOMNODEREMOVEDFROMDOCUMENT_LISTENER  = 0x08,
        DOMNODEINSERTEDINTODOCUMENT_LISTENER = 0x10,
        DOMATTRMODIFIED_LISTENER             = 0x20,
        DOMCHARACTERDATAMODIFIED_LISTENER    = 0x40
    };

    bool hasListenerType(ListenerType listenerType) const { return (m_listenerTypes & listenerType); }
    void addListenerType(ListenerType listenerType) { m_listenerTypes = m_listenerTypes | listenerType; }

    CSSStyleDeclarationImpl *getOverrideStyle(ElementImpl *elt, DOMStringImpl *pseudoElt);

    bool async() const { return m_async; }
    void setAsync(bool b) { m_async = b; }
    void abort();
    void load(const DOMString &uri);
    void loadXML(const DOMString &source);
    // from cachedObjectClient
    void setStyleSheet(const DOM::DOMString &url, const DOM::DOMString &sheet, const DOM::DOMString &charset, const DOM::DOMString &mimetype);
    void error(int err, const QString &text);

    typedef QMap<QString, ProcessingInstructionImpl*> LocalStyleRefs;
    LocalStyleRefs* localStyleRefs() { return &m_localStyleRefs; }

    virtual void defaultEventHandler(EventImpl *evt);

    void setHTMLWindowEventListener(EventName id, EventListener *listener);
    void setHTMLWindowEventListener(unsigned id, EventListener *listener);

    EventListener *getHTMLWindowEventListener(EventName id);
    EventListener *getHTMLWindowEventListener(unsigned id);

    EventListener *createHTMLEventListener(const QString& code, const QString& name, NodeImpl* node);

    void addWindowEventListener(EventName id, EventListener *listener, const bool useCapture);
    void removeWindowEventListener(EventName id, EventListener *listener, bool useCapture);
    bool hasWindowEventListener(EventName id);

    EventListener *createHTMLEventListener(QString code);

    /**
     * Searches through the document, starting from fromNode, for the next selectable element that comes after fromNode.
     * The order followed is as specified in section 17.11.1 of the HTML4 spec, which is elements with tab indexes
     * first (from lowest to highest), and then elements without tab indexes (in document order).
     *
     * @param fromNode The node from which to start searching. The node after this will be focused. May be null.
     *
     * @return The focus node that comes after fromNode
     *
     * See http://www.w3.org/TR/html4/interact/forms.html#h-17.11.1
     */
    NodeImpl *nextFocusNode(NodeImpl *fromNode);

    /**
     * Searches through the document, starting from fromNode, for the previous selectable element (that comes _before_)
     * fromNode. The order followed is as specified in section 17.11.1 of the HTML4 spec, which is elements with tab
     * indexes first (from lowest to highest), and then elements without tab indexes (in document order).
     *
     * @param fromNode The node from which to start searching. The node before this will be focused. May be null.
     *
     * @return The focus node that comes before fromNode
     *
     * See http://www.w3.org/TR/html4/interact/forms.html#h-17.11.1
     */
    NodeImpl *previousFocusNode(NodeImpl *fromNode);

    ElementImpl* findAccessKeyElement(QChar c);

    int nodeAbsIndex(NodeImpl *node);
    NodeImpl *nodeWithAbsIndex(int absIndex);

    /**
     * Handles a HTTP header equivalent set by a meta tag using <meta http-equiv="..." content="...">. This is called
     * when a meta tag is encountered during document parsing, and also when a script dynamically changes or adds a meta
     * tag. This enables scripts to use meta tags to perform refreshes and set expiry dates in addition to them being
     * specified in a HTML file.
     *
     * @param equiv The http header name (value of the meta tag's "equiv" attribute)
     * @param content The header value (value of the meta tag's "content" attribute)
     */
    void processHttpEquiv(const DOMString &equiv, const DOMString &content);

    void dispatchImageLoadEventSoon(HTMLImageElementImpl *);
    void dispatchImageLoadEventsNow();
    void removeImage(HTMLImageElementImpl *);
    virtual void timerEvent(QTimerEvent *);

    // Returns the owning element in the parent document.
    // Returns 0 if this is the top level document.
    HTMLPartContainerElementImpl *ownerElement() const;

    khtml::SecurityOrigin* origin() const;
    void setOrigin(khtml::SecurityOrigin*);
    
    // These represent JS operations on domain strings, rather than full-blown origins.
    // (so no port, protocol, etc.)
    void setDomain( const DOMString &newDomain ); 
    DOMString domain() const;

    bool isURLAllowed(const QString& url) const;

    HTMLElementImpl* body() const;

    DOMString toString() const;
 
    bool execCommand(const DOMString &command, bool userInterface, const DOMString &value);
    bool queryCommandEnabled(const DOMString &command);
    bool queryCommandIndeterm(const DOMString &command);
    bool queryCommandState(const DOMString &command);
    bool queryCommandSupported(const DOMString &command);
    DOMString queryCommandValue(const DOMString &command);
    
    
    // We version the tree to help determine which collection caches are 
    // valid. All collections depend on the structural changes; and may depend 
    // on some set of attributes.
    enum TreeVersion {
        TV_Structural,
        TV_IDNameHref,
        TV_Class,
        NumTreeVersions
    };
    
    void incDOMTreeVersion(unsigned ver) { ++m_domTreeVersions[ver]; }
    unsigned int domTreeVersion(unsigned ver) const { return m_domTreeVersions[ver]; }

    // Since applications often re-creat nodelists all over the place, we cache 
    // their caches in the documents. For now, we only do it for things that can be 
    // parametrices by type + base node.
    DynamicNodeListImpl::Cache* acquireCachedNodeListInfo(DynamicNodeListImpl::CacheFactory* fact,
                                                          NodeImpl* base, int type);
    void releaseCachedNodeListInfo(DynamicNodeListImpl::Cache* cache);

    JSEditor *jsEditor();

    QHash<DOMString,khtml::CounterNode*>* counters(const khtml::RenderObject* o) { return m_counterDict.value(o); }
    void setCounters(const khtml::RenderObject* o, QHash<DOMString,khtml::CounterNode*> *dict) { m_counterDict.insert(o, dict);}
    void removeCounters(const khtml::RenderObject* o) { delete m_counterDict.take(o); }

    ElementMappingCache& underDocNamedCache() {
        return m_underDocNamedCache;
    }
    
    ElementMappingCache& getElementByIdCache() const {
        return m_getElementByIdCache;
    }

    DOMString contentLanguage() const { return m_contentLanguage; }
    void setContentLanguage(const QString& cl) { m_contentLanguage = cl; }

    khtml::DynamicDomRestyler& dynamicDomRestyler() { return *m_dynamicDomRestyler; }
    const khtml::DynamicDomRestyler& dynamicDomRestyler() const { return *m_dynamicDomRestyler; }

    // WebCore compatibility
    const WebCore::SVGDocumentExtensions* svgExtensions();
    WebCore::SVGDocumentExtensions* accessSVGExtensions();

Q_SIGNALS:
    void finishedParsing();

protected:
    khtml::CSSStyleSelector *m_styleSelector;
    KHTMLView *m_view;
    QStringList m_state;
    int         m_stateRestorePos;

    khtml::DocLoader *m_docLoader;
    khtml::Tokenizer *m_tokenizer;
    KEncodingDetector *m_decoder;
    QUrl m_url;
    QUrl m_baseURL;
    QString m_baseTarget;

    mutable DocumentTypeImpl *m_doctype;

    mutable DOMImplementationImpl *m_implementation; // lazily created

    QString m_usersheet;
    QString m_printSheet;
    QStringList m_availableSheets;

    DOMString m_contentLanguage;

    // Track the number of currently loading top-level stylesheets.  Sheets
    // loaded using the @import directive are not included in this count.
    // We use this count of pending sheets to detect when we can begin attaching
    // elements.
    int m_pendingStylesheets;
    bool m_ignorePendingStylesheets;

    CSSStyleSheetImpl *m_elemSheet;

    QPaintDevice *m_paintDevice;
    ParseMode pMode;
    HTMLMode hMode;

    QColor m_textColor;
    NodeImpl *m_hoverNode;
    NodeImpl *m_focusNode;
    NodeImpl *m_activeNode;
    NodeImpl *m_cssTarget;

    unsigned int m_domTreeVersions[NumTreeVersions];

    WebCore::SVGDocumentExtensions* m_svgExtensions;

    QList<NodeIteratorImpl*> m_nodeIterators;
    AbstractViewImpl *m_defaultView;

    unsigned short m_listenerTypes;
    StyleSheetListImpl* m_styleSheets;
    StyleSheetListImpl *m_addedStyleSheets; // programmatically added style sheets
    LocalStyleRefs m_localStyleRefs; // references to inlined style elements
    WindowEventTargetImpl* m_windowEventTarget;
    RegisteredListenerList m_windowEventListeners;
    QList<NodeImpl*> m_maintainsState;

    // ### evaluate for placement in RenderStyle
    QHash<const khtml::RenderObject*,QHash<DOMString,khtml::CounterNode*> *> m_counterDict;

    khtml::DynamicDomRestyler *m_dynamicDomRestyler;

    bool visuallyOrdered;
    bool m_bParsing;
    bool m_docChanged;
    bool m_styleSelectorDirty;
    bool m_inStyleRecalc;
    bool m_async;
    bool m_hadLoadError;
    bool m_docLoading;
    bool m_bVariableLength;

    QEventLoop* m_inSyncLoad;

    DOMString m_title;
    DOMString m_preferredStylesheetSet;
    khtml::CachedCSSStyleSheet *m_loadingXMLDoc;

    mutable ElementImpl* m_documentElement;

    //int m_decoderMibEnum;

    //Forms, images, etc., must be quickly accessible via document.name.
    ElementMappingCache m_underDocNamedCache;

    //Cache for nodelists and collections.
    QHash<long,DynamicNodeListImpl::Cache*> m_nodeListCache;

    QLinkedList<HTMLImageElementImpl*> m_imageLoadEventDispatchSoonList;
    QLinkedList<HTMLImageElementImpl*> m_imageLoadEventDispatchingList;
    int m_imageLoadEventTimer;

    //Cache for getElementById
    mutable ElementMappingCache m_getElementByIdCache;

    SharedPtr<khtml::RenderArena> m_renderArena;
private:
    JSEditor *m_jsEditor;
    mutable RefPtr<khtml::SecurityOrigin> m_origin;

    int m_selfOnlyRefCount;
public:
    // Nodes belonging to this document hold "self-only" references -
    // these are enough to keep the document from being destroyed, but
    // not enough to keep it from removing its children. This allows a
    // node that outlives its document to still have a valid document
    // pointer without introducing reference cycles

    void selfOnlyRef() { ++m_selfOnlyRefCount; }
    void selfOnlyDeref() {
        --m_selfOnlyRefCount;
        if (!m_selfOnlyRefCount && !refCount())
            delete this;
    }

    // This is called when our last outside reference dies
    virtual void removedLastRef();
};

/*
 * This represent the Window object at the DOM level --- for now it only plays
 * the role of an event dispatch target, and isn't accessible directly
 * (it turns into Window in JS land)
 */
class WindowEventTargetImpl : public EventTargetImpl
{
public:
    WindowEventTargetImpl(DOM::DocumentImpl* owner);
    
    virtual Type eventTargetType() const;
    virtual DocumentImpl* eventTargetDocument();    
    KJS::Window* window();
private:
    DOM::DocumentImpl* m_owner;
};

class DocumentFragmentImpl : public NodeBaseImpl
{
public:
    DocumentFragmentImpl(DocumentImpl *doc);

    // DOM methods overridden from  parent classes
    virtual DOMString nodeName() const;
    virtual unsigned short nodeType() const;
    virtual WTF::PassRefPtr<NodeImpl> cloneNode ( bool deep );

    // Other methods (not part of DOM)
    virtual bool childTypeAllowed( unsigned short type );

    virtual DOMString toString() const;
};


class DocumentTypeImpl : public NodeImpl
{
public:
    DocumentTypeImpl(DOMImplementationImpl *_implementation, DocumentImpl *doc,
                     const DOMString &qualifiedName, const DOMString &publicId,
                     const DOMString &systemId);
    ~DocumentTypeImpl();

    // DOM methods & attributes for DocumentType
    NamedNodeMapImpl *entities() const;
    NamedNodeMapImpl *notations() const;

    DOMString name() const { return m_qualifiedName; }
    DOMString publicId() const { return m_publicId; }
    DOMString systemId() const { return m_systemId; }
    DOMString internalSubset() const { return m_subset; }

    // DOM methods overridden from  parent classes
    virtual DOMString nodeName() const;
    virtual unsigned short nodeType() const;
    virtual bool childTypeAllowed( unsigned short type );
    virtual WTF::PassRefPtr<NodeImpl> cloneNode ( bool deep );

    // Other methods (not part of DOM)
    void setName(const DOMString& n) { m_qualifiedName = n; }
    void setPublicId(const DOMString& publicId) { m_publicId = publicId; }
    void setSystemId(const DOMString& systemId) { m_systemId = systemId; }
    void setInternalSubset(const DOMString& subset) { m_subset = subset; }
    DOMImplementationImpl *implementation() const { return m_implementation; }

    virtual DOMString toString() const;

protected:
    DOMImplementationImpl *m_implementation;
    mutable NamedNodeMapImpl* m_entities;
    mutable NamedNodeMapImpl* m_notations;

    DOMString m_qualifiedName;
    DOMString m_publicId;
    DOMString m_systemId;
    DOMString m_subset;
};

class XMLDocumentImpl : public DocumentImpl
{
public:
    XMLDocumentImpl(KHTMLView *v) : DocumentImpl(v) { }

    virtual void close (  );
};

} //namespace
#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
