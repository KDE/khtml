/* This file is part of the KDE project

   Copyright (C) 1997 Martin Jones (mjones@kde.org)
             (C) 1998 Waldo Bastian (bastian@kde.org)
             (C) 1998, 1999 Torben Weis (weis@kde.org)
             (C) 1999 Lars Knoll (knoll@kde.org)
             (C) 1999 Antti Koivisto (koivisto@kde.org)
             (C) 2006 Germain Garand (germain@ebooksfrance.org)

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KHTMLVIEW_H
#define KHTMLVIEW_H

#include <khtml_export.h>

// qt includes and classes
#include <QScrollArea>

class QPainter;
class QRect;
template< typename T > class QVector;
template <class T> class QStack;

namespace DOM {
    class HTMLDocumentImpl;
    class DocumentImpl;
    class ElementImpl;
    class HTMLTitleElementImpl;
    class HTMLGenericFormElementImpl;
    class HTMLFormElementImpl;
    class HTMLAnchorElementImpl;
    class HTMLInputElementImpl;
    class NodeImpl;
    class CSSProperty;
}

namespace KJS {
    class WindowFunc;
    class ExternalFunc;
}

namespace khtml {
    class RenderObject;
    class RenderCanvas;
        class RenderLineEdit;
    class RenderPartObject;
    class RenderWidget;
    class RenderLayer;
    class RenderBox;
    class CSSStyleSelector;
    class LineEditWidget;
    class CaretBox;
    class HTMLTokenizer;
    class KHTMLWidgetPrivate;
    class KHTMLWidget
    {
    public:
        KHTMLWidget();
        ~KHTMLWidget();
        KHTMLWidgetPrivate* m_kwp;
    };
    void applyRule(DOM::CSSProperty *prop);
}

class KHTMLPart;
class KHTMLViewPrivate;

namespace khtml {

}

/**
 * Renders and displays HTML in a QScrollArea.
 *
 * Suitable for use as an application's main view.
 **/
class KHTML_EXPORT KHTMLView : public QScrollArea, public khtml::KHTMLWidget
{
    Q_OBJECT

    friend class DOM::HTMLDocumentImpl;
    friend class DOM::HTMLTitleElementImpl;
    friend class DOM::HTMLGenericFormElementImpl;
    friend class DOM::HTMLFormElementImpl;
    friend class DOM::HTMLAnchorElementImpl;
    friend class DOM::HTMLInputElementImpl;
    friend class DOM::NodeImpl;
    friend class DOM::ElementImpl;
    friend class DOM::DocumentImpl;
    friend class KHTMLPart;
    friend class KHTMLFind;
    friend class StorePass;
    friend class khtml::RenderCanvas;
    friend class khtml::RenderObject;
    friend class khtml::RenderLineEdit;
    friend class khtml::RenderPartObject;
    friend class khtml::RenderWidget;
    friend class khtml::KHTMLWidgetPrivate;
    friend class khtml::RenderLayer;
    friend class khtml::RenderBox;
    friend class khtml::CSSStyleSelector;
    friend class khtml::LineEditWidget;
    friend class khtml::HTMLTokenizer;
    friend class KJS::WindowFunc;
    friend class KJS::ExternalFunc;
    friend void khtml::applyRule(DOM::CSSProperty *prop);


public:
    /**
     * Constructs a KHTMLView.
     */
    KHTMLView( KHTMLPart *part, QWidget *parent );
    virtual ~KHTMLView();

    /**
     * Returns a pointer to the KHTMLPart that is
     * rendering the page.
     **/
    KHTMLPart *part() const { return m_part; }

    int frameWidth() const { return _width; }

    /**
     * Sets a margin in x direction.
     */
    void setMarginWidth(int x);

    /**
     * Returns the margin width.
     *
     * A return value of -1 means the default value will be used.
     */
    int marginWidth() const { return _marginWidth; }

    /*
     * Sets a margin in y direction.
     */
    void setMarginHeight(int y);

    /**
     * Returns the margin height.
     *
     * A return value of -1 means the default value will be used.
     */
    int marginHeight() { return _marginHeight; }

    /**
     * Sets vertical scrollbar mode.
     *
     * WARNING: do not call this method on a base class pointer unless you
     *          specifically want QAbstractScrollArea's variant (not recommended).
     *          QAbstractScrollArea::setVerticalScrollBarPolicy is *not* virtual.
     */
    virtual void setVerticalScrollBarPolicy( Qt::ScrollBarPolicy policy );

    /**
     * Sets horizontal scrollbar mode.
     *
     * WARNING: do not call this method on a base class pointer unless you
     *          specifically want QAbstractScrollArea's variant (not recommended).
     *          QAbstractScrollArea::setHorizontalScrollBarPolicy is *not* virtual.
     */
    virtual void setHorizontalScrollBarPolicy( Qt::ScrollBarPolicy policy );

    /**
     * Prints the HTML document.
     * @param quick if true, fully automated printing, without print dialog
     */
    void print( bool quick = false );

    /**
     * Display all accesskeys in small tooltips
     */
    void displayAccessKeys();

    /**
     * Returns the contents area's width
     */
    int contentsWidth() const;

    /**
     * Returns the contents area's height
     */
    int contentsHeight() const;

    /**
     * Returns the x coordinate of the contents area point
     * that is currently located at the top left in the viewport
     */
    int contentsX() const;

    /**
     * Returns the y coordinate of the contents area point
     * that is currently located at the top left in the viewport
     */
    int contentsY() const;

    /**
     * Returns the width of the viewport
     */
    int visibleWidth() const;

    /**
     * Returns the height of the viewport
     */
    int visibleHeight() const;

    /**
     *  Place the contents area point x/y
     *  at the top left of the viewport
     */
    void setContentsPos(int x, int y);

    /**
     * Returns a point translated to viewport coordinates
     * @param p the contents area point to translate
     *
     */
    QPoint contentsToViewport(const QPoint& p) const;

    /**
     * Returns a point translated to contents area coordinates
     * @param p the viewport point to translate
     *
     */
    QPoint viewportToContents(const QPoint& p) const;

    /**
     * Returns a point translated to contents area coordinates
     * @param x x coordinate of viewport point to translate
     * @param y y coordinate of viewport point to translate
     * @param cx resulting x coordinate
     * @param cy resulting y coordinate
     *
     */
    void viewportToContents(int x, int y, int& cx, int& cy) const;

    /**
     * Returns a point translated to viewport coordinates
     * @param x x coordinate of contents area point to translate
     * @param y y coordinate of contents area point to translate
     * @param cx resulting x coordinate
     * @param cy resulting y coordinate
     *
     */
    void contentsToViewport(int x, int y, int& cx, int& cy) const;

    /**
     * Scrolls the content area by a given amount
     * @param x x offset
     * @param y y offset
     */
    void scrollBy(int x, int y);

    /**
     * Requests an update of the content area
     * @param r the content area rectangle to update
     */
    void updateContents( const QRect& r );
    void updateContents(int x, int y, int w, int h);

    void addChild(QWidget *child, int dx, int dy);

    /**
     * Requests an immediate repaint of the content area
     * @param r the content area rectangle to repaint
     */
    void repaintContents( const QRect& r );
    void repaintContents(int x, int y, int w, int h);

    /**
     * Apply a zoom level to the content area
     * @param percent a zoom level expressed as a percentage
     */
    void setZoomLevel( int percent );

    /**
     * Retrieve the current zoom level
     *
     */
    int zoomLevel() const;

    /**
     * Smooth Scrolling Mode enumeration
     * @li SSMDisabled smooth scrolling is disabled
     * @li SSMWhenEfficient only use smooth scrolling on pages that do not require a full repaint of the content area when scrolling
     * @li SSMAlways smooth scrolling is performed unconditionally
     */
    enum SmoothScrollingMode { SSMDisabled = 0, SSMWhenEfficient, SSMEnabled };

    /**
     * Set the smooth scrolling mode.
     *
     * Smooth scrolling mode is normally controlled by the configuration file's SmoothScrolling key.
     * Using this setter will override the configuration file's settings.
     *
     * @since 4.1
     */
    void setSmoothScrollingMode( SmoothScrollingMode m );

    /**
     * Retrieve the current smooth scrolling mode
     *
     * @since 4.1
     */
    SmoothScrollingMode smoothScrollingMode() const;

public Q_SLOTS:
    /**
     * Resize the contents area
     * @param w the new width
     * @param h the new height
     */
    virtual void resizeContents(int w, int h);

    /**
     * ensure the display is up to date
     */
    void layout();


Q_SIGNALS:
    /**
     * This signal is used for internal layouting. Don't use it to check if rendering finished.
     * Use @ref KHTMLPart completed() signal instead.
     */
    void finishedLayout();
    void cleared();
    void zoomView( int );
    void hideAccessKeys();
    void repaintAccessKeys();
    void findAheadActive( bool );

protected:
    void clear();

    virtual bool event ( QEvent * event );
    virtual void paintEvent( QPaintEvent * );
    virtual void resizeEvent ( QResizeEvent * event );
    virtual void showEvent ( QShowEvent * );
    virtual void hideEvent ( QHideEvent *);
    virtual bool focusNextPrevChild( bool next );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
    virtual void mouseDoubleClickEvent( QMouseEvent * );
    virtual void mouseMoveEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    virtual void wheelEvent(QWheelEvent*);
#endif
    virtual void dragEnterEvent( QDragEnterEvent* );
    virtual void dropEvent( QDropEvent* );
    virtual void closeEvent ( QCloseEvent * );
    virtual bool widgetEvent( QEvent * );
    virtual bool viewportEvent( QEvent * e );
    virtual bool eventFilter(QObject *, QEvent *);
    virtual void scrollContentsBy( int dx, int dy );

    void keyPressEvent( QKeyEvent *_ke );
    void keyReleaseEvent ( QKeyEvent *_ke );
    void doAutoScroll();
    void timerEvent ( QTimerEvent * );

    void setSmoothScrollingModeDefault( SmoothScrollingMode m );

protected Q_SLOTS:
    void slotPaletteChanged();

private Q_SLOTS:
    void tripleClickTimeout();
    void accessKeysTimeout();
    void scrollTick();

    /**
    * @internal
    * used for autoscrolling with MMB
    */
    void slotMouseScrollTimer();

private:
    void resizeContentsToViewport();

    void scheduleRelayout(khtml::RenderObject* clippedObj=0);
    void unscheduleRelayout();

    bool hasLayoutPending();

    void scheduleRepaint(int x, int y, int w, int h, bool asap=false);
    void unscheduleRepaint();

    bool needsFullRepaint() const;

    void closeChildDialogs();
    bool dialogsAllowed();

    void setMouseEventsTarget( QWidget* w );
    QWidget* mouseEventsTarget() const;

    QStack<QRegion>* clipHolder() const;
    void setClipHolder( QStack<QRegion>* ch );

    void setPart(KHTMLPart *part);

    /**
     * Paints the HTML document to a QPainter.
     * The document will be scaled to match the width of
     * rc and clipped to fit in the height.
     * yOff determines the vertical offset in the document to start with.
     * more, if nonzero will be set to true if the documents extends
     * beyond the rc or false if everything below yOff was painted.
     **/
    void paint(QPainter *p, const QRect &rc, int yOff = 0, bool *more = 0);

    void render(QPainter *p, const QRect& r, const QPoint& off);

    /**
     * Get/set the CSS Media Type.
     *
     * Media type is set to "screen" for on-screen rendering and "print"
     * during printing. Other media types lack the proper support in the
     * renderer and are not activated. The DOM tree and the parser itself,
     * however, properly handle other media types. To make them actually work
     * you only need to enable the media type in the view and if necessary
     * add the media type dependent changes to the renderer.
     */
    void setMediaType( const QString &medium );
    QString mediaType() const;

    bool pagedMode() const;

    bool scrollTo(const QRect &);

    bool focusNextPrevNode(bool next);
    bool handleAccessKey(const QKeyEvent* ev);
    bool focusNodeWithAccessKey(QChar c, KHTMLView* caller = NULL);
    QMap< DOM::ElementImpl*, QChar > buildFallbackAccessKeys() const;
    void displayAccessKeys( KHTMLView* caller, KHTMLView* origview, QVector< QChar >& taken, bool use_fallbacks );
    bool isScrollingFromMouseWheel() const;
    void setHasStaticBackground(bool partial=false);
    void setHasNormalBackground();
    void addStaticObject(bool fixed);
    void removeStaticObject(bool fixed);
    void applyTransforms( int& x, int& y, int& w, int& h) const;
    void revertTransforms( int& x, int& y, int& w, int& h) const;
    void revertTransforms( int& x, int& y ) const;
    void checkExternalWidgetsPosition();

    void setIgnoreWheelEvents(bool e);

    void initWidget();

    DOM::NodeImpl *nodeUnderMouse() const;
    DOM::NodeImpl *nonSharedNodeUnderMouse() const;

    void restoreScrollBar();

    QStringList formCompletionItems(const QString &name) const;
    void clearCompletionHistory(const QString& name);
    void addFormCompletionItem(const QString &name, const QString &value);

    void addNonPasswordStorableSite( const QString& host );
    void delNonPasswordStorableSite( const QString& host );
    bool nonPasswordStorableSite( const QString& host ) const;

    bool dispatchMouseEvent(int eventId, DOM::NodeImpl *targetNode,
			    DOM::NodeImpl *targetNodeNonShared, bool cancelable,
			    int detail,QMouseEvent *_mouse, bool setUnder,
			    int mouseEventType, int orientation=0);
    bool dispatchKeyEvent( QKeyEvent *_ke );
    bool dispatchKeyEventHelper( QKeyEvent *_ke, bool generate_keypress );

    void complete( bool pendingAction );

    void updateScrollBars();
    void setupSmoothScrolling(int dx, int dy);

    /**
     * Returns the current caret policy when the view is not focused.
     * @return a KHTMLPart::CaretDisplay value
     */
    int caretDisplayPolicyNonFocused() const;

    /**
     * Sets the caret display policy when the view is not focused.
     * @param policy new display policy as
     *		defined by KHTMLPart::CaretDisplayPolicy
     */
    void setCaretDisplayPolicyNonFocused(int policy);

    // -- caret event handler

    /**
     * Evaluates key presses for caret navigation on editable nodes.
     * @return true if event has been handled
     */
    bool caretKeyPressEvent(QKeyEvent *);

    // ------------------------------------- member variables ------------------------------------
 private:
    friend class KHTMLViewPrivate;
    enum LinkCursor { LINK_NORMAL, LINK_MAILTO, LINK_NEWWINDOW };

    void setWidgetVisible(::khtml::RenderWidget*, bool visible);


    int _width;
    int _height;

    int _marginWidth;
    int _marginHeight;

    KHTMLPart *m_part;
    KHTMLViewPrivate* const d;

    QString m_medium;   // media type
};

#endif

