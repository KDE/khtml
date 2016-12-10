#include "kjserrordlg.h"

#include <QPushButton>

KJSErrorDlg::KJSErrorDlg(QWidget *parent)
    : QDialog(parent)
{
    setupUi(this);
    QPushButton *clear = _buttonBox->addButton(i18n("C&lear"), QDialogButtonBox::ActionRole);
    clear->setIcon(QIcon::fromTheme("edit-clear-locationbar-ltr"));
    connect(clear, SIGNAL(clicked()), this, SLOT(clear()));
    connect(_buttonBox, SIGNAL(rejected()), this, SLOT(hide()));
    init();
}

void KJSErrorDlg::addError(const QString &error)
{
    _errorText->append(error);
}

void KJSErrorDlg::setURL(const QString &url)
{
    _url->setText(url);
}

void KJSErrorDlg::clear()
{
    _errorText->clear();
    init();
}

void KJSErrorDlg::init()
{
    _errorText->setAcceptRichText(false);
}
