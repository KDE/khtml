/*
 * This file is part of the HTML widget for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
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
#ifndef render_canvas_h
#define render_canvas_h

#include "render_block.h"

class KHTMLView;

namespace khtml {

class RenderPage;
class RenderStyle;

enum CanvasMode {
    CanvasViewPort, // Paints inside a viewport
    CanvasPage,     // Paints one page
    CanvasDocument  // Paints the whole document
};

class RenderCanvas : public RenderBlock
{
public:
    RenderCanvas(DOM::NodeImpl* node, KHTMLView *view);
    ~RenderCanvas();

    virtual const char *renderName() const { return "RenderCanvas"; }

    virtual bool isCanvas() const { return true; }

    virtual void setStyle(RenderStyle *style);
    virtual void layout();
    virtual void calcWidth();
    virtual void calcHeight();
    virtual void calcMinMaxWidth();
    virtual bool absolutePosition(int &xPos, int&yPos, bool f = false) const;

    int docHeight() const;
    int docWidth() const;

    KHTMLView *view() const { return m_view; }

    virtual void repaint(Priority p=NormalPriority);
    virtual void repaintRectangle(int x, int y, int w, int h, Priority p=NormalPriority, bool f=false);
    void repaintViewRectangle(int x, int y, int w, int h, bool asap=false);
    bool needsFullRepaint() const;
    void deferredRepaint( RenderObject* o );
    void scheduleDeferredRepaints();

    virtual void paint(PaintInfo&, int tx, int ty);
    virtual void paintBoxDecorations(PaintInfo& paintInfo, int _tx, int _ty);
    virtual void setSelection(RenderObject *s, int sp, RenderObject *e, int ep);
    virtual void clearSelection(bool doRepaint=true);
    virtual RenderObject *selectionStart() const { return m_selectionStart; }
    virtual RenderObject *selectionEnd() const { return m_selectionEnd; }
    bool hasSelection() const { return m_selectionStart && m_selectionEnd; }

    void setPrintImages(bool enable) { m_printImages = enable; }
    bool printImages() const { return m_printImages; }

    void setCanvasMode(CanvasMode mode) { m_canvasMode = mode; }
    CanvasMode canvasMode() const { return m_canvasMode; }

    void setPagedMode(bool b) { m_pagedMode = b; }
    void setStaticMode(bool b) { m_staticMode = b; }

    bool pagedMode() const { return m_pagedMode; }
    bool staticMode() const { return m_staticMode; }

    void setPageTop(int top) {
        m_pageTop = top;
//         m_y = top;
    }
    void setPageBottom(int bottom) { m_pageBottom = bottom; }
    int pageTop() const { return m_pageTop; }
    int pageBottom() const { return m_pageBottom; }

    int pageTopAfter(int y) const {
        if (pageHeight() == 0) return 0;
        return (y / pageHeight() + 1) * pageHeight() ;
    }

    int crossesPageBreak(int top, int bottom) const {
        if (pageHeight() == 0) return false;
        int pT = top / pageHeight();
        // bottom is actually the first line not in the box
        int pB = (bottom-1) / pageHeight();
        return (pT == pB) ? 0 : (pB + 1);
    }

    void setPageNumber(int number) { m_pageNr = number; }
    int pageNumber() const { return m_pageNr; }

    void updateInvalidatedFonts();
public:
    virtual void setWidth( int width ) { m_rootWidth = m_width = width; }
    virtual void setHeight( int height ) { m_rootHeight = m_height = height; }

//     void setPageHeight( int height ) { m_viewportHeight = m_pageHeight = height; }
    int pageHeight() const { return m_pageBottom - m_pageTop; }

    int viewportWidth() const { return m_viewportWidth; }
    int viewportHeight() const { return m_viewportHeight; }

    RenderPage* page();

    QRect selectionRect() const;
    QRegion staticRegion() const;

    void setMaximalOutlineSize(int o) { m_maximalOutlineSize = o; }
    int maximalOutlineSize() const { return m_maximalOutlineSize; }
    
    void setNeedsWidgetMasks( bool b=true);
    bool needsWidgetMasks() const { return m_needsWidgetMasks; }
    
    void addStaticObject( RenderObject*o, bool positioned=false );
    void removeStaticObject( RenderObject*o, bool positioned=false );

    void updateDocSizeAfterLayerTranslation( RenderObject* o, bool posXOffset, bool posYOffset );

    bool isPerformingLayout() const { return m_isPerformingLayout; }
protected:
    // makes sure document, scrollbars and viewport size are accurate
    void updateDocumentSize();

    // internal setters for cached values of document width/height
    // Setting to -1/-1 invalidates the cache.
    void setCachedDocWidth(int w ) { m_cachedDocWidth = w; }
    void setCachedDocHeight(int h) { m_cachedDocHeight = h; }

    virtual void selectionStartEnd(int& spos, int& epos);

    virtual QRect viewRect() const;

    KHTMLView *m_view;

    RenderObject* m_selectionStart;
    RenderObject* m_selectionEnd;
    int m_selectionStartPos;
    int m_selectionEndPos;

    CanvasMode m_canvasMode;

    int m_rootWidth;
    int m_rootHeight;

    int m_viewportWidth;
    int m_viewportHeight;
    
    int m_cachedDocWidth;
    int m_cachedDocHeight;

    bool m_printImages;
    bool m_needsFullRepaint;

    // Canvas is not interactive
    bool m_staticMode;
    // Canvas is paged
    bool m_pagedMode;
    // Canvas contains overlaid widgets
    bool m_needsWidgetMasks;
    // Whether we are currently performing a layout
    bool m_isPerformingLayout;

    short m_pageNr;

    int m_pageTop;
    int m_pageBottom;

    RenderPage* m_page;

    int m_maximalOutlineSize; // Used to apply a fudge factor to dirty-rect checks on blocks/tables.
    QList<RenderObject*> m_dirtyChildren;
    QSet<RenderObject*>m_fixedBackground;
    QSet<RenderObject*>m_fixedPosition;
};

inline RenderCanvas* RenderObject::canvas() const
{
    return static_cast<RenderCanvas*>(document()->renderer());
}

// Represents the page-context of CSS
class RenderPage
{
public:
    RenderPage(RenderCanvas* canvas) : m_canvas(canvas),
        m_marginTop(0), m_marginBottom(0),
        m_marginLeft(0), m_marginRight(0),
        m_pageWidth(0), m_pageHeight(0),
        m_fixedSize(false)
    {
        m_style = new RenderPageStyle();
    }
    virtual ~RenderPage()
    {
        delete m_style;
    }

    int marginTop() const       { return m_marginTop; }
    int marginBottom() const    { return m_marginBottom; }
    int marginLeft() const      { return m_marginLeft; }
    int marginRight() const     { return m_marginRight; }

    void setMarginTop(int margin)       { m_marginTop = margin; }
    void setMarginBottom(int margin)    { m_marginBottom = margin; }
    void setMarginLeft(int margin)      { m_marginLeft = margin; }
    void setMarginRight(int margin)     { m_marginRight = margin; }

    int pageWidth() const   { return m_pageWidth; }
    int pageHeight() const  { return m_pageHeight; }

    void setPageSize(int width, int height) {
        m_pageWidth = width;
        m_pageHeight = height;
    }

    // Returns true if size was set by document, false if set by user-agent
    bool fixedSize() const { return m_fixedSize; }
    void setFixedSize(bool b) { m_fixedSize = b; }

    RenderPageStyle* style() { return m_style; }
    const RenderPageStyle* style() const { return m_style; }

protected:
    RenderCanvas* m_canvas;
    RenderPageStyle* m_style;

    int m_marginTop;
    int m_marginBottom;
    int m_marginLeft;
    int m_marginRight;

    int m_pageWidth;
    int m_pageHeight;

    bool m_fixedSize;
};

}
#endif
