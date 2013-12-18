#ifndef TESTKHTMLNEW_H
#define TESTKHTMLNEW_H

#include <kxmlguiwindow.h>

class KHTMLPart;
class QComboBox;
class QToolButton;
class QLineEdit;
class QUrl;
class KHTMLGlobal;
class QLabel;
class QMovie;

namespace KParts
{
struct BrowserArguments;
class OpenUrlArguments;
}

/**
 * @internal
 */
class TestKHTML : public KXmlGuiWindow
{
    Q_OBJECT
public:
    TestKHTML();
    ~TestKHTML();

    KHTMLPart *doc() const;

public Q_SLOTS:
    void openUrl(const QUrl &url, const KParts::OpenUrlArguments&, const KParts::BrowserArguments &args);
    void openUrl(const QUrl &url);
    void openUrl(const QString &url);
    void openUrl();

    void reload();
    void toggleNavigable(bool s);
    void toggleEditable(bool s);

private Q_SLOTS:
    void startLoading();
    void finishedLoading();

private:
    void setupActions();

    KHTMLPart *m_part;
    QComboBox *m_combo;
    QToolButton *m_goButton;
    QToolButton *m_reloadButton;
    QLineEdit *m_comboEdit;
    QLabel *m_indicator;
    QMovie *m_movie;

#ifndef __KDE_HAVE_GCC_VISIBILITY
    KHTMLGlobal *m_factory;
#endif

};

#endif
