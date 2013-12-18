set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  ${KDE_ENABLE_EXCEPTIONS}")


find_package(OpenSSL)
set_package_properties(OpenSSL PROPERTIES DESCRIPTION "Support for secure network communications (SSL and TLS)"
                       URL "http://openssl.org"
                       TYPE RECOMMENDED
                       PURPOSE "KDE uses OpenSSL for the bulk of secure communications, including secure web browsing via HTTPS"
                      )
if(OPENSSL_FOUND)
   set(KSSL_HAVE_SSL 1)
   include_directories(${OPENSSL_INCLUDE_DIR})
endif()

configure_file(config-khtml.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-khtml.h )

set(khtmlgraphics_STAT_SRCS
  platform/graphics/AffineTransform.cpp
  platform/graphics/FloatPoint.cpp
  platform/graphics/FloatRect.cpp
  platform/graphics/FloatSize.cpp
  platform/graphics/IntRect.cpp
  platform/graphics/Path.cpp
  platform/graphics/GraphicsTypes.cpp
  platform/graphics/PathTraversalState.cpp
  #platform/graphics/Image.cpp
  #platform/MIMETypeRegistry.cpp
  platform/graphics/qt/AffineTransformQt.cpp
  platform/graphics/qt/FloatPointQt.cpp
  platform/graphics/qt/FloatRectQt.cpp
  platform/graphics/qt/IntPointQt.cpp
  platform/graphics/qt/IntRectQt.cpp
  platform/graphics/qt/IntSizeQt.cpp
  platform/graphics/qt/PathQt.cpp
  #platform/qt/MIMETypeRegistryQt.cpp
)

set(khtmlsvg_STAT_SRCS
  svg/SVGDocument.cpp
  svg/SVGSVGElement.cpp
  svg/SVGStyledLocatableElement.cpp
  svg/SVGLocatable.cpp
  svg/SVGStyledElement.cpp
  svg/SVGStylable.cpp
  svg/SVGElement.cpp
  svg/SVGLength.cpp
  svg/SVGTransformable.cpp
  svg/SVGTransform.cpp
  svg/SVGAngle.cpp
  svg/SVGDocumentExtensions.cpp
  svg/SVGParserUtilities.cpp
  svg/SVGTransformDistance.cpp
  svg/SVGTransformList.cpp
  svg/SVGStringList.cpp
  svg/SVGStyledTransformableElement.cpp
  svg/SVGTests.cpp
  svg/SVGLangSpace.cpp
  svg/SVGExternalResourcesRequired.cpp
  svg/SVGRectElement.cpp
  svg/SVGPreserveAspectRatio.cpp
  svg/SVGFitToViewBox.cpp
  svg/SVGCircleElement.cpp
  svg/SVGStyleElement.cpp
  svg/SVGEllipseElement.cpp
  svg/SVGPointList.cpp
  svg/SVGAnimatedPoints.cpp
  svg/SVGPolyElement.cpp
  svg/SVGPolygonElement.cpp
  svg/SVGPolylineElement.cpp
  svg/SVGAnimatedPathData.cpp
  svg/SVGPathSegArc.cpp
  svg/SVGPathSegClosePath.cpp
  svg/SVGPathSegCurvetoCubic.cpp
  svg/SVGPathSegCurvetoCubicSmooth.cpp
  svg/SVGPathSegCurvetoQuadratic.cpp
  svg/SVGPathSegCurvetoQuadraticSmooth.cpp
  svg/SVGPathSegLineto.cpp
  svg/SVGPathSegLinetoHorizontal.cpp
  svg/SVGPathSegLinetoVertical.cpp
  svg/SVGPathSegList.cpp
  svg/SVGPathSegMoveto.cpp
  svg/SVGPathElement.cpp
  svg/SVGURIReference.cpp
  svg/SVGStopElement.cpp
  svg/SVGGradientElement.cpp
  svg/SVGLinearGradientElement.cpp
  svg/SVGRadialGradientElement.cpp
  svg/SVGDefsElement.cpp
  svg/SVGClipPathElement.cpp
  svg/SVGGElement.cpp
  svg/SVGElementInstance.cpp
  svg/SVGElementInstanceList.cpp
  svg/SVGUseElement.cpp
  svg/SVGLineElement.cpp
  svg/SVGTextPathElement.cpp
  svg/SVGTextContentElement.cpp
  svg/SVGNumberList.cpp
  svg/SVGTextPositioningElement.cpp
  svg/SVGLengthList.cpp
  svg/SVGTextElement.cpp
  svg/SVGAElement.cpp
  svg/SVGScriptElement.cpp
  svg/SVGTitleElement.cpp
  svg/SVGDescElement.cpp
  svg/SVGNames.cpp
  svg/SVGZoomAndPan.cpp
  svg/SVGViewSpec.cpp
  svg/SVGTSpanElement.cpp
  svg/SVGGlyphElement.cpp
  svg/SVGAltGlyphElement.cpp
  svg/SVGHKernElement.cpp
  svg/SVGMissingGlyphElement.cpp
  svg/SVGFontElement.cpp
  svg/SVGFontData.cpp
#############################################################
## GRAPHICS #################################################
#############################################################
  svg/graphics/SVGResource.cpp
  svg/graphics/SVGPaintServer.cpp
  svg/graphics/qt/SVGPaintServerQt.cpp
  svg/graphics/SVGPaintServerSolid.cpp
  svg/graphics/qt/SVGPaintServerSolidQt.cpp
  svg/graphics/SVGResourceClipper.cpp
  svg/graphics/qt/SVGResourceClipperQt.cpp
  svg/graphics/SVGPaintServerGradient.cpp
  svg/graphics/SVGPaintServerLinearGradient.cpp
  svg/graphics/SVGPaintServerRadialGradient.cpp
  svg/graphics/qt/SVGPaintServerGradientQt.cpp
  svg/graphics/qt/SVGPaintServerLinearGradientQt.cpp
  svg/graphics/qt/SVGPaintServerRadialGradientQt.cpp
#############################################################
## RENDERING PART BELOW   ###################################
#############################################################
  rendering/RenderSVGRoot.cpp
  rendering/RenderPath.cpp
  rendering/SVGRenderStyleDefs.cpp
  rendering/SVGRenderStyle.cpp
  rendering/RenderSVGGradientStop.cpp
  rendering/RenderSVGContainer.cpp
  rendering/SVGRenderSupport.cpp
  rendering/RenderSVGHiddenContainer.cpp
  rendering/RenderSVGTransformableContainer.cpp
  rendering/SVGInlineFlowBox.cpp
  rendering/SVGRootInlineBox.cpp
  rendering/RenderSVGTextPath.cpp
  rendering/SVGInlineTextBox.cpp
  rendering/SVGCharacterLayoutInfo.cpp
  rendering/RenderSVGInline.cpp
  rendering/RenderSVGText.cpp
  rendering/RenderSVGBlock.cpp
  rendering/RenderSVGInlineText.cpp
  rendering/RenderSVGTSpan.cpp
)

# khtml/dom/Makefile.am: khtmldom

set(khtmldom_STAT_SRCS
  dom/dom_misc.cpp
  dom/html_block.cpp
  dom/html_inline.cpp
  dom/css_rule.cpp
  dom/dom_node.cpp
  dom/html_document.cpp
  dom/html_list.cpp
  dom/css_stylesheet.cpp
  dom/dom_string.cpp
  dom/html_element.cpp
  dom/html_misc.cpp
  dom/css_value.cpp
  dom/dom_text.cpp
  dom/html_object.cpp
  dom/dom_doc.cpp
  dom/dom_xml.cpp
  dom/html_head.cpp
  dom/dom_element.cpp
  dom/html_base.cpp
  dom/html_image.cpp
  dom/html_form.cpp
  dom/dom2_range.cpp
  dom/html_table.cpp
  dom/dom2_traversal.cpp
  dom/dom2_events.cpp
  dom/dom2_views.cpp
  dom/QualifiedName.cpp
  dom/dom3_xpath.cpp
)

#set_source_files_properties(${khtmldom_STAT_SRCS} PROPERTIES COMPILE_FLAGS ${KDE_ENABLE_EXCEPTIONS})

# khtml/misc/Makefile.am: khtmlmisc

set(khtmlmisc_STAT_SRCS
  misc/loader.cpp
  misc/helper.cpp
  misc/arena.cpp
  misc/stringit.cpp
  misc/paintbuffer.cpp
  misc/imagefilter.cpp
  misc/borderarcstroker.cpp
  misc/idstring.cpp
  misc/htmlnames.cpp
  misc/AtomicString.cpp
  misc/woff.cpp
  misc/guess_ja.cpp
  misc/kencodingdetector.cpp
)

set(khtmlediting_STAT_SRCS
  editing/jsediting.cpp
  editing/editing.cpp
  editing/editor.cpp
  editing/htmlediting_impl.cpp
)

set(khtmlfind_STAT_SRCS
  ui/findbar/khtmlfind.cpp
  ui/findbar/khtmlfindbar.cpp
)

qt5_wrap_ui(khtmlfind_STAT_SRCS
  ui/findbar/khtmlfindbar_base.ui
)

set(passwordbar_STAT_SRCS
  ui/passwordbar/storepassbar.cpp
)

qt5_wrap_ui(passwordbar_STAT_SRCS
  ui/passwordbar/storepassbar_base.ui
)

# khtml/html/Makefile.am: khtmlhtml

set(khtmlhtml_STAT_SRCS
  html/htmlparser.cpp
  html/htmltokenizer.cpp
  html/htmlprospectivetokenizer.cpp
  html/dtd.cpp
  html/html_headimpl.cpp
  html/html_blockimpl.cpp
  html/html_elementimpl.cpp
  html/html_inlineimpl.cpp
  html/html_documentimpl.cpp
  html/html_baseimpl.cpp
  html/html_imageimpl.cpp
  html/html_listimpl.cpp
  html/html_miscimpl.cpp
  html/html_formimpl.cpp
  html/html_objectimpl.cpp
  html/html_tableimpl.cpp
  html/html_canvasimpl.cpp
  html/HTMLMediaElement.cpp
  html/HTMLAudioElement.cpp
  html/HTMLVideoElement.cpp
  html/HTMLSourceElement.cpp
  html/TimeRanges.cpp
  html/ksslkeygen.cpp
  html/kopenssl.cpp
)

# khtml/ecma/Makefile.am: kjs_html

set(kjs_html_STAT_SRCS
  ecma/kjs_binding.cpp
  ecma/kjs_dom.cpp
  ecma/kjs_html.cpp
  ecma/kjs_window.cpp
  ecma/kjs_navigator.cpp
  ecma/kjs_proxy.cpp
  ecma/kjs_css.cpp
  ecma/kjs_range.cpp
  ecma/kjs_traversal.cpp
  ecma/kjs_events.cpp
  ecma/kjs_views.cpp
#  ecma/kjs_debugwin.cpp
  ecma/kjs_mozilla.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/JSTimeRanges.h
  ${CMAKE_CURRENT_BINARY_DIR}/JSTimeRanges.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/JSMediaError.h
  ${CMAKE_CURRENT_BINARY_DIR}/JSMediaError.cpp
  ecma/JSHTMLElement.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLMediaElement.h
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLMediaElement.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLAudioElement.h
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLAudioElement.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLVideoElement.h
  ${CMAKE_CURRENT_BINARY_DIR}/JSHTMLVideoElement.cpp
  ecma/xmlhttprequest.cpp
  ecma/xmlserializer.cpp
  ecma/domparser.cpp
  ecma/kjs_context2d.cpp
  ecma/kjs_audio.cpp
  ecma/kjs_xpath.cpp
  ecma/kjs_scriptable.cpp
  ecma/kjs_data.cpp
  ecma/kjs_arraybuffer.cpp
  ecma/kjs_arraybufferview.cpp
  ecma/kjs_arraytyped.cpp
  ecma/kjs_clientrect.cpp
)

# TODO: set(KJS_DEBUGGER) once we can depend on the ktexteditor interfaces
if (KJS_DEBUGGER)
  set (kjs_html_STAT_SRCS ${kjs_html_STAT_SRCS}
  ecma/debugger/debugwindow.cpp
  ecma/debugger/debugdocument.cpp
  ecma/debugger/consoledock.cpp
  ecma/debugger/breakpointsdock.cpp
  ecma/debugger/callstackdock.cpp
  ecma/debugger/localvariabledock.cpp
  ecma/debugger/scriptsdock.cpp
  ecma/debugger/value2string.cpp
  ecma/debugger/errordlg.cpp
  ecma/debugger/interpreter_ctx.cpp
  )
endif()

#qt5_add_resources( kjs_html_STAT_SRCS ecma/debugger/debugger.qrc )

# khtml/rendering/Makefile.am: khtmlrender

set(khtmlrender_STAT_SRCS
  rendering/bidi.cpp
  rendering/break_lines.cpp
  rendering/render_block.cpp
  rendering/render_inline.cpp
  rendering/render_style.cpp
  rendering/render_object.cpp
  rendering/render_container.cpp
  rendering/render_box.cpp
  rendering/render_flow.cpp
  rendering/render_text.cpp
  rendering/render_arena.cpp
  rendering/render_layer.cpp
  rendering/render_image.cpp
  rendering/render_table.cpp
  rendering/table_layout.cpp
  rendering/render_replaced.cpp
  rendering/render_form.cpp
  rendering/render_list.cpp
  rendering/render_canvas.cpp
  rendering/render_frames.cpp
  rendering/render_br.cpp
  rendering/render_body.cpp
  rendering/font.cpp
  rendering/render_line.cpp
  rendering/render_generated.cpp
  rendering/enumerate.cpp
  rendering/counter_tree.cpp
  rendering/render_canvasimage.cpp
  rendering/render_position.cpp
  rendering/render_media.cpp
  rendering/media_controls.cpp
)

# khtml/css/Makefile.am: khtmlcss

set(khtmlcss_STAT_SRCS
  css/css_stylesheetimpl.cpp
  css/css_ruleimpl.cpp
  css/css_valueimpl.cpp
  css/css_svgvalueimpl.cpp
  css/css_base.cpp
  css/cssparser.cpp
  css/cssstyleselector.cpp
  css/csshelper.cpp
  css/parser.cpp
  css/css_renderstyledeclarationimpl.cpp
  css/css_mediaquery.cpp
  css/css_svgcssparser.cpp
  css/SVGCSSStyleSelector.cpp
  css/css_webfont.cpp
)

# khtml/xml/Makefile.am: khtmlxml

set(khtmlxml_STAT_SRCS
  xml/security_origin.cpp
  xml/dom_docimpl.cpp
  xml/dom_nodeimpl.cpp
  xml/dom_nodelistimpl.cpp
  xml/dom_textimpl.cpp
  xml/dom_elementimpl.cpp
  xml/dom_stringimpl.cpp
  xml/dom2_rangeimpl.cpp
  xml/dom2_traversalimpl.cpp
  xml/xml_tokenizer.cpp
  xml/dom_xmlimpl.cpp
  xml/dom2_eventsimpl.cpp
  xml/dom2_viewsimpl.cpp
  xml/dom_restyler.cpp
  xml/ClassNames.cpp
  xml/dom_position.cpp
  xml/dom_positioniterator.cpp
  xml/dom_selection.cpp
  xml/wa_selectors.cpp
  xml/dom3_xpathimpl.cpp
)

# khtml/imload/Makefile.am: khtmlimload

set(khtmlimload_STAT_SRCS
  imload/imageplane.cpp
  imload/rawimageplane.cpp
  imload/scaledimageplane.cpp
  imload/pixmapplane.cpp
  imload/animprovider.cpp
  imload/imagepainter.cpp
  imload/updater.cpp
  imload/image.cpp
  imload/imagemanager.cpp
  imload/animtimer.cpp
  imload/canvasimage.cpp
)

# khtml/imload/decoders/Makefile.am: decoders

set(decoders_STAT_SRCS
  imload/decoders/jpegloader.cpp
  imload/decoders/pngloader.cpp
  imload/decoders/gifloader.cpp
  imload/decoders/qimageioloader.cpp
)

# khtml/xpath/
set(xpath_STAT_SRCS
  xpath/expression.cpp
  xpath/functions.cpp
  xpath/parsedstatement.cpp
  xpath/path.cpp
  xpath/predicate.cpp
  xpath/step.cpp
  xpath/tokenizer.cpp
  xpath/parser.cpp
  xpath/util.cpp
  xpath/variablereference.cpp
#  xpath/XPathNSResolverImpl.cpp
)

add_subdirectory( css )

include_directories(
   ${PHONON_INCLUDE_DIR}
   ${KWindowSystem_INCLUDE_DIRS}
)

include_directories(
#   ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/misc
  ${CMAKE_CURRENT_SOURCE_DIR}/dom
  ${CMAKE_CURRENT_SOURCE_DIR}/xml
  ${CMAKE_CURRENT_SOURCE_DIR}/html
  ${CMAKE_CURRENT_SOURCE_DIR}/rendering
  ${CMAKE_CURRENT_SOURCE_DIR}/ecma
  ${CMAKE_CURRENT_SOURCE_DIR}/imload
  ${CMAKE_CURRENT_SOURCE_DIR}/imload/decoders
  ${CMAKE_CURRENT_SOURCE_DIR}/java
  ${CMAKE_CURRENT_SOURCE_DIR}/svg
  ${CMAKE_CURRENT_SOURCE_DIR}/svg/graphics
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/graphics
  ${CMAKE_CURRENT_SOURCE_DIR}/platform/text
  ${CMAKE_CURRENT_SOURCE_DIR}/platform
  ${CMAKE_CURRENT_SOURCE_DIR}/compat
  ${CMAKE_CURRENT_SOURCE_DIR}/css
  ${CMAKE_CURRENT_SOURCE_DIR}/xpath
)
include_directories(
 ${CMAKE_CURRENT_BINARY_DIR}
# ${CMAKE_CURRENT_BINARY_DIR}/misc
# ${CMAKE_CURRENT_BINARY_DIR}/dom
# ${CMAKE_CURRENT_BINARY_DIR}/xml
# ${CMAKE_CURRENT_BINARY_DIR}/html
# ${CMAKE_CURRENT_BINARY_DIR}/rendering
# ${CMAKE_CURRENT_BINARY_DIR}/imload
# ${CMAKE_CURRENT_BINARY_DIR}/imload/decoders
  ${CMAKE_CURRENT_BINARY_DIR}/java
# ${CMAKE_CURRENT_BINARY_DIR}/svg
# ${CMAKE_CURRENT_BINARY_DIR}/svg/graphics
# ${CMAKE_CURRENT_BINARY_DIR}/platform/graphics
# ${CMAKE_CURRENT_BINARY_DIR}/platform/text
# ${CMAKE_CURRENT_SOURCE_DIR}/platform
# ${CMAKE_CURRENT_BINARY_DIR}/compat
  ${CMAKE_CURRENT_BINARY_DIR}/css
)

include_directories(${JPEG_INCLUDE_DIR} ${GIF_INCLUDE_DIR} ${PNG_INCLUDE_DIR} )

set( KDELIBSUFF ${LIB_SUFFIX} )
configure_file(html/ksslconfig.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/ksslconfig.h )

add_subdirectory( kmultipart )
add_subdirectory( java )

add_definitions(-DENABLE_VIDEO)

########### libkhtml ###############

set(khtml_LIB_SRCS
   khtmlview.cpp
   khtmlviewbar.cpp
   khtmlviewbarwidget.cpp
   khtml_part.cpp
   khtml_childframe.cpp
   khtml_run.cpp
   khtml_global.cpp
   khtml_settings.cpp
   khtml_filter.cpp
   khtml_events.cpp
   khtml_ext.cpp
   khtml_pagecache.cpp
   khtml_printsettings.cpp
   khtml_iface.cpp
   khtmlimage.cpp
   kjserrordlg.cpp
   ${xpath_STAT_SRCS}
   ${khtmlfind_STAT_SRCS}
   ${passwordbar_STAT_SRCS}
   ${khtmlxml_STAT_SRCS}
   ${khtmlhtml_STAT_SRCS}
   ${khtmlrender_STAT_SRCS}
   ${khtmlcss_STAT_SRCS}
   ${khtmlmisc_STAT_SRCS}
   ${khtmlediting_STAT_SRCS}
   ${kjs_html_STAT_SRCS}
   ${khtmldom_STAT_SRCS}
   ${khtmlimload_STAT_SRCS}
   ${decoders_STAT_SRCS}
   ${khtmlgraphics_STAT_SRCS} # for WebCore compatibility
   ${khtmlsvg_STAT_SRCS} # for SVG
   ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KHtmlPart.xml # just so that it gets generated
   )

qt5_generate_dbus_interface(khtml_iface.h org.kde.KHtmlPart.xml)

if(kdelibs_SOURCE_DIR)
    qt5_generate_dbus_interface(${CMAKE_SOURCE_DIR}/tier3/kio/src/ioslaves/http/kcookiejar/kcookieserver.h org.kde.KCookieServer.xml)
    set(KCookieServer_DBUS_INTERFACE ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KCookieServer.xml)
endif()

qt5_add_dbus_interface(khtml_LIB_SRCS ${KCookieServer_DBUS_INTERFACE} kcookiejar_interface)


qt5_wrap_ui(khtml_LIB_SRCS
   kjserrordlgbase.ui
   htmlpageinfo.ui
   html/keygenwizard.ui
   html/keygenwizard2.ui
)

add_library(KF5KHtml ${khtml_LIB_SRCS})
add_library(KF5::KHtml ALIAS KF5KHtml)

target_link_libraries(KF5KHtml
  PUBLIC
    Qt5::Gui
    KF5::KIOCore
    KF5::JS
    KF5::I18n
    KF5::Parts
  PRIVATE
    ${PHONON_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${GIF_LIBRARIES}
    ${PNG_LIBRARIES}
    ${X11_LIBRARIES}
    Qt5::PrintSupport
    Qt5::X11Extras
    KF5::Archive
    KF5::SonnetCore
    KF5::WidgetsAddons
    KF5::WindowSystem
    KF5::Wallet
    KF5::IconThemes
    KF5::Notifications
    KF5::Bookmarks
    KF5::KIOWidgets # KUrlRequester
    KF5::GlobalAccel
)
if (KJS_DEBUGGER)
  target_link_libraries(KF5KHtml PRIVATE KF5::KTextEditor)
endif()

set_target_properties(KF5KHtml PROPERTIES VERSION ${KHTML_VERSION_STRING}
                                          SOVERSION ${KHTML_SOVERSION}
                                          EXPORT_NAME KHtml
                     )

generate_export_header(KF5KHtml BASE_NAME KHtml)


macro(CREATE_LUT _in_FILE _out_FILE)

   add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${_out_FILE}
      COMMAND ${PERL_EXECUTABLE} ${KJS_CREATE_HASH_TABLE} ${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE} > ${CMAKE_CURRENT_BINARY_DIR}/${_out_FILE}
      DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE}
   )
   macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE} ${CMAKE_CURRENT_BINARY_DIR}/${_out_FILE} )
endmacro(CREATE_LUT)


create_lut(ecma/kjs_dom.cpp kjs_dom.lut.h)
create_lut(ecma/kjs_html.cpp kjs_html.lut.h)
create_lut(ecma/kjs_window.cpp kjs_window.lut.h)
create_lut(ecma/kjs_css.cpp kjs_css.lut.h)
create_lut(ecma/kjs_events.cpp kjs_events.lut.h)
create_lut(ecma/kjs_navigator.cpp kjs_navigator.lut.h)
create_lut(ecma/kjs_mozilla.cpp kjs_mozilla.lut.h)
create_lut(ecma/kjs_range.cpp kjs_range.lut.h)
create_lut(ecma/kjs_traversal.cpp kjs_traversal.lut.h)
create_lut(ecma/kjs_views.cpp kjs_views.lut.h)
create_lut(ecma/xmlhttprequest.cpp xmlhttprequest.lut.h)
create_lut(ecma/xmlserializer.cpp xmlserializer.lut.h)
create_lut(ecma/domparser.cpp domparser.lut.h)
create_lut(ecma/kjs_context2d.cpp kjs_context2d.lut.h)
create_lut(ecma/kjs_xpath.cpp kjs_xpath.lut.h)
create_lut(ecma/kjs_arraybuffer.cpp kjs_arraybuffer.lut.h)
create_lut(ecma/kjs_arraybufferview.cpp kjs_arraybufferview.lut.h)
create_lut(ecma/kjs_clientrect.cpp kjs_clientrect.lut.h)

macro(create_js_binding _in_FILE)
   get_filename_component(_in_filename ${_in_FILE} NAME_WE)
   set(_out_h_FILE "${CMAKE_CURRENT_BINARY_DIR}/JS${_in_filename}.h")
   set(_out_cpp_FILE "${CMAKE_CURRENT_BINARY_DIR}/JS${_in_filename}.cpp")
   set(_scripts_dir "${CMAKE_CURRENT_SOURCE_DIR}/bindings/scripts")
   add_custom_command(OUTPUT ${_out_h_FILE} ${_out_cpp_FILE}
      COMMAND ${PERL_EXECUTABLE} -I${_scripts_dir}
              ${_scripts_dir}/generate-bindings.pl
              --include=html
              --generator=JS
              --outputdir=${CMAKE_CURRENT_BINARY_DIR}
              --preprocessor=\"${QT_MOC_EXECUTABLE} -E\"
              --defines=ENABLE_VIDEO
              ${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE}
      MAIN_DEPENDENCY ${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE}
      DEPENDS ${_scripts_dir}/CodeGeneratorJS.pm
      )
   macro_add_file_dependencies(${CMAKE_CURRENT_SOURCE_DIR}/${_in_FILE} ${_out_h_FILE} ${_out_cpp_FILE})
endmacro(create_js_binding)

create_js_binding(html/TimeRanges.idl)
create_js_binding(html/MediaError.idl)
create_js_binding(html/HTMLMediaElement.idl)
create_js_binding(html/HTMLAudioElement.idl)
create_js_binding(html/HTMLVideoElement.idl)
#create_js_binding(svg/SVGRectElement.idl)
#create_js_binding(svg/SVGRect.idl)
#create_js_binding(svg/SVGElement.idl)
#create_js_binding(svg/SVGDocument.idl)
#create_js_binding(svg/SVGAngle.idl)
#create_js_binding(svg/SVGSVGElement.idl)
#create_js_binding(svg/SVGLength.idl)
#create_js_binding(svg/SVGNumber.idl)
#create_js_binding(svg/SVGPoint.idl)
#create_js_binding(svg/SVGMatrix.idl)
#create_js_binding(svg/SVGTransform.idl)
#create_js_binding(svg/SVGException.idl)
#create_js_binding(svg/SVGPaint.idl)
#create_js_binding(svg/SVGNumberList.idl)
#create_js_binding(svg/SVGAnimatedLength.idl)
#create_js_binding(svg/SVGRectElement.idl)
#create_js_binding(svg/SVGTransformList.idl)
#create_js_binding(svg/SVGAnimatedTransformList.idl)
##create_js_binding(svg/SVGLocatable.idl)
##create_js_binding(svg/SVGTransformable.idl)
#create_js_binding(svg/SVGAnimatedString.idl)
#create_js_binding(svg/SVGAnimatedBoolean.idl)
#create_js_binding(svg/SVGStringList.idl)
#create_js_binding(svg/SVGEllipseElement.idl)
#create_js_binding(svg/SVGCircleElement.idl)
#create_js_binding(svg/SVGLineElement.idl)
#create_js_binding(svg/SVGScriptElement.idl)
#create_js_binding(svg/SVGPathSeg.idl)
#create_js_binding(svg/SVGPathElement.idl)
#create_js_binding(svg/SVGAnimatedNumber.idl)
#create_js_binding(svg/SVGPathSegArcAbs.idl)
#create_js_binding(svg/SVGPathSegArcRel.idl)
#create_js_binding(svg/SVGPathSegClosePath.idl)
#create_js_binding(svg/SVGPathSegCurvetoCubicAbs.idl)
#create_js_binding(svg/SVGPathSegCurvetoCubicRel.idl)
#create_js_binding(svg/SVGPathSegCurvetoCubicSmoothAbs.idl)
#create_js_binding(svg/SVGPathSegCurvetoCubicSmoothRel.idl)
#create_js_binding(svg/SVGPathSegCurvetoQuadraticAbs.idl)
#create_js_binding(svg/SVGPathSegCurvetoQuadraticRel.idl)
#create_js_binding(svg/SVGPathSegCurvetoQuadraticSmoothAbs.idl)
#create_js_binding(svg/SVGPathSegCurvetoQuadraticSmoothRel.idl)
#create_js_binding(svg/SVGPathSegLinetoAbs.idl)
#create_js_binding(svg/SVGPathSegLinetoHorizontalAbs.idl)
#create_js_binding(svg/SVGPathSegLinetoHorizontalRel.idl)
#create_js_binding(svg/SVGPathSegLinetoRel.idl)
#create_js_binding(svg/SVGPathSegLinetoVerticalAbs.idl)
#create_js_binding(svg/SVGPathSegLinetoVerticalRel.idl)
#create_js_binding(svg/SVGPathSegList.idl)
#create_js_binding(svg/SVGPathSegMovetoAbs.idl)
#create_js_binding(svg/SVGPathSegMovetoRel.idl)



########### khtmlpart ###############

set(khtmlpart_PART_SRCS khtml_factory.cpp )


add_library(khtmlpart MODULE ${khtmlpart_PART_SRCS})
set_target_properties(khtmlpart PROPERTIES PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX}")

target_link_libraries(khtmlpart KF5::KHtml KF5::XmlGui KF5::TextWidgets KF5::Parts KF5::I18n)

install(TARGETS khtmlpart  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### next target ###############

# Note that khtmlimage.cpp is part of libkhtml because it uses internal objects (render tree and loader)
# Only the entry point is separated into khtmlimage_init.cpp
add_library(khtmlimagepart MODULE khtmlimage_init.cpp)


target_link_libraries(khtmlimagepart KF5::KHtml KF5::WidgetsAddons KF5::XmlGui KF5::TextWidgets KF5::Parts KF5::I18n)

install(TARGETS khtmlimagepart  DESTINATION ${PLUGIN_INSTALL_DIR} )

########### next target ###############

add_library(khtmladaptorpart MODULE khtmladaptorpart.cpp)

target_link_libraries(khtmladaptorpart KF5::Parts KF5::JS KF5::I18n KF5::XmlGui)

install(TARGETS khtmladaptorpart DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install(TARGETS KF5KHtml EXPORT KF5KHtmlTargets ${INSTALL_TARGETS_DEFAULT_ARGS})

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/khtml_export.h khtmlview.h khtml_part.h khtml_events.h khtml_settings.h khtmldefaults.h misc/kencodingdetector.h
         DESTINATION  ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
install( FILES dom/dom_exception.h dom/html_base.h dom/html_inline.h dom/css_rule.h dom/dom_html.h
               dom/html_block.h dom/html_list.h dom/css_stylesheet.h dom/dom_misc.h dom/html_document.h
               dom/html_misc.h dom/css_value.h dom/dom_node.h dom/html_element.h dom/html_object.h
               dom/dom_core.h dom/dom_string.h dom/html_form.h dom/html_table.h dom/dom_doc.h
               dom/dom_text.h dom/html_head.h dom/dom_element.h dom/dom_xml.h dom/html_image.h
               dom/dom2_range.h dom/dom2_traversal.h dom/dom2_events.h dom/dom2_views.h
         DESTINATION  ${INCLUDE_INSTALL_DIR}/dom  COMPONENT Devel)

install( FILES khtml.desktop khtmlimage.desktop
         khtmladaptorpart.desktop DESTINATION  ${SERVICES_INSTALL_DIR} )
install( FILES khtml.rc khtml_browser.rc  DESTINATION  ${DATA_INSTALL_DIR}/khtml )

install( FILES ${CMAKE_CURRENT_BINARY_DIR}/org.kde.KHtmlPart.xml DESTINATION ${DBUS_INTERFACES_INSTALL_DIR} )

install( FILES error.html DESTINATION ${DATA_INSTALL_DIR}/khtml )

install( FILES khtmlrc DESTINATION ${CONFIG_INSTALL_DIR} )


