// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2000 Harri Porten (porten@kde.org)
 *  Copyright (c) 2000 Daniel Molkentin (molkentin@kde.org)
 *  Copyright (c) 2000 Stefan Schimanski (schimmi@kde.org)
 *  Copyright (C) 2003 Apple Computer, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "kjs_navigator.h"
#include "kjs_navigator.lut.h"

#include <QLocale>


#include <kconfig.h>
#include <kconfiggroup.h>
#include <QDebug>

#include <kprotocolmanager.h>
#include <kservice.h>
#include <kservicetypetrader.h>
#include <kjs/lookup.h>
#include <kjs_binding.h>
#include <khtml_part.h>
#include <sys/utsname.h>
#include <qstandardpaths.h>

using namespace KJS;

namespace KJS {

    // All objects that need plugin info must inherit from PluginBase
    // Its ctor and dtor take care of the refcounting on the static lists.
    class PluginBase : public JSObject {
    public:
        PluginBase(ExecState *exec, bool loadPluginInfo);
        virtual ~PluginBase();

        struct MimeClassInfo;
        struct PluginInfo;

        struct MimeClassInfo {
            QString type;
            QString desc;
            QString suffixes;
            PluginInfo *plugin;
        };

        struct PluginInfo {
            QString name;
            QString file;
            QString desc;
            QList<const MimeClassInfo*> mimes;
        };

        static QList<const PluginInfo*> *plugins;
        static QList<const MimeClassInfo*> *mimes;

    private:
        static int m_refCount;
    };


    class Plugins : public PluginBase {
    public:
        Plugins(ExecState *exec, bool pluginsEnabled)
          : PluginBase(exec, pluginsEnabled),
            m_pluginsEnabled(pluginsEnabled) {}

        using KJS::JSObject::getOwnPropertySlot;
        virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        JSValue *getValueProperty(ExecState *exec, int token) const;
        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
        static JSValue *pluginByName( ExecState* exec, const QString& name );
        bool pluginsEnabled() const { return m_pluginsEnabled; }
    private:
        static JSValue *indexGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
        static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
        bool m_pluginsEnabled;
    };


    class MimeTypes : public PluginBase {
    public:
        MimeTypes(ExecState *exec, bool pluginsEnabled)
          : PluginBase(exec, pluginsEnabled),
            m_pluginsEnabled(pluginsEnabled) {}
        using KJS::JSObject::getOwnPropertySlot;
        virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
        JSValue *getValueProperty(ExecState *exec, int token) const;
        static JSValue *mimeTypeByName( ExecState* exec, const QString& name );
        bool pluginsEnabled() const { return m_pluginsEnabled; }
    private:
        static JSValue *indexGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
        static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
        bool m_pluginsEnabled;
    };


    class Plugin : public PluginBase {
    public:
        Plugin(ExecState *exec, const PluginBase::PluginInfo *info)
          : PluginBase( exec, true )
        { m_info = info; }
        using KJS::JSObject::getOwnPropertySlot;
        virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
        JSValue *mimeByName(ExecState* exec, const QString& name ) const;
        JSValue *getValueProperty(ExecState *exec, int token) const;
        const PluginBase::PluginInfo *pluginInfo() const { return m_info; }
    private:
        const PluginBase::PluginInfo *m_info;
        static JSValue *indexGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
        static JSValue *nameGetter(ExecState *, JSObject*, const Identifier&, const PropertySlot&);
    };


    class MimeType : public PluginBase {
    public:
        MimeType(ExecState *exec, const PluginBase::MimeClassInfo *info)
          : PluginBase( exec, true )
        { m_info = info; }
        using KJS::JSObject::getOwnPropertySlot;
        virtual bool getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot);
        virtual const ClassInfo* classInfo() const { return &info; }
        static const ClassInfo info;
        JSValue *getValueProperty(ExecState *exec, int token) const;
    private:
        const PluginBase::MimeClassInfo *m_info;
    };

}


QList<const PluginBase::PluginInfo*> *KJS::PluginBase::plugins;
QList<const PluginBase::MimeClassInfo*> *KJS::PluginBase::mimes;
int KJS::PluginBase::m_refCount;

const ClassInfo Navigator::info = { "Navigator", 0, &NavigatorTable, 0 };
/*
@begin NavigatorTable 12
  appCodeName	Navigator::AppCodeName	DontDelete|ReadOnly
  appName	Navigator::AppName	DontDelete|ReadOnly
  appVersion	Navigator::AppVersion	DontDelete|ReadOnly
  language	Navigator::Language	DontDelete|ReadOnly
  userAgent	Navigator::UserAgent	DontDelete|ReadOnly
  userLanguage	Navigator::UserLanguage	DontDelete|ReadOnly
  browserLanguage Navigator::BrowserLanguage	DontDelete|ReadOnly
  platform	Navigator::Platform	DontDelete|ReadOnly
  cpuClass      Navigator::CpuClass     DontDelete|ReadOnly
  plugins	Navigator::_Plugins	DontDelete|ReadOnly
  mimeTypes	Navigator::_MimeTypes	DontDelete|ReadOnly
  product	Navigator::Product	DontDelete|ReadOnly
  vendor	Navigator::Vendor	DontDelete|ReadOnly
  vendorSub	Navigator::VendorSub	DontDelete|ReadOnly
  productSub    Navigator::ProductSub   DontDelete|ReadOnly
  cookieEnabled	Navigator::CookieEnabled DontDelete|ReadOnly
  javaEnabled	Navigator::JavaEnabled	DontDelete|Function 0
@end
*/
KJS_IMPLEMENT_PROTOFUNC(NavigatorFunc)

Navigator::Navigator(ExecState *exec, KHTMLPart *p)
  : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype()), m_part(p) { }

bool Navigator::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "Navigator::getOwnPropertySlot " << propertyName.ascii();
#endif
  return getStaticPropertySlot<NavigatorFunc, Navigator, JSObject>(exec, &NavigatorTable, this, propertyName, slot);
}

JSValue *Navigator::getValueProperty(ExecState *exec, int token) const
{
  QUrl url = m_part->url();
  QString host = url.host();
  if (host.isEmpty())
     host = "localhost";
  QString userAgent  = KProtocolManager::userAgentForHost(host);
// ### get the following from the spoofing UA files as well.
  QString appName = "Netscape";
  QString product = "Gecko";
  QString productSub = "20030107";
  QString vendor = "KDE";
  QString vendorSub = "";

  switch (token) {
  case AppCodeName:
    return jsString("Mozilla");
  case AppName:
    // If we find "Mozilla" but not "(compatible, ...)" we are a real Netscape
    if (userAgent.indexOf(QLatin1String("Mozilla")) >= 0 &&
        userAgent.indexOf(QLatin1String("compatible")) == -1)
    {
      //qDebug() << "appName -> Mozilla";
      return jsString("Netscape");
    }
    if (userAgent.indexOf(QLatin1String("Microsoft")) >= 0 ||
        userAgent.indexOf(QLatin1String("MSIE")) >= 0)
    {
      //qDebug() << "appName -> IE";
      return jsString("Microsoft Internet Explorer");
    }
    //qDebug() << "appName -> Default";
    return jsString(appName);
  case AppVersion:
    // We assume the string is something like Mozilla/version (properties)
    return jsString(userAgent.mid(userAgent.indexOf('/') + 1));
  case Product:
    // We are pretending to be Mozilla or Safari
    if (userAgent.indexOf(QLatin1String("Mozilla")) >= 0 &&
        userAgent.indexOf(QLatin1String("compatible")) == -1)
    {
        return jsString("Gecko");
    }
    // When spoofing as IE, we use jsUndefined().
    if (userAgent.indexOf(QLatin1String("Microsoft")) >= 0 ||
        userAgent.indexOf(QLatin1String("MSIE")) >= 0)
    {
        return jsUndefined();
    }
    return jsString(product);
  case ProductSub:
    {
      int ix = userAgent.indexOf("Gecko");
      if (ix >= 0 && userAgent.length() >= ix+14 && userAgent.at(ix+5) == '/' &&
          userAgent.indexOf(QRegExp("\\d{8}"), ix+6) == ix+6)
      {
          // We have Gecko/<productSub> in the UA string
          return jsString(userAgent.mid(ix+6, 8));
      }
    }
    return jsString(productSub);
  case Vendor:
    if (userAgent.indexOf(QLatin1String("Safari")) >= 0)
    {
        return jsString("Apple Computer, Inc.");
    }
    return jsString(vendor);
  case VendorSub:
    return jsString(vendorSub);
  case BrowserLanguage:
  case Language:
  case UserLanguage:
    return jsString(QLocale::languageToString(QLocale().language()));
  case UserAgent:
    return jsString(userAgent);
  case Platform:
    // yet another evil hack, but necessary to spoof some sites...
    if ( (userAgent.indexOf(QLatin1String("Win"),0,Qt::CaseInsensitive)>=0) )
      return jsString("Win32");
    else if ( (userAgent.indexOf(QLatin1String("Macintosh"),0,Qt::CaseInsensitive)>=0) ||
              (userAgent.indexOf(QLatin1String("Mac_PowerPC"),0,Qt::CaseInsensitive)>=0) )
      return jsString("MacPPC");
    else
    {
        struct utsname name;
        int ret = uname(&name);
        if ( ret >= 0 )
            return jsString(QString::fromLatin1("%1 %2").arg(name.sysname).arg(name.machine));
        else // can't happen
            return jsString("Unix X11");
    }
  case CpuClass:
  {
    struct utsname name;
    int ret = uname(&name);
    if ( ret >= 0 )
      return jsString(name.machine);
    else // can't happen
      return jsString("unknown");
  }
  case _Plugins:
    return new Plugins(exec, m_part->pluginsEnabled());
  case _MimeTypes:
    return new MimeTypes(exec, m_part->pluginsEnabled());
  case CookieEnabled:
    return jsBoolean(true); /// ##### FIXME
  default:
    // qDebug() << "WARNING: Unhandled token in DOMEvent::getValueProperty : " << token;
    return jsNull();
  }
}

/*******************************************************************/

PluginBase::PluginBase(ExecState *exec, bool loadPluginInfo)
  : JSObject(exec->lexicalInterpreter()->builtinObjectPrototype() )
{
    if ( loadPluginInfo && !plugins ) {
        plugins = new QList<const PluginInfo*>;
        mimes = new QList<const MimeClassInfo*>;

        // read in using KServiceTypeTrader
        const KService::List offers = KServiceTypeTrader::self()->query("Browser/View");
        KService::List::const_iterator it;
        for ( it = offers.begin(); it != offers.end(); ++it ) {

            QVariant pluginsinfo = (**it).property( "X-KDE-BrowserView-PluginsInfo" );
            if ( !pluginsinfo.isValid() ) {
                // <backwards compatible>
                if ((**it).library() == QLatin1String("libnsplugin"))
                    pluginsinfo = QVariant("nsplugins/pluginsinfo");
                else
                // </backwards compatible>
                    continue;
            }
            // read configuration
            QString fn = QStandardPaths::locate(QStandardPaths::GenericDataLocation, pluginsinfo.toString());
            const KSharedConfig::Ptr sc = KSharedConfig::openConfig(fn);
            const int num = sc->group("").readEntry("number", 0);
            for ( int n = 0; n < num; n++ ) {
                const KConfigGroup kc = sc->group(QString::number(n));
                PluginInfo *plugin = new PluginInfo;

                plugin->name = kc.readEntry("name");
                plugin->file = kc.readPathEntry("file", QString());
                plugin->desc = kc.readEntry("description");

                plugins->append( plugin );

                const QStringList types = kc.readXdgListEntry("mime");
                QStringList::const_iterator type;
                for ( type=types.begin(); type!=types.end(); ++type ) {

                    // get mime information
                    const QStringList tokens = (*type).split(':', QString::KeepEmptyParts);
                    if ( tokens.count() < 3 ) // we need 3 items
                        continue;

                    MimeClassInfo *mime = new MimeClassInfo;
                    QStringList::ConstIterator token = tokens.begin();
                    mime->type = (*token).toLower();
                    //qDebug() << "mime->type=" << mime->type;
                    ++token;

                    mime->suffixes = *token;
                    ++token;

                    mime->desc = *token;
                    ++token;

                    mime->plugin = plugin;

                    mimes->append( mime );
                    plugin->mimes.append( mime );

                }
            }
        }
    }

    m_refCount++;
}

PluginBase::~PluginBase()
{
    m_refCount--;
    if ( m_refCount==0 ) {
        if (plugins)
            qDeleteAll(*plugins);
        if (mimes)
            qDeleteAll(*mimes);
        delete plugins;
        delete mimes;
        plugins = 0;
        mimes = 0;
    }
}


/*******************************************************************/

const ClassInfo Plugins::info = { "PluginArray", 0, &PluginsTable, 0 };
/*
@begin PluginsTable 4
  length	Plugins_Length  	DontDelete|ReadOnly
  refresh	Plugins_Refresh 	DontDelete|Function 0
  item  	Plugins_Item    	DontDelete|Function 1
  namedItem  	Plugins_NamedItem   	DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(PluginsFunc)

JSValue *Plugins::getValueProperty(ExecState*, int token) const
{
  assert(token == Plugins_Length);
  Q_UNUSED(token);
  if (pluginsEnabled())
    return jsNumber(plugins->count());
  else
    return jsNumber(0);
}

JSValue *Plugins::indexGetter(ExecState* exec, JSObject*, const Identifier& /*propertyName*/, const PropertySlot& slot)
{
  return new Plugin(exec, plugins->at(slot.index()));
}

JSValue *Plugins::nameGetter(ExecState* exec, JSObject*, const Identifier& propertyName, const PropertySlot& /*slot*/)
{
  return pluginByName(exec, propertyName.qstring());
}

bool Plugins::getOwnPropertySlot(ExecState *exec, const Identifier &propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "Plugins::getOwnPropertySlot " << propertyName.qstring();
#endif
  if (getStaticOwnPropertySlot<PluginsFunc, Plugins>(&PluginsTable, this, propertyName, slot))
      return true;

  if (pluginsEnabled()) {
    // plugins[#]
    bool ok;
    unsigned int i = propertyName.toArrayIndex(&ok);
    if (ok && i < static_cast<unsigned>(plugins->count())) {
      slot.setCustomIndex(this, i, indexGetter);
      return true;
    }

    // plugin[name]
    QList<const PluginInfo*>::const_iterator it, end = plugins->constEnd();
    for (it = plugins->constBegin(); it != end; ++it) {
      if ((*it)->name == propertyName.qstring()) {
        slot.setCustom(this, nameGetter);
        return true;
      }
    }
  }

  return PluginBase::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *Plugins::pluginByName( ExecState* exec, const QString& name )
{
  QList<const PluginInfo*>::const_iterator it, end = plugins->constEnd();
  for (it = plugins->constBegin(); it != end; ++it) {
    if ((*it)->name == name)
      return new Plugin(exec, *it);
  }
  return jsUndefined();
}

JSValue *PluginsFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::Plugins, thisObj );

  KJS::Plugins* base = static_cast<KJS::Plugins *>(thisObj);
  if (!base->pluginsEnabled()) return jsUndefined();

  switch( id ) {
  case Plugins_Refresh:
    return jsUndefined(); //## TODO
  case Plugins_Item:
  {
    bool ok;
    unsigned int i = args[0]->toString(exec).toArrayIndex(&ok);
    if (ok && i < static_cast<unsigned>(base->plugins->count()))
      return new Plugin( exec, base->plugins->at(i) );
    return jsUndefined();
  }
  case Plugins_NamedItem:
  {
    UString s = args[0]->toString(exec);
    return base->pluginByName( exec, s.qstring() );
  }
  default:
    // qDebug() << "WARNING: Unhandled token in PluginsFunc::callAsFunction : " << id;
    return jsUndefined();
  }
}

/*******************************************************************/

const ClassInfo MimeTypes::info = { "MimeTypeArray", 0, &MimeTypesTable, 0 };
/*
@begin MimeTypesTable 3
  length	MimeTypes_Length  	DontDelete|ReadOnly
  item  	MimeTypes_Item    	DontDelete|Function 1
  namedItem  	MimeTypes_NamedItem   	DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(MimeTypesFunc)

JSValue *MimeTypes::indexGetter(ExecState* exec, JSObject*, const Identifier& /*propertyName*/, const PropertySlot& slot)
{
  return new MimeType(exec, mimes->at(slot.index()));
}

JSValue *MimeTypes::nameGetter(ExecState* exec, JSObject*, const Identifier& propertyName, const PropertySlot& /*slot*/)
{
  return mimeTypeByName(exec, propertyName.qstring());
}

bool MimeTypes::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "MimeTypes::getOwnPropertySlot " << propertyName.qstring();
#endif
  if (getStaticOwnPropertySlot<MimeTypesFunc, MimeTypes>(&MimeTypesTable, this, propertyName, slot))
      return true;

  if (pluginsEnabled()) {
    // mimeTypes[#]
    bool ok;
    unsigned int i = propertyName.toArrayIndex(&ok);
    if (ok && i < static_cast<unsigned>(mimes->count())) {
      slot.setCustomIndex(this, i, indexGetter);
      return true;
    }

    // mimeTypes[name]
    QList<const MimeClassInfo*>::const_iterator it, end = mimes->constEnd();
    for (it = mimes->constBegin(); it != end; ++it) {
      if ((*it)->type == propertyName.qstring()) {
        slot.setCustom(this, nameGetter);
        return true;
      }
    }
  }

  return PluginBase::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *MimeTypes::mimeTypeByName( ExecState* exec, const QString& name )
{
  //qDebug() << "MimeTypes[" << name << "]";
  QList<const MimeClassInfo*>::const_iterator it, end = mimes->constEnd();
  for (it = mimes->constBegin(); it != end; ++it) {
    if ((*it)->type == name)
      return new MimeType(exec, (*it));
  }
  return jsUndefined();
}

JSValue *MimeTypes::getValueProperty(ExecState* /*exec*/, int token) const
{
  assert(token == MimeTypes_Length);
  Q_UNUSED(token);
  if (pluginsEnabled())
    return jsNumber(mimes->count());
  else
    return jsNumber(0);
}

JSValue *MimeTypesFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::MimeTypes, thisObj );
  KJS::MimeTypes* base = static_cast<KJS::MimeTypes *>(thisObj);

  if (!base->pluginsEnabled()) return jsUndefined();

  switch( id ) {
  case MimeTypes_Item:
  {
    bool ok;
    unsigned int i = args[0]->toString(exec).toArrayIndex(&ok);
    if (ok && i < static_cast<unsigned>(base->mimes->count()))
      return new MimeType( exec, base->mimes->at(i) );
    return jsUndefined();
  }
  case MimeTypes_NamedItem:
  {
    UString s = args[0]->toString(exec);
    return base->mimeTypeByName( exec, s.qstring() );
  }
  default:
    // qDebug() << "WARNING: Unhandled token in MimeTypesFunc::callAsFunction : " << id;
    return jsUndefined();
  }
}

/************************************************************************/
const ClassInfo Plugin::info = { "Plugin", 0, &PluginTable, 0 };
/*
@begin PluginTable 7
  name  	Plugin_Name	  	DontDelete|ReadOnly
  filename  	Plugin_FileName    	DontDelete|ReadOnly
  description  	Plugin_Description    	DontDelete|ReadOnly
  length  	Plugin_Length    	DontDelete|ReadOnly
  item  	Plugin_Item	   	DontDelete|Function 1
  namedItem  	Plugin_NamedItem   	DontDelete|Function 1
@end
*/
KJS_IMPLEMENT_PROTOFUNC(PluginFunc)

JSValue *Plugin::indexGetter(ExecState *exec, JSObject*, const Identifier& /*propertyName*/, const PropertySlot& slot)
{
    Plugin *thisObj = static_cast<Plugin *>(slot.slotBase());
    return new MimeType(exec, thisObj->m_info->mimes.at(slot.index()));
}

JSValue *Plugin::nameGetter(ExecState *exec, JSObject*, const Identifier& propertyName, const PropertySlot& slot)
{
  Plugin *thisObj = static_cast<Plugin *>(slot.slotBase());
  return thisObj->mimeByName(exec, propertyName.qstring());
}

bool Plugin::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "Plugin::getOwnPropertySlot " << propertyName.qstring();
#endif
  if (getStaticOwnPropertySlot<PluginFunc, Plugin>(&PluginTable, this, propertyName, slot))
      return true;

  // plugin[#]
  bool ok;
  unsigned int i = propertyName.toArrayIndex(&ok);
  if (ok && i < static_cast<unsigned>(m_info->mimes.count())) {
    slot.setCustomIndex(this, i, indexGetter);
    return true;
  }

  // plugin["name"]
  QList<const MimeClassInfo*>::const_iterator it, end = mimes->constEnd();
  for (it = mimes->constBegin(); it != end; ++it) {
    if ((*it)->type == propertyName.qstring()) {
      slot.setCustom(this, nameGetter);
      return true;
    }
  }

  return PluginBase::getOwnPropertySlot(exec, propertyName, slot);
}

JSValue *Plugin::mimeByName(ExecState* exec, const QString& name) const
{
  const QList<const MimeClassInfo*>& mimes = m_info->mimes;
  QList<const MimeClassInfo*>::const_iterator it, end = mimes.end();
  for (it = mimes.begin(); it != end; ++it) {
    if ((*it)->type == name)
      return new MimeType(exec, (*it));
  }
  return jsUndefined();
}

JSValue *Plugin::getValueProperty(ExecState* /*exec*/, int token) const
{
  switch( token ) {
  case Plugin_Name:
    return jsString( UString(m_info->name) );
  case Plugin_FileName:
    return jsString( UString(m_info->file) );
  case Plugin_Description:
    return jsString( UString(m_info->desc) );
  case Plugin_Length:
    return jsNumber( m_info->mimes.count() );
  default:
    // qDebug() << "WARNING: Unhandled token in Plugin::getValueProperty : " << token;
    return jsUndefined();
  }
}

JSValue *PluginFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &args)
{
  KJS_CHECK_THIS( KJS::Plugin, thisObj );
  KJS::Plugin* plugin = static_cast<KJS::Plugin *>(thisObj);
  switch( id ) {
  case Plugin_Item:
  {
    bool ok;
    unsigned int i = args[0]->toString(exec).toArrayIndex(&ok);
    if (ok && i< static_cast<unsigned>(plugin->pluginInfo()->mimes.count()))
      return new MimeType( exec, plugin->pluginInfo()->mimes.at(i) );
    return jsUndefined();
  }
  case Plugin_NamedItem:
  {
    UString s = args[0]->toString(exec);
    return plugin->mimeByName( exec, s.qstring() );
  }
  default:
    // qDebug() << "WARNING: Unhandled token in PluginFunc::callAsFunction : " << id;
    return jsUndefined();
  }
}

/*****************************************************************************/

const ClassInfo MimeType::info = { "MimeType", 0, &MimeTypeTable, 0 };
/*
@begin MimeTypeTable 4
  description  	MimeType_Description    	DontDelete|ReadOnly
  enabledPlugin MimeType_EnabledPlugin    	DontDelete|ReadOnly
  suffixes	MimeType_Suffixes	    	DontDelete|ReadOnly
  type  	MimeType_Type			DontDelete|ReadOnly
@end
*/

bool MimeType::getOwnPropertySlot(ExecState *exec, const Identifier& propertyName, PropertySlot& slot)
{
#ifdef KJS_VERBOSE
  qDebug() << "MimeType::get " << propertyName.qstring();
#endif
  return getStaticValueSlot<MimeType, JSObject>(exec, &MimeTypeTable, this, propertyName, slot);
}

JSValue *MimeType::getValueProperty(ExecState* exec, int token) const
{
  switch( token ) {
  case MimeType_Type:
    return jsString( UString(m_info->type) );
  case MimeType_Suffixes:
    return jsString( UString(m_info->suffixes) );
  case MimeType_Description:
    return jsString( UString(m_info->desc) );
  case MimeType_EnabledPlugin:
    return new Plugin(exec, m_info->plugin);
  default:
    // qDebug() << "WARNING: Unhandled token in MimeType::getValueProperty : " << token;
    return jsUndefined();
  }
}


JSValue *NavigatorFunc::callAsFunction(ExecState *exec, JSObject *thisObj, const List &)
{
  KJS_CHECK_THIS( KJS::Navigator, thisObj );
  Navigator *nav = static_cast<Navigator *>(thisObj);
  // javaEnabled()
  return jsBoolean(nav->part()->javaEnabled());
}
