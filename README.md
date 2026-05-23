# TML

HTML rendering engine

## Introduction

TML is a web rendering engine, based on the KParts technology and using KJS for JavaScript support.


## Usage

If you are using CMake, you need to have

    find_package(KF5THtml NO_MODULE)

(or similar) in your CMakeLists.txt file, and you need to link to KF5::THtml.

To use TML in your application, create an instance of THtmlPart, embed it in
your application like any other KPart, and call methods to control what it
displays:

    QUrl url("https://www.kde.org");
    THtmlPart *w = new THtmlPart();
    w->openUrl(url);
    w->view()->resize(500, 400);
    w->show();


## Alternatives

Note that using THtmlPart may introduce security vulnerabilities and unnecessary
bloat to your application. Qt's text widgets are rich-text capable, and will
interpret a limited subset of HTML.

Another option is to use KDEWebKit. WebKit is a fork of the original KHTML architecture with substantial
industry support.
