#ifndef TESTKHTML_H
#define TESTKHTML_H

#include <QDebug>
#include <khtml_part.h>
#include <QList>
#include <QMap>
#include <QTime>

/**
 * @internal
 */
class Dummy : public QObject
{
  Q_OBJECT
public:
  Dummy( KHTMLPart *part ) : QObject( part ) { m_part = part; }

private Q_SLOTS:
  void slotOpenURL( const QUrl &url, const KParts::OpenUrlArguments& args, const KParts::BrowserArguments& browserArgs )
  {
      m_part->setArguments( args );
      m_part->browserExtension()->setBrowserArguments( browserArgs );
    m_part->openUrl( url );
  }
  void reload()
  {
      KParts::OpenUrlArguments args;
      args.setReload( true );
      m_part->setArguments( args );
      m_part->openUrl( m_part->url() );
  }
  
  void toggleNavigable(bool s)
  {
      m_part->setCaretMode(s);
  }

  void toggleEditable(bool s)
  {
      qDebug() << "editable: " << s;
      m_part->setEditable(s);
  }

  void doBenchmark();

  void handleDone();

  void nextRun();
private:
  KHTMLPart *m_part;
  QList<QString> filesToBenchmark;
  QMap<QString, QList<int> > results;
  int                 benchmarkRun;
  QTime               loadTimer;

};

#endif
