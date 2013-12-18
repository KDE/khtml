#include "kjserrordlg.h"

KJSErrorDlg::KJSErrorDlg( QWidget *parent )
  : QDialog( parent )
{
  setupUi( this );
  connect(_clear,SIGNAL(clicked()),this,SLOT(clear()));
  connect(_close,SIGNAL(clicked()),this,SLOT(hide()));
  init();
}

void KJSErrorDlg::addError( const QString & error )
{
  _errorText->append(error);
}

void KJSErrorDlg::setURL( const QString & url )
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
