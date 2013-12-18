#!/usr/bin/python
# -*- coding: utf-8 -*-
# misc/gennames.py generates the appropriate ID_ and ATTR_ constants
# as well as static mappings. This generates WC compatibility stuff ---
# the Names constants.

attrsList = []
tagsList = []

f = open("svgtags.in", "r")
for i in f.xreadlines():
    i = i.strip()
    if i == "": continue
    if i.startswith("#"): continue
    tagsList.append(i)
f.close()

f = open("svgattrs.in", "r")
for i in f.xreadlines():
    i = i.strip()
    if i == "": continue
    if i.startswith("#"): continue
    attrsList.append(i)
f.close()

f = open("SVGNames.h", "w")
f.write("#ifndef SVGNAMES_H\n")
f.write("#define SVGNAMES_H\n")
f.write("\n")
f.write("#include \"misc/htmlnames.h\"\n")
f.write("#include \"dom/QualifiedName.h\"\n")
f.write("#include \"xml/Document.h\"\n")
f.write("\n")
f.write("#define idAttr ATTR_ID\n")
f.write("\n")
f.write("namespace WebCore {\n")
f.write("    namespace SVGNames {\n")
f.write("        void init();\n");
initString = ""
definition = ""
definitionHtml = ""
definitionXLink = ""
for i in tagsList:
    f.write(" " * 8 + "extern DOM::QualifiedName %sTag;\n" % (i.replace("-", "_")))
    definition = definition + (" " * 8 + "DOM::QualifiedName %sTag;\n" % (i.replace("-", "_")))
    # ID_TEXT is hidden, we need to use ATTR_TEXT instead
    if i == 'text':
        initString = initString + ("            %sTag = DOM::QualifiedName(DOM::makeId(DOM::svgNamespace, DOM::localNamePart(ATTR_%s)), DOM::emptyPrefixName);\n" % (i.replace("-", "_"), i.upper().replace("-", "_")))
    else:
        initString = initString + ("            %sTag = DOM::QualifiedName(DOM::makeId(DOM::svgNamespace, ID_%s), DOM::emptyPrefixName);\n" % (i.replace("-", "_"), i.upper().replace("-", "_")))
for i in attrsList:
    f.write(" " * 8 + "extern DOM::QualifiedName %sAttr;\n" % (i.replace("-", "_")))
    definition = definition + (" " * 8 + "DOM::QualifiedName %sAttr;\n" % (i.replace("-", "_")))
    initString = initString + ("            %sAttr = DOM::QualifiedName(ATTR_%s, DOM::emptyPrefixName);\n" % (i.replace("-", "_"), i.upper().replace("-", "_")))
f.write("    }\n")
f.write("    namespace HTMLNames {\n")
f.write(" " * 8 + "extern DOM::QualifiedName classAttr;\n")
definitionHtml = definitionHtml + (" " * 8 + "DOM::QualifiedName classAttr;\n")
initString = initString + ("            WebCore::HTMLNames::classAttr = DOM::QualifiedName(ATTR_CLASS, DOM::emptyPrefixName);\n")
f.write("    }\n")
f.write("}\n")
f.write("\n")
f.write("#endif\n")


############################################################
#generate XLinkAttrs
############################################################

xlinkAttrs = []

f = open("xlinkattrs.in", "r")
for i in f.xreadlines():
    i = i.strip()
    if i == "": continue
    if i.startswith("#"): continue
    xlinkAttrs.append(i)
f.close()

f = open("XLinkNames.h", "w")
f.write("#ifndef XLinkNames_H\n")
f.write("#define XLinkNames_H\n")
f.write("\n")
f.write("#include \"misc/htmlnames.h\"\n")
f.write("#include \"dom/QualifiedName.h\"\n")
f.write("#include \"xml/Document.h\"\n")
f.write("\n")
f.write("namespace WebCore {\n")
f.write("    namespace XLinkNames {\n")
for i in xlinkAttrs:
    f.write(" " * 8 + "extern DOM::QualifiedName %sAttr;\n" % (i.replace("-", "_")))
    definitionXLink = definitionXLink + (" " * 8 + "DOM::QualifiedName %sAttr;\n" % (i.replace("-", "_")))
    initString = initString + ("            WebCore::XLinkNames::%sAttr = DOM::QualifiedName(ATTR_XLINK_%s, DOM::emptyPrefixName);\n" % (i.replace("-", "_"), i.upper().replace("-", "_")))
f.write("    }\n")
f.write("}\n")
f.write("\n")
f.write("#endif\n")
f.close()

############################
# generate init function
############################

f = open("SVGNames.cpp", "w")
f.write("#include \"svg/SVGNames.h\"\n\n")
f.write("#include \"svg/XLinkNames.h\"\n\n")
f.write("namespace WebCore {")
f.write("""
    namespace HTMLNames {
%s
    }
    namespace XLinkNames {
%s
    }
    namespace SVGNames {
%s
        void init()
        {
%s
        }
    }\n""" % (definitionHtml, definitionXLink, definition, initString))
f.write("}\n")
f.write("\n")
f.close()

