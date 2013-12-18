// -*- c-basic-offset: 2 -*-
/* This file is part of the KDE project
 *
 * Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 *                     1999-2001 Lars Knoll <knoll@kde.org>
 *                     1999-2001 Antti Koivisto <koivisto@kde.org>
 *                     2000-2001 Simon Hausmann <hausmann@kde.org>
 *                     2000-2001 Dirk Mueller <mueller@kde.org>
 *                     2000 Stefan Schimanski <1Stein@gmx.de>
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
 */
#ifndef __khtml_part_h__
#define __khtml_part_h__

#include "dom/dom_doc.h"
#include "dom/dom2_range.h"

#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kfind.h>
#include <kfinddialog.h>
#include <klocalizedstring.h>
#include <kencodingdetector.h>
#include <kencodingprober.h>
#include <QtCore/QRegExp>
#include <QUrl>

class KHTMLPartPrivate;
class KHTMLPartBrowserExtension;
class KJSProxy;
class KHTMLView;
class KHTMLViewBar;
class KHTMLFindBar;
class KHTMLSettings;
class KJavaAppletContext;
class KJSErrorDlg;

namespace DOM
{
  class HTMLDocument;
  class HTMLDocumentImpl;
  class DocumentImpl;
  class Document;
  class XMLDocumentImpl;
  class HTMLTitleElementImpl;
  class HTMLFrameElementImpl;
  class HTMLIFrameElementImpl;
  class HTMLObjectElementImpl;
  class HTMLFormElementImpl;
  class HTMLAnchorElementImpl;
  class HTMLMetaElementImpl;
  class NodeImpl;
  class ElementImpl;
  class Node;
  class HTMLEventListener;
  class EventListener;
  class HTMLPartContainerElementImpl;
  class HTMLObjectBaseElementImpl;
  class Position;
  class Selection;
  class Range;
  class Editor;
}

namespace WebCore
{
    class SVGDocumentExtensions;
}

namespace KJS
{
  class Interpreter;
  class HTMLElement;
}

namespace khtml
{
  class DocLoader;
  class RenderPart;
  class ChildFrame;
  class MousePressEvent;
  class MouseDoubleClickEvent;
  class MouseMoveEvent;
  class MouseReleaseEvent;
  class DrawContentsEvent;
  class CachedObject;
  class RenderWidget;
  class RenderBlock;
  class CSSStyleSelector;
  class HTMLTokenizer;
  class XMLTokenizer;
  struct EditorContext;
  class EditCommandImpl;
  class KHTMLPartAccessor;
}

namespace KJS {
    class Window;
    class WindowFunc;
    class ExternalFunc;
    class JSEventListener;
    class JSLazyEventListener;
    class JSNodeFilter;
    class DOMDocument;
    class SourceFile;
    class ScheduledAction;
    class DOMSelection;
    class DOMSelectionProtoFunc;
    class KHTMLPartScriptable;
}

namespace KParts
{
  class PartManager;
  class ScriptableExtension;
}

namespace KWallet
{
  class Wallet;
}

/**
 * This class is khtml's main class. It features an almost complete
 * web browser, and html renderer.
 *
 * The easiest way to use this class (if you just want to display an HTML
 * page at some URL) is the following:
 *
 * \code
 * QUrl url = "http://www.kde.org";
 * KHTMLPart *w = new KHTMLPart();
 * w->openUrl(url);
 * w->view()->resize(500, 400);
 * w->show();
 * \endcode
 *
 * Java and JavaScript are enabled by default depending on the user's
 * settings. If you do not need them, and especially if you display
 * unfiltered data from untrusted sources, it is strongly recommended to
 * turn them off. In that case, you should also turn off the automatic
 * redirect and plugins:
 *
 * \code
 * w->setJScriptEnabled(false);
 * w->setJavaEnabled(false);
 * w->setMetaRefreshEnabled(false);
 * w->setPluginsEnabled(false);
 * \endcode
 *
 * You may also wish to disable external references.  This will prevent KHTML
 * from loading images, frames, etc,  or redirecting to external sites.
 *
 * \code
 * w->setOnlyLocalReferences(true);
 * \endcode
 *
 * Some apps want to write their HTML code directly into the widget instead of
 * opening an url. You can do this in the following way:
 *
 * \code
 * QString myHTMLCode = ...;
 * KHTMLPart *w = new KHTMLPart();
 * w->begin();
 * w->write(myHTMLCode);
 * ...
 * w->end();
 * \endcode
 *
 * You can do as many calls to write() as you wish.  There are two
 * write() methods, one accepting a QString and one accepting a
 * @p char @p * argument. You should use one or the other
 * (but not both) since the method using
 * the @p char @p * argument does an additional decoding step to convert the
 * written data to Unicode.
 *
 * It is also possible to write content to the HTML part using the
 * standard streaming API from KParts::ReadOnlyPart. The usage of
 * the API is similar to that of the begin(), write(), end() process
 * described above as the following example shows:
 *
 * \code
 * KHTMLPart *doc = new KHTMLPart();
 * doc->openStream( "text/html", QUrl() );
 * doc->writeStream( QCString( "<html><body><p>KHTML Rocks!</p></body></html>" ) );
 * doc->closeStream();
 * \endcode
 *
 * @short HTML Browser Widget
 * @author Lars Knoll (knoll@kde.org)
 *
 */
class KHTML_EXPORT KHTMLPart : public KParts::ReadOnlyPart
{
  Q_OBJECT
  friend class KHTMLView;
  friend class DOM::HTMLTitleElementImpl;
  friend class DOM::HTMLFrameElementImpl;
  friend class DOM::HTMLIFrameElementImpl;
  friend class DOM::HTMLObjectBaseElementImpl;
  friend class DOM::HTMLObjectElementImpl;
  friend class DOM::HTMLAnchorElementImpl;
  friend class DOM::HTMLMetaElementImpl;
  friend class DOM::NodeImpl;
  friend class DOM::ElementImpl;
  friend class KHTMLRun;
  friend class DOM::HTMLFormElementImpl;
  friend class KJS::Window;
  friend class KJS::ScheduledAction;
  friend class KJS::JSNodeFilter;
  friend class KJS::WindowFunc;
  friend class KJS::ExternalFunc;
  friend class KJS::JSEventListener;
  friend class KJS::JSLazyEventListener;
  friend class KJS::DOMDocument;
  friend class KJS::HTMLElement;
  friend class KJS::SourceFile;
  friend class KJS::DOMSelection;
  friend class KJS::DOMSelectionProtoFunc;
  friend class KJS::KHTMLPartScriptable;
  friend class KJSProxy;
  friend class KHTMLPartBrowserExtension;
  friend class DOM::DocumentImpl;
  friend class DOM::HTMLDocumentImpl;
  friend class DOM::Selection;
  friend class DOM::Editor;
  friend class KHTMLPartBrowserHostExtension;
  friend class khtml::HTMLTokenizer;
  friend class khtml::XMLTokenizer;
  friend class khtml::RenderWidget;
  friend class khtml::RenderBlock;
  friend class khtml::CSSStyleSelector;
  friend class khtml::EditCommandImpl;
  friend class khtml::KHTMLPartAccessor;
  friend class KHTMLPartIface;
  friend class KHTMLPartFunction;
  friend class KHTMLPopupGUIClient;
  friend class KHTMLFind;
  friend class StorePass;
  friend class WebCore::SVGDocumentExtensions;

  Q_PROPERTY( bool javaScriptEnabled READ jScriptEnabled WRITE setJScriptEnabled )
  Q_PROPERTY( bool javaEnabled READ javaEnabled WRITE setJavaEnabled )
  Q_PROPERTY( bool dndEnabled READ dndEnabled WRITE setDNDEnabled )
  Q_PROPERTY( bool pluginsEnabled READ pluginsEnabled WRITE setPluginsEnabled )
  Q_PROPERTY( DNSPrefetch dnsPrefetch READ dnsPrefetch WRITE setDNSPrefetch )

  /*
   *
   * Don't add setOnlyLocalReferences here. It shouldn't be accessible via DBus.
   *
   **/
  Q_PROPERTY( bool modified READ isModified )
  Q_PROPERTY( QString encoding READ encoding WRITE setEncoding )
  Q_PROPERTY( QString lastModified READ lastModified )
  Q_PROPERTY( bool metaRefreshEnabled READ metaRefreshEnabled WRITE setMetaRefreshEnabled )

public:
  enum GUIProfile { DefaultGUI, BrowserViewGUI /* ... */ };

     /**
     * DNS Prefetching Mode enumeration
     * @li DNSPrefetchDisabled do not prefetch hostnames
     * @li DNSPrefetchEnabled always prefetch hostnames
     * @li DNSPrefetchOnlyWWWAndSLD only do DNS prefetching for bare SLD and www sub-domain
     */

  enum DNSPrefetch {
       DNSPrefetchDisabled=0,
       DNSPrefetchEnabled,
       DNSPrefetchOnlyWWWAndSLD
  };

  /**
   * Constructs a new KHTMLPart.
   *
   * KHTML basically consists of two objects: The KHTMLPart itself,
   * holding the document data (DOM document), and the KHTMLView,
   * derived from QScrollArea, in which the document content is
   * rendered in. You can specify two different parent objects for a
   * KHTMLPart, one parent for the KHTMLPart document and one parent
   * for the KHTMLView. If the second @p parent argument is 0L, then
   * @p parentWidget is used as parent for both objects, the part and
   * the view.
   */
  KHTMLPart( QWidget *parentWidget = 0,
             QObject *parent = 0, GUIProfile prof = DefaultGUI );
  /**
   * Constructs a new KHTMLPart.
   *
   * This constructor is useful if you wish to subclass KHTMLView.
   * If the @p view passed  as first argument to the constructor was built with a
   * null KHTMLPart pointer, then the newly created KHTMLPart will be assigned as the view's part.
   *
   * Therefore, you might either initialize the view as part of the initialization list of
   * your derived KHTMLPart class constructor:
   * \code
   *   MyKHTMLPart() : KHTMLPart( new MyKHTMLView( this ), ...
   * \endcode
   * Or separately build the KHTMLView beforehand:
   * \code
   *   KHTMLView * v = KHTMLView( 0L, parentWidget());
   *   KHTMLPart * p = KHTMLPart( v ); // p will be assigned to v, so that v->part() == p
   * \endcode
   */
  KHTMLPart( KHTMLView *view, QObject *parent = 0, GUIProfile prof = DefaultGUI );

  /**
   * Destructor.
   */
  virtual ~KHTMLPart();

  /**
   * Opens the specified URL @p url.
   *
   * Reimplemented from KParts::ReadOnlyPart::openUrl .
   */
  virtual bool openUrl(const QUrl &url);

  /**
   * Stops loading the document and kills all data requests (for images, etc.)
   */
  virtual bool closeUrl();

  /**
   * Called when a certain error situation (i.e. connection timed out) occurred.
   * The default implementation either shows a KIO error dialog or loads a more
   * verbose error description a as page, depending on the users configuration.
   * @p job is the job that signaled the error situation
   */
  virtual void showError( KJob* job );

  /**
   * Returns a reference to the DOM HTML document (for non-HTML documents, returns null)
   */
  DOM::HTMLDocument htmlDocument() const;

  /**
   * Returns a reference to the DOM document.
   */
  DOM::Document document() const;

  /**
   * Returns the content of the source document.
   */
   QString documentSource() const;

  /**
   * Returns the node that has the keyboard focus.
   */
  DOM::Node activeNode() const;

  /**
   * Returns a pointer to the KParts::BrowserExtension.
   */
  KParts::BrowserExtension *browserExtension() const;
  KParts::BrowserHostExtension *browserHostExtension() const;

  /**
   * Returns a pointer to the HTML document's view.
   */
  KHTMLView *view() const;

  /**
   * Enable/disable Javascript support. Note that this will
   * in either case permanently override the default usersetting.
   * If you want to have the default UserSettings, don't call this
   * method.
   */
  void setJScriptEnabled( bool enable );

  /**
   * Returns @p true if Javascript support is enabled or @p false
   * otherwise.
   */
  bool jScriptEnabled() const;

  /**
   * Returns the JavaScript interpreter the part is using. This method is
   * mainly intended for applications which embed and extend the part and
   * provides a mechanism for adding additional native objects to the
   * interpreter (or removing the built-ins).
   *
   * One thing people using this method to add things to the interpreter must
   * consider, is that when you start writing new content to the part, the
   * interpreter is cleared. This includes both use of the
   * begin( const QUrl &, int, int ) method, and the openUrl( const QUrl & )
   * method. If you want your objects to have a longer lifespan, then you must
   * retain a KJS::Object yourself to ensure that the reference count of your
   * custom objects never reaches 0. You will also need to re-add your
   * bindings every time this happens - one way to detect the need for this is
   * to connect to the docCreated() signal, another is to reimplement the
   * begin() method.
   */
  KJS::Interpreter *jScriptInterpreter();

  /**
   * Enable/disable statusbar messages.
   * When this class wants to set the statusbar text, it emits
   * setStatusBarText(const QString & text)
   * If you want to catch this for your own statusbar, note that it returns
   * back a rich text string, starting with "<qt>".  This you need to
   * either pass this into your own QLabel or to strip out the tags
   * before passing it to QStatusBar::message(const QString & message)
   *
   * @see KParts::Part::setStatusBarText( const QString & text )
   */
  void setStatusMessagesEnabled( bool enable );

  /**
   * Returns @p true if status messages are enabled.
   */
  bool statusMessagesEnabled() const;

  /**
   * Enable/disable automatic forwarding by &lt;meta http-equiv="refresh" ....&gt;
   */
  void setMetaRefreshEnabled( bool enable );

  /**
   * Returns @p true if automatic forwarding is enabled.
   */
  bool metaRefreshEnabled() const;

  /**
   * Same as executeScript( const QString & ) except with the Node parameter
   * specifying the 'this' value.
   */
  QVariant executeScript( const DOM::Node &n, const QString &script );

  /**
   * Enables or disables Drag'n'Drop support. A drag operation is started if
   * the users drags a link.
   */
  void setDNDEnabled( bool b );

  /**
   * Returns whether Dragn'n'Drop support is enabled or not.
   */
  bool dndEnabled() const;

  /**
   * Enables/disables Java applet support. Note that calling this function
   * will permanently override the User settings about Java applet support.
   * Not calling this function is the only way to let the default settings
   * apply.
   */
  void setJavaEnabled( bool enable );

  /**
   * Return @p true if Java applet support is enabled, @p false if disabled
   */
  bool javaEnabled() const;

  /**
   * Enables or disables plugins, default is enabled
   */
  void setPluginsEnabled( bool enable );

  /**
   * Returns @p true if plugins are enabled, @p false if disabled.
   */
  bool pluginsEnabled() const;

  /**
   * Specifies whether images contained in the document should be loaded
   * automatically or not.
   *
   * @note Request will be ignored if called before begin().
   */
  void setAutoloadImages( bool enable );
  /**
   * Returns whether images contained in the document are loaded automatically
   * or not.
   * @note that the returned information is unrelieable as long as no begin()
   * was called.
   */
  bool autoloadImages() const;

  /**
   * Security option.
   *
   * Specify whether only file:/ or data:/ urls are allowed to be loaded without
   * user confirmation by KHTML.
   * ( for example referenced by stylesheets, images, scripts, subdocuments, embedded elements ).
   *
   * This option is mainly intended for enabling the "mail reader mode", where you load untrusted
   * content with a file:/ url.
   *
   * Please note that enabling this option currently automatically disables Javascript,
   * Java and Plugins support. This might change in the future if the security model
   * is becoming more sophisticated, so don't rely on this behaviour.
   *
   * ( default @p false - everything is loaded unless forbidden by KApplication::authorizeURLAction).
   */
  void setOnlyLocalReferences( bool enable );

  /**
   * Security option. If this is set to true, content loaded from external URLs
   * will be permitted to access images on disk regardless of of the Kiosk policy.
   * You should be careful in enabling this, as it may make it easier to fake
   * any HTML-based chrome or to perform other such user-confusion attack.
   * @p false by default.
   *
   * @since 4.6
   */
  void setForcePermitLocalImages( bool enable );

  /**
   * Sets whether DNS Names found in loaded documents'anchors should be pre-fetched (pre-resolved).
   * Note that calling this function will permanently override the User settings about
   * DNS prefetch support.
   * Not calling this function is the only way to let the default settings apply.
   *
   * @note This setting has no effect if @ref setOnlyLocalReferences() mode is enabled.
   *
   * @param pmode the mode to set. See @ref DNSPrefetch enum for explanation of values.
   *
   * @since 4.2
   */
  void setDNSPrefetch( DNSPrefetch pmode );

  /**
   * Returns currently set DNS prefetching mode.
   * See @p DNSPrefetch enum for explanation of values.
   *
   * @note Always returns  @p DNSPrefetchDisabled if @ref setOnlyLocalReferences() mode is enabled.
   *
   * @since 4.2
   */
  DNSPrefetch dnsPrefetch() const;

  /**
   * Returns whether only file:/ or data:/ references are allowed
   * to be loaded ( default @p false ).  See setOnlyLocalReferences.
   **/
  bool onlyLocalReferences() const;

  /**
   * If true, local image files will be loaded even when forbidden by the
   * Kiosk/KAuthorized policies ( default @p false ). See @ref setForcePermitLocalImages.
   *
   * @since 4.6
   **/
  bool forcePermitLocalImages() const;

  /** Returns whether caret mode is on/off.
   */
  bool isCaretMode() const;

  /**
   * Returns @p true if the document is editable, @p false otherwise.
   */
  bool isEditable() const;

  /**
   * Sets the caret to the given position.
   *
   * If the given location is invalid, it will snap to the nearest valid
   * location. Immediately afterwards a @p caretPositionChanged signal
   * containing the effective position is emitted
   * @param node node to set to
   * @param offset zero-based offset within the node
   * @param extendSelection If @p true, a selection will be spanned from the
   *	last caret position to the given one. Otherwise, any existing selection
   *	will be deselected.
   */
  void setCaretPosition(DOM::Node node, long offset, bool extendSelection = false);

  /**
   * Enumeration for displaying the caret.
   */
  enum CaretDisplayPolicy {
      CaretVisible, /**< caret is displayed */
      CaretInvisible, /**<  caret is not displayed */
      CaretBlink /**< caret toggles between visible and invisible */
  };

  /**
   * Returns the current caret policy when the view is not focused.
   */
  CaretDisplayPolicy caretDisplayPolicyNonFocused() const;

  /**
   * Sets the caret display policy when the view is not focused.
   *
   * Whenever the caret is in use, this property determines how the
   * caret should be displayed when the document view is not focused.
   *
   * The default policy is CaretInvisible.
   * @param policy new display policy
   */
  void setCaretDisplayPolicyNonFocused(CaretDisplayPolicy policy);

#ifndef KDE_NO_COMPAT
  QUrl baseURL() const;
#endif

  /**
   * Returns the URL for the background Image (used by save background)
   */
  QUrl backgroundURL() const;

  /**
   * Schedules a redirection after @p delay seconds.
   */
  void scheduleRedirection( int delay, const QString &url, bool lockHistory = true );

  /**
   * Clears the widget and prepares it for new content.
   *
   * If you want url() to return
   * for example "file:/tmp/test.html", you can use the following code:
   * \code
   * view->begin( QUrl("file:/tmp/test.html" ) );
   * \endcode
   *
   * @param url is the url of the document to be displayed.  Even if you
   * are generating the HTML on the fly, it may be useful to specify
   * a directory so that any pixmaps are found.
   *
   * @param xOffset is the initial horizontal scrollbar value. Usually
   * you don't want to use this.
   *
   * @param yOffset is the initial vertical scrollbar value. Usually
   * you don't want to use this.
   *
   * All child frames and the old document are removed if you call
   * this method.
   */
  virtual void begin( const QUrl &url = QUrl(), int xOffset = 0, int yOffset = 0 );

  /**
   * Writes another part of the HTML code to the widget.
   *
   * You may call
   * this function many times in sequence. But remember: The fewer calls
   * you make, the faster the widget will be.
   *
   * The HTML code is send through a decoder which decodes the stream to
   * Unicode.
   *
   * The @p len parameter is needed for streams encoded in utf-16,
   * since these can have \\0 chars in them. In case the encoding
   * you're using isn't utf-16, you can safely leave out the length
   * parameter.
   *
   * Attention: Don't mix calls to write( const char *) with calls
   * to write( const QString & ).
   *
   * The result might not be what you want.
   */
  virtual void write( const char *str, int len = -1 );

  /**
   * Writes another part of the HTML code to the widget.
   *
   * You may call
   * this function many times in sequence. But remember: The fewer calls
   * you make, the faster the widget will be.
   *
   * For historic and backward compatibility reasons, this method will force
   * the use of strict mode for the document, unless setAlwaysHonourDoctype()
   * has been called previously.
   */
   // FIXME KDE5: always honour doctype, remove setAlwaysHonourDoctype()
  virtual void write( const QString &str );

  /**
   * Call this after your last call to write().
   */
  virtual void end();

  /*
   * Prints the current HTML page laid out for the printer.
   *
   * (not implemented at the moment)
   */
  //    void print(QPainter *, int pageHeight, int pageWidth);

  /**
   * Paints the HTML page to a QPainter. See KHTMLView::paint for details
   */
  void paint( QPainter *, const QRect &, int = 0, bool * = 0 );

  /**
   * Sets the encoding the page uses.
   *
   * This can be different from the charset. The widget will try to reload the current page in the new
   * encoding, if url() is not empty.
   */
  bool setEncoding( const QString &name, bool override = false );

  /**
   * Returns the encoding the page currently uses.
   *
   * Note that the encoding might be different from the charset.
   */
  QString encoding() const;

  /**
   * Sets a user defined style sheet to be used on top of the HTML 4
   * default style sheet.
   *
   * This gives a wide range of possibilities to
   * change the layout of the page.
   *
   * To have an effect this function has to be called after calling begin().
   */
  void setUserStyleSheet( const QUrl &url );

  /**
   * Sets a user defined style sheet to be used on top of the HTML 4
   * default style sheet.
   *
   * This gives a wide range of possibilities to
   * change the layout of the page.
   *
   * To have an effect this function has to be called after calling begin().
   */
  void setUserStyleSheet( const QString &styleSheet );

public:

  /**
   * Sets the standard font style.
   *
   * @param name The font name to use for standard text.
   */
  void setStandardFont( const QString &name );

  /**
   * Sets the fixed font style.
   *
   * @param name The font name to use for fixed text, e.g.
   * the <tt>&lt;pre&gt;</tt> tag.
   */
  void setFixedFont( const QString &name );

  /**
   * Finds the anchor named @p name.
   *
   * If the anchor is found, the widget
   * scrolls to the closest position. Returns @p if the anchor has
   * been found.
   */
  bool gotoAnchor( const QString &name );

  /**
   * Go to the next anchor
   *
   * This is useful to navigate from outside the navigator
   */
  bool nextAnchor();

  /**
   * Go to previous anchor
   */
  bool prevAnchor();

  /**
   * Sets the cursor to use when the cursor is on a link.
   */
  void setURLCursor( const QCursor &c );

  /**
   * Returns the cursor which is used when the cursor is on a link.
   */
  QCursor urlCursor() const;

  /**
   * Extra Find options that can be used when calling the extended findText().
   */
  enum FindOptions
  {
  	FindLinksOnly   = 1 * KFind::MinimumUserOption,
  	FindNoPopups    = 2 * KFind::MinimumUserOption
  	//FindIncremental = 4 * KFind::MinimumUserOption
  };

  /**
   * Starts a new search by popping up a dialog asking the user what he wants to
   * search for.
   */
  void findText();

  /**
   * Starts a new search, but bypasses the user dialog.
   * @param str The string to search for.
   * @param options Find options.
   * @param parent Parent used for centering popups like "string not found".
   * @param findDialog Optionally, you can supply your own dialog.
   */
  void findText( const QString &str, long options, QWidget *parent = 0,
                 KFindDialog *findDialog = 0 );

  /**
   * Initiates a text search.
   */
  void findTextBegin();

  /**
   * Finds the next occurrence of a string set by @ref findText()
   * @param reverse if @p true, revert seach direction (only if no find dialog is used)
   * @return @p true if a new match was found.
   */
  bool findTextNext( bool reverse = false );

  /**
   * Sets the Zoom factor. The value is given in percent, larger values mean a
   * generally larger font and larger page contents.
   *
   * The given value should be in the range of 20..300, values outside that
   * range are not guaranteed to work. A value of 100 will disable all zooming
   * and show the page with the sizes determined via the given lengths in the
   * stylesheets.
   */
  void setZoomFactor(int percent);

  /**
   * Returns the current zoom factor.
   */
  int zoomFactor() const;

  /**
   * Sets the scale factor to be applied to fonts. The value is given in percent,
   * larger values mean generally larger fonts.
   *
   * The given value should be in the range of 20..300, values outside that
   * range are not guaranteed to work. A value of 100 will disable all scaling of font sizes
   * and show the page with the sizes determined via the given lengths in the
   * stylesheets.
   */
  void setFontScaleFactor(int percent);

  /**
   * Returns the current font scale factor.
   */
  int fontScaleFactor() const;

  /**
   * Returns the text the user has marked.
   */
  virtual QString selectedText() const;

  /**
   * Return the text the user has marked.  This is guaranteed to be valid xml,
   * and to contain the \<html> and \<body> tags.
   *
   * FIXME probably should make virtual for 4.0 ?
   */
  QString selectedTextAsHTML() const;

  /**
   * Returns the selected part of the HTML.
   */
  DOM::Range selection() const;

  /**
   * Returns the selected part of the HTML by returning the starting and end
   * position.
   *
   * If there is no selection, both nodes and offsets are equal.
   * @param startNode returns node selection starts in
   * @param startOffset returns offset within starting node
   * @param endNode returns node selection ends in
   * @param endOffset returns offset within end node.
   */
  void selection(DOM::Node &startNode, long &startOffset,
  		DOM::Node &endNode, long &endOffset) const;

  /**
   * Sets the current selection.
   */
  void setSelection( const DOM::Range & );

  /**
   * Has the user selected anything?
   *
   *  Call selectedText() to
   * retrieve the selected text.
   *
   * @return @p true if there is text selected.
   */
  bool hasSelection() const;

  /**
   * Returns the instance of the attached html editor interface.
   *
   */
  DOM::Editor *editor() const;

  /**
   * Marks all text in the document as selected.
   */
  void selectAll();

  /**
   * Convenience method to show the document's view.
   *
   * Equivalent to widget()->show() or view()->show() .
   */
  void show();

  /**
   * Convenience method to hide the document's view.
   *
   * Equivalent to widget()->hide() or view()->hide().
   */
  void hide();

  /**
   * Returns a reference to the partmanager instance which
   * manages html frame objects.
   */
  KParts::PartManager *partManager();

  /**
   * Saves the KHTMLPart's complete state (including child frame
   * objects) to the provided QDataStream.
   *
   * This is called from the saveState() method of the
   * browserExtension().
   */
  virtual void saveState( QDataStream &stream );
  /**
   * Restores the KHTMLPart's previously saved state (including
   * child frame objects) from the provided QDataStream.
   *
   * @see saveState()
   *
   * This is called from the restoreState() method of the
   * browserExtension() .
   **/
  virtual void restoreState( QDataStream &stream );

  /**
   * Returns the @p Node currently under the mouse.
   *
   * The returned node may be a shared node (e. g. an \<area> node if the
   * mouse is hovering over an image map).
   */
  DOM::Node nodeUnderMouse() const;

  /**
   * Returns the @p Node currently under the mouse that is not shared.
   *
   * The returned node is always the node that is physically under the mouse
   * pointer (irrespective of logically overlying elements like, e. g.,
   * \<area> on image maps).
   */
  DOM::Node nonSharedNodeUnderMouse() const;

  /**
   * @internal
   */
  const KHTMLSettings *settings() const;

  /**
   * Returns a pointer to the parent KHTMLPart if the part is a frame
   * in an HTML frameset.
   *
   *  Returns 0L otherwise.
   */
   // ### KDE5 make const
  KHTMLPart *parentPart();

  /**
   * Returns a list of names of all frame (including iframe) objects of
   * the current document. Note that this method is not working recursively
   * for sub-frames.
   */
  QStringList frameNames() const;

  QList<KParts::ReadOnlyPart*> frames() const;

  /**
   * Finds a frame by name. Returns 0L if frame can't be found.
   */
  KHTMLPart *findFrame( const QString &f );

  /**
   * Recursively finds the part containing the frame with name @p f
   * and checks if it is accessible by @p callingPart
   * Returns 0L if no suitable frame can't be found.
   * Returns parent part if a suitable frame was found and
   * frame info in @p *childFrame
   */
  KHTMLPart *findFrameParent( KParts::ReadOnlyPart *callingPart, const QString &f, khtml::ChildFrame **childFrame=0 );

  /**
   * Return the current frame (the one that has focus)
   * Not necessarily a direct child of ours, framesets can be nested.
   * Returns "this" if this part isn't a frameset.
   */
  KParts::ReadOnlyPart *currentFrame() const;

  /**
   * Returns whether a frame with the specified name is exists or not.
   * In contrast to the findFrame method this one also returns @p true
   * if the frame is defined but no displaying component has been
   * found/loaded, yet.
   */
  bool frameExists( const QString &frameName );

  /**
   * Returns child frame framePart its script interpreter
   */
  KJSProxy *framejScript(KParts::ReadOnlyPart *framePart);

  /**
   * Finds a frame by name. Returns 0L if frame can't be found.
   */
  KParts::ReadOnlyPart *findFramePart( const QString &f );
  /**
   * Called by KJS.
   * Sets the StatusBarText assigned
   * via window.status
   */
  void setJSStatusBarText( const QString &text );

  /**
   * Called by KJS.
   * Sets the DefaultStatusBarText assigned
   * via window.defaultStatus
   */
  void setJSDefaultStatusBarText( const QString &text );

  /**
   * Called by KJS.
   * Returns the StatusBarText assigned
   * via window.status
   */
  QString jsStatusBarText() const;

  /**
   * Called by KJS.
   * Returns the DefaultStatusBarText assigned
   * via window.defaultStatus
   */
  QString jsDefaultStatusBarText() const;

  /**
   * Referrer used for links in this page.
   */
  QString referrer() const;

  /**
   * Referrer used to obtain this page.
   */
  QString pageReferrer() const;

  /**
   * Last-modified date (in raw string format), if received in the [HTTP] headers.
   */
  QString lastModified() const;

  /**
   * Loads a style sheet into the stylesheet cache.
   */
  void preloadStyleSheet( const QString &url, const QString &stylesheet );

  /**
   * Loads a script into the script cache.
   */
  void preloadScript( const QString &url, const QString &script );

  /**
   * Returns whether the given point is inside the current selection.
   *
   * The coordinates are content-coordinates.
   */
   bool isPointInsideSelection(int x, int y);

  /**
   * @internal
   */
  bool restored() const;

  /**
   * Sets whether the document's Doctype should always be used
   * to determine the parsing mode for the document.
   *
   * Without this, parsing will be forced to
   * strict mode when using the write( const QString &str )
   * method for backward compatibility reasons.
   *
   */
   // ### KDE5 remove - fix write( const QString &str ) instead
  void setAlwaysHonourDoctype( bool b = true );

  // ### KDE5 remove me
  enum FormNotification { NoNotification = 0, Before, Only, Unused=255 };
  /**
   * Determine if signal should be emitted before, instead or never when a
   * submitForm() happens.
   * ### KDE5 remove me
   */
  void setFormNotification(FormNotification fn);

  /**
   * Determine if signal should be emitted before, instead or never when a
   * submitForm() happens.
   * ### KDE5 remove me
   */
  FormNotification formNotification() const;

  /**
   * Returns the toplevel (origin) URL of this document, even if this
   * part is a frame or an iframe.
   *
   * @return the actual original url.
   */
  QUrl toplevelURL();

  /**
   * Checks whether the page contains unsubmitted form changes.
   *
   * @return @p true if form changes exist
   */
  bool isModified() const;

  /**
   * Shows or hides the suppressed popup indicator
   */
  void setSuppressedPopupIndicator( bool enable, KHTMLPart *originPart = 0 );

  /**
   * @internal
   */
  bool inProgress() const;

Q_SIGNALS:
  /**
   * Emitted if the cursor is moved over an URL.
   */
  void onURL( const QString &url );

  /**
   * Emitted when the user clicks the right mouse button on the document.
   * See KParts::BrowserExtension for two more popupMenu signals emitted by khtml,
   * with much more information in the signal.
   */
  void popupMenu( const QString &url, const QPoint &point );

  /**
   * This signal is emitted when the selection changes.
   */
  void selectionChanged();

  /**
   * This signal is emitted when an element retrieves the
   * keyboard focus. Note that the signal argument can be
   * a null node if no element is active, meaning a node
   * has explicitly been deactivated without a new one
   * becoming active.
   */
  void nodeActivated( const DOM::Node & );

  /**
   * @internal */
  void docCreated();

  /**
   * This signal is emitted whenever the caret position has been changed.
   *
   * The signal transmits the position the DOM::Range way, the node and
   * the zero-based offset within this node.
   * @param node node which the caret is in. This can be null if the caret
   *	has been deactivated.
   * @param offset offset within the node. If the node is null, the offset
   *	is meaningless.
   */
  void caretPositionChanged(const DOM::Node &node, long offset);


  /**
   * If form notification is on, this will be emitted either for a form
   * submit or before the form submit according to the setting.
   * ### KDE4 remove me
   */
  void formSubmitNotification(const char *action, const QString& url,
                  const QByteArray& formData, const QString& target,
                  const QString& contentType, const QString& boundary);

  /**
   * Emitted whenever the configuration has changed
   */
  void configurationChanged();


protected:

  /**
   * returns a QUrl object for the given url. Use when
   * you know what you're doing.
   */
  QUrl completeURL( const QString &url );

  /**
   * presents a detailed error message to the user.
   * @p errorCode kio error code, eg KIO::ERR_SERVER_TIMEOUT.
   * @p text kio additional information text.
   * @p url the url that triggered the error.
   */
  void htmlError( int errorCode, const QString& text, const QUrl& reqUrl );

  virtual void customEvent( QEvent *event );

  /**
   * Eventhandler of the khtml::MousePressEvent.
   */
  virtual void khtmlMousePressEvent( khtml::MousePressEvent *event );
  /**
   * Eventhandler for the khtml::MouseDoubleClickEvent.
   */
  virtual void khtmlMouseDoubleClickEvent( khtml::MouseDoubleClickEvent * );
  /**
   * Eventhandler for the khtml::MouseMouseMoveEvent.
   */
  virtual void khtmlMouseMoveEvent( khtml::MouseMoveEvent *event );
  /**
   * Eventhandler for the khtml::MouseMouseReleaseEvent.
   */
  virtual void khtmlMouseReleaseEvent( khtml::MouseReleaseEvent *event );
  /**
   * Eventhandler for the khtml::DrawContentsEvent.
   */
  virtual void khtmlDrawContentsEvent( khtml::DrawContentsEvent * );

  /**
   * Internal reimplementation of KParts::Part::guiActivateEvent .
   */
  virtual void guiActivateEvent( KParts::GUIActivateEvent *event );

  /**
   * Internal empty reimplementation of KParts::ReadOnlyPart::openFile .
   */
  virtual bool openFile();

  virtual bool urlSelected( const QString &url, int button, int state,
                            const QString &_target,
                            const KParts::OpenUrlArguments& args = KParts::OpenUrlArguments(),
                            const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );

  /**
   * This method is called when a new embedded object (include html frames) is to be created.
   * Reimplement it if you want to add support for certain embeddable objects without registering
   * them in the KDE wide registry system (KSyCoCa) . Another reason for re-implementing this
   * method could be if you want to derive from KTHMLPart and also want all html frame objects
   * to be a object of your derived type, in which case you should return a new instance for
   * the mimetype 'text/html' .
   */
  virtual KParts::ReadOnlyPart *createPart( QWidget *parentWidget,
                                            QObject *parent,
                                            const QString &mimetype, QString &serviceName,
                                            QStringList &serviceTypes, const QStringList &params);

  // This is for RenderPartObject. We want to ask the 'download plugin?'
  // question only once per mimetype
  bool pluginPageQuestionAsked( const QString& mimetype ) const;
  void setPluginPageQuestionAsked( const QString& mimetype );

  enum PageSecurity { NotCrypted, Encrypted, Mixed };
  void setPageSecurity( PageSecurity sec );

  /**
   * Implements the streaming API of KParts::ReadOnlyPart.
   */
  virtual bool doOpenStream( const QString& mimeType );

  /**
   * Implements the streaming API of KParts::ReadOnlyPart.
   */
  virtual bool doWriteStream( const QByteArray& data );

  /**
   * Implements the streaming API of KParts::ReadOnlyPart.
   */
  virtual bool doCloseStream();

  /**
   * @internal
   */
  virtual void timerEvent(QTimerEvent *);

  /**
   * Will pre-resolve @p name according to dnsPrefetch current settings
   * Returns @p true if the name will be pre-resolved.
   * Otherwise returns false.
   */

  bool mayPrefetchHostname( const QString& name );

  /**
   * @internal
   */
  void updateZoomFactor();

public Q_SLOTS:

  /**
   * Sets the focused node of the document to the specified node. If the node is a form control, the control will
   * receive focus in the same way that it would if the user had clicked on it or tabbed to it with the keyboard. For
   * most other types of elements, there is no visual indication of whether or not they are focused.
   *
   * See activeNode
   *
   * @param node The node to focus
   */
  void setActiveNode( const DOM::Node &node );

  /**
   * Stops all animated images on the current and child pages
   */
  void stopAnimations();

  /**
   * Execute the specified snippet of JavaScript code.
   *
   * Returns @p true if JavaScript was enabled, no error occurred
   * and the code returned @p true itself or @p false otherwise.
   * @deprecated, use executeString( DOM::Node(), script)
   */
  QVariant executeScript( const QString &script );

  /**
   * Enables/disables caret mode.
   *
   * Enabling caret mode displays a caret which can be used to navigate
   * the document using the keyboard only. Caret mode is switched off by
   * default.
   *
   * @param enable @p true to enable, @p false to disable caret mode.
   */
  void setCaretMode(bool enable);

  /**
   * Makes the document editable.
   *
   * Setting this property to @p true makes the document, and its
   * subdocuments (such as frames, iframes, objects) editable as a whole.
   * FIXME: insert more information about navigation, features etc. as seen fit
   *
   * @param enable @p true to set document editable, @p false to set it
   *	read-only.
   */
  void setEditable(bool enable);

  /**
   * Sets the visibility of the caret.
   *
   * This methods displays or hides the caret regardless of the current
   * caret display policy (see setCaretDisplayNonFocused), and regardless
   * of focus.
   *
   * The caret will be shown/hidden only under at least one of
   * the following conditions:
   * @li the document is editable
   * @li the document is in caret mode
   * @li the document's currently focused element is editable
   *
   * @param show @p true to make visible, @p false to hide.
   */
  void setCaretVisible(bool show);

  // ### KDE4 FIXME:
  //          Remove this and make the one below protected+virtual slot.
  //          Warning: this is effectively "internal".  Be careful.
  void submitFormProxy( const char *action, const QString &url,
                        const QByteArray &formData,
                        const QString &target,
                        const QString& contentType = QString(),
                        const QString& boundary = QString() );

protected Q_SLOTS:

  /**
   * Called when the job downloading the page is finished.
   * Can be reimplemented, for instance to get metadata out of the job,
   * but make sure to call KHTMLPart::slotFinished() too.
   */
  virtual void slotFinished( KJob* );

protected:
  /**
   * Hook for adding code before a job is started.
   * This can be used to add metadata, like job->addMetaData("PropagateHttpHeader", "true")
   * to get the HTTP headers.
   */
  virtual void startingJob( KIO::Job * ) {}

private Q_SLOTS:

  /**
   * @internal
   */
  void reparseConfiguration();

  /**
   * @internal
   */
  void slotData( KIO::Job*, const QByteArray &data );
  /**
  * @internal
  */
  void slotInfoMessage( KJob*, const QString& msg );
  /**
   * @internal
   */
  void slotRestoreData( const QByteArray &data );
  /**
   * @internal
   */
  void slotFinishedParsing();
  /**
   * @internal
   */
  void slotRedirect();
  /**
   * @internal
   */
  void slotRedirection( KIO::Job*, const QUrl& );
  /**
   * @internal
   */
  void slotDebugScript();
  /**
   * @internal
   */
  void slotDebugDOMTree();
  /**
   * @internal
   */
  void slotDebugRenderTree();

  void slotDebugFrameTree();

  /**
   * @internal
   */
  void slotStopAnimations();
  /**
   * @internal
   */
  virtual void slotViewDocumentSource();
  /**
   * @internal
   */
  virtual void slotViewFrameSource();
  /**
   * @internal
   */
  void slotViewPageInfo();
  /**
   * @internal
   */
  virtual void slotSaveBackground();
  /**
   * @internal
   */
  virtual void slotSaveDocument();
  /**
   * @internal
   */
  virtual void slotSaveFrame();
  /**
   * @internal
   */
  virtual void slotSecurity();
  /**
   * @internal
   */
  virtual void slotSetEncoding(const QString &);

  /**
   * @internal
   */
  virtual void slotUseStylesheet();

  virtual void slotFind();
  virtual void slotFindDone(); // ### remove me
  virtual void slotFindDialogDestroyed(); // ### remove me
  void slotFindNext();
  void slotFindPrev();
  void slotFindAheadText();
  void slotFindAheadLink();

  void slotIncZoom();
  void slotDecZoom();
  void slotIncZoomFast();
  void slotDecZoomFast();

  void slotIncFontSize();
  void slotDecFontSize();
  void slotIncFontSizeFast();
  void slotDecFontSizeFast();

  void slotLoadImages();
  void slotWalletClosed();
  void launchWalletManager();
  void walletMenu();
  void delNonPasswordStorableSite();
  void removeStoredPasswordForm(QAction* action);
  void addWalletFormKey(const QString& walletFormKey);

  /**
   * @internal
   */
  void submitFormAgain();

  /**
   * @internal
   */
  void updateActions();
  /**
   * @internal
   */
  void slotPartRemoved( KParts::Part *part );
  /**
   * @internal
   */
  void slotActiveFrameChanged( KParts::Part *part );
  /**
   * @internal
   */
  void slotChildStarted( KIO::Job *job );
  /**
   * @internal
   */
  void slotChildCompleted();
  /**
   * @internal
   */
  void slotChildCompleted( bool );
  /**
   * @internal
   */
  void slotParentCompleted();
  /**
   * @internal
   */
  void slotChildURLRequest( const QUrl &url, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &args );
  /**
   * @internal
   */
  void slotChildDocCreated();
  /**
   * @internal
   */
  void slotRequestFocus( KParts::ReadOnlyPart * );
  void slotLoaderRequestStarted( khtml::DocLoader*, khtml::CachedObject* obj);
  void slotLoaderRequestDone( khtml::DocLoader*, khtml::CachedObject *obj );
  void checkCompleted();

  /**
   * @internal
   */
  void slotAutoScroll();

  void slotPrintFrame();

  void slotSelectAll();

  /**
   * @internal
   */
  void slotProgressUpdate();

  /*
   * @internal
   */
  void slotJobPercent( KJob*, unsigned long );

  /*
   * @internal
   */
  void slotJobDone( KJob* );

  /*
   * @internal
   */
  void slotUserSheetStatDone( KJob* );

  /*
   * @internal
   */
  void slotJobSpeed( KJob*, unsigned long );

  /**
   * @internal
   */
  void slotClearSelection();

  /**
   * @internal
   */
  void slotZoomView( int );

  /**
   * @internal
   */
  void slotAutomaticDetectionLanguage(KEncodingProber::ProberType scri);

  /**
   * @internal
   */
  void slotToggleCaretMode();

  /**
   * @internal
   */
  void suppressedPopupMenu();

  /**
   * @internal
   */
  void togglePopupPassivePopup();

  /**
   * @internal
   */
  void showSuppressedPopups();

  /**
   * @internal
   */
  void launchJSConfigDialog();

  /**
   * @internal
   */
  void launchJSErrorDialog();

  /**
   * @internal
   */
  void removeJSErrorExtension();

  /**
   * @internal
   */
  void disableJSErrorExtension();

  /**
   * @internal
   */
  void jsErrorDialogContextMenu();

  /**
   * @internal
   * used to restore or reset the view's scroll position (including positioning on anchors)
   * once a sufficient portion of the document as been laid out.
   */
  void restoreScrollPosition();

  void walletOpened(KWallet::Wallet*);

private:

  KJSErrorDlg *jsErrorExtension();

  enum StatusBarPriority { BarDefaultText, BarHoverText, BarOverrideText };
  void setStatusBarText( const QString& text, StatusBarPriority p);

  bool restoreURL( const QUrl &url );
  void clearCaretRectIfNeeded();
  void setFocusNodeIfNeeded(const DOM::Selection &);
  void selectionLayoutChanged();
  void notifySelectionChanged(bool closeTyping=true);
  void resetFromScript();
  void emitSelectionChanged();
  void onFirstData();
  // Returns whether callingHtmlPart may access this part
  bool checkFrameAccess(KHTMLPart *callingHtmlPart);
  bool openUrlInFrame(const QUrl &url, const KParts::OpenUrlArguments& arguments, const KParts::BrowserArguments &browserArguments);
  void startAutoScroll();
  void stopAutoScroll();
  void overURL( const QString &url, const QString &target, bool shiftPressed = false );
  void resetHoverText(); // Undo overURL and reset HoverText

  KParts::ScriptableExtension *scriptableExtension( const DOM::NodeImpl *);

  KWallet::Wallet* wallet();

  void openWallet(DOM::HTMLFormElementImpl*);
  void saveToWallet(const QString& key, const QMap<QString,QString>& data);
  void dequeueWallet(DOM::HTMLFormElementImpl*);
  void saveLoginInformation(const QString& host, const QString& key, const QMap<QString, QString>& walletMap);

  void enableFindAheadActions(bool);

  /**
   * Returns a pointer to the top view bar.
   */
  KHTMLViewBar *pTopViewBar() const;

  /**
   * Returns a pointer to the bottom view bar.
   */
  KHTMLViewBar *pBottomViewBar() const;

  /**
   * @internal
   */
  bool pFindTextNextInThisFrame( bool reverse );

  /**
   * @internal
   */
  // ### KDE4 FIXME:
  //          It is desirable to be able to filter form submissions as well.
  //          For instance, forms can have a target and an inheriting class
  //          might want to filter based on the target.  Make this protected
  //          and virtual, or provide a better solution.
  //          See the web_module for the sidebar for an example where this is
  //          necessary.
  void submitForm( const char *action, const QString &url, const QByteArray &formData,
                   const QString &target, const QString& contentType = QString(),
                   const QString& boundary = QString() );

  void popupMenu( const QString &url );

  void init( KHTMLView *view, GUIProfile prof );


  void clear();

  QVariant crossFrameExecuteScript(const QString& target, const QString& script);

  /**
   * @internal returns a name for a frame without a name.
   * This function returns a sequence of names.
   * All names in a sequence are different but the sequence is
   * always the same.
   * The sequence is reset in clear().
   */
  QString requestFrameName();

  // Requests loading of a frame or iframe element
  void loadFrameElement( DOM::HTMLPartContainerElementImpl *frame, const QString &url, const QString &frameName,
                         const QStringList &args = QStringList(), bool isIFrame = false );

  // Requests loading of an object or embed element. Returns true if
  // loading succeeded.
  bool loadObjectElement( DOM::HTMLPartContainerElementImpl *frame, const QString &url, const QString &serviceType,
                          const QStringList &args = QStringList() );

  // Tries an open a URL in given ChildFrame with all known navigation information
  // like mimetype and the like in the KParts arguments.
  //
  // Returns true if it's done -- which excludes the case when it's still resolving
  // the mimetype.
  // ### refine comment wrt to error case
  bool requestObject( khtml::ChildFrame *child, const QUrl &url,
                      const KParts::OpenUrlArguments &args = KParts::OpenUrlArguments(),
                      const KParts::BrowserArguments& browserArgs = KParts::BrowserArguments() );

  // This method does the loading inside a ChildFrame once we know what mimetype to
  // load it as
  bool processObjectRequest( khtml::ChildFrame *child, const QUrl &url, const QString &mimetype );

  // helper for reporting ChildFrame load failure
  void childLoadFailure( khtml::ChildFrame *child );

  // Updates the ChildFrame to use the particular part, hooking up the various
  // signals, connections, etc.
  void connectToChildPart( khtml::ChildFrame *child, KParts::ReadOnlyPart *part,
                           const QString &mimetype );

  // Low-level navigation of the part itself --- this doesn't ask the user
  // to save things or such, and assumes that all the ChildFrame info is already
  // filed in with things like the mimetype and so on
  //
  // Returns if successful or not
  bool navigateChild( khtml::ChildFrame *child, const QUrl& url );

  // Helper for executing javascript: or about: protocols
  bool navigateLocalProtocol( khtml::ChildFrame *child, KParts::ReadOnlyPart *part,
                              const QUrl& url );


  DOM::EventListener *createHTMLEventListener( QString code, QString name, DOM::NodeImpl *node, bool svg = false );

  DOM::HTMLDocumentImpl *docImpl() const;
  DOM::DocumentImpl *xmlDocImpl() const;
  khtml::ChildFrame *frame( const QObject *obj );

  khtml::ChildFrame *recursiveFrameRequest( KHTMLPart *callingHtmlPart, const QUrl &url,
                                            const KParts::OpenUrlArguments& args, const KParts::BrowserArguments &browserArgs,
                                            bool callParent = true );

  bool checkLinkSecurity( const QUrl &linkURL,const KLocalizedString &message = KLocalizedString(), const QString &button = QString() );
  QVariant executeScript( const QString& filename, int baseLine, const DOM::Node &n, const QString& script );

  KJSProxy *jScript();

  KHTMLPart *opener();
  long cacheId() const;
  void setOpener( KHTMLPart *_opener );
  bool openedByJS();
  void setOpenedByJS( bool _openedByJS );

  void checkEmitLoadEvent();
  void emitLoadEvent();

  bool initFindNode( bool selection, bool reverse, bool fromCursor );

  /** extends the current selection to the given content-coordinates @p x, @p y
   * @param x content x-coordinate
   * @param y content y-coordinate
   * @param absX absolute x-coordinate of @p innerNode
   * @param absY absolute y-coordinate of @p innerNode
   * @param innerNode node from which to start extending the selection. The
   *	caller has to ensure that the node has a renderer.
   * @internal
   */
  void extendSelectionTo(int x, int y, const DOM::Node &innerNode);
  /** checks whether a selection is extended.
   * @return @p true if a selection is extended by the mouse.
   */
  bool isExtendingSelection() const;
  KEncodingDetector *createDecoder();
  QString defaultEncoding() const;

  /** .html, .xhtml or .xml */
  QString defaultExtension() const;

  /** @internal
   * generic zoom in
   */
  void zoomIn(const int stepping[], int count);
  /** @internal
   * generic zoom out
   */
  void zoomOut(const int stepping[], int count);

  void incFontSize(const int stepping[], int count);

  void decFontSize(const int stepping[], int count);

  void emitCaretPositionChanged(const DOM::Position &pos);

  void setDebugScript( bool enable );

  void runAdFilter();

  khtml::EditorContext *editorContext() const;

  /**
   * initialises the caret if it hasn't been used yet.
   * @internal
   */
  void initCaret();

  /**
   * Returns the selected part of the HTML.
   */
  const DOM::Selection &caret() const;

  /**
   * Returns the drag caret of the HTML.
   */
  const DOM::Selection &dragCaret() const;

  /**
   * Sets the current caret to the given selection.
   */
  void setCaret(const DOM::Selection &, bool closeTyping=true);

  /**
   * Sets the current drag caret.
   */
  void setDragCaret(const DOM::Selection &);

  /**
   * Clears the current selection.
   */
  void clearSelection();

  /**
   * Invalidates the current selection.
   */
  void invalidateSelection();

  /**
   * Controls the visibility of the selection.
   */
  void setSelectionVisible(bool flag=true);

  /**
   * Paints the caret.
   */
  void paintCaret(QPainter *p, const QRect &rect) const;

  /**
   * Paints the drag caret.
   */
  void paintDragCaret(QPainter *p, const QRect &rect) const;

  /**
   * Returns selectedText without any leading or trailing whitespace,
   * and with non-breaking-spaces turned into normal spaces.
   *
   * Note that hasSelection can return true and yet simplifiedSelectedText can be empty,
   * e.g. when selecting a single space.
   */
  QString simplifiedSelectedText() const;

  bool handleMouseMoveEventDrag(khtml::MouseMoveEvent *event);
  bool handleMouseMoveEventOver(khtml::MouseMoveEvent *event);
  void handleMouseMoveEventSelection(khtml::MouseMoveEvent *event);

  void handleMousePressEventSingleClick(khtml::MousePressEvent *event);
  void handleMousePressEventDoubleClick(khtml::MouseDoubleClickEvent *event);
  void handleMousePressEventTripleClick(khtml::MouseDoubleClickEvent *event);

  KHTMLPartPrivate *d;
  friend class KHTMLPartPrivate;

public: // So we don't end up having to add 50 more friends

  /** @internal Access to internal APIs. Don't use outside */
  KHTMLPartPrivate* impl() { return d; }
};


#endif
