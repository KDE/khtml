package org.kde.kjas.server;

import java.applet.*;
import java.util.*;
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import java.io.*;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import org.kde.javascript.JSObject;

final class KJASAuthenticator extends Authenticator {
    private Hashtable authentication;

    KJASAuthenticator() {
        authentication = new Hashtable();
        setDefault(this);
    }
    final void addURL(URL url, String user, String password, String authname) {
        String key = new String(url.getProtocol() + ":" + url.getHost() + ":" +
                url.getPort() + "_" + authname);
        String [] auths = { user, password };
        authentication.put(key, auths);
    }
    final protected PasswordAuthentication getPasswordAuthentication() {
        URL url;
        String key = new String(getRequestingProtocol() + ":" + getRequestingHost() + ":" + getRequestingPort() + "_" + getRequestingPrompt());
        String [] auths = (String []) authentication.get(key);
        if (auths != null) {
            char [] pw = new char[auths[1].length()];
            auths[1].getChars(0, auths[1].length(), pw, 0);
            return new PasswordAuthentication(auths[0], pw);
        }
        return null;
    }
}

/**
 * The context in which applets live.
 */
public class KJASAppletContext implements AppletContext
{
    private Hashtable stubs;
    private Hashtable images;
    private Vector    pendingImages;
    private Hashtable streams;
    private Stack jsobjects;

    private String myID;
    private KJASAppletClassLoader loader;
    private boolean active;
    private final static KJASAuthenticator authenticator = new KJASAuthenticator();

    /**
     * Create a KJASAppletContext
     */
    public KJASAppletContext( String _contextID )
    {
        stubs  = new Hashtable();
        images = new Hashtable();
        pendingImages = new Vector();
        streams = new Hashtable();
        jsobjects = new Stack();
        myID   = _contextID;
        active = true;
    }

    public String getID()
    {
        return myID;
    }

    public String getAppletID(Applet applet) 
    {
        Enumeration e = stubs.keys();
        while ( e.hasMoreElements() )
        {
            String appletID = (String) e.nextElement();
            KJASAppletStub stub = (KJASAppletStub) stubs.get(appletID);
            if (stub.getApplet() == applet)
                return appletID;
        }
        return null;
    }

    public Applet getAppletById(String appletId) {
        return ((KJASAppletStub) stubs.get( appletId )).getApplet();
    }

    public String getAppletName(String appletID) {
        KJASAppletStub stub = (KJASAppletStub) stubs.get(appletID);
        if (stub == null)
            return null;
        return stub.getAppletName();
    }
    public void createApplet( String appletID, String name,
                              String className, String docBase,
                              String username, String password, String authname,
                              String codeBase, String archives,
                              String width, String height,
                              String windowName, Hashtable params )
    {
        //do kludges to support mess with parameter table and
        //the applet variables
        String key = new String( "ARCHIVE" );
        if (params.containsKey(key)) {
            String param_archive = (String)params.get(key);
            if (archives == null) {
                // There is no 'archive' attribute
                // but a 'archive' param. fix archive list 
                // from param value
                archives = param_archive;
            } else {
                // there is already an archive attribute.
                // just add the value of the param to the list.
                // But ignore bill$ personal archive format called
                // .cab because java doesn't understand it.
                if (!param_archive.toLowerCase().endsWith(".cab")) {
                    archives =  param_archive + "," + archives;
                }
            }
        } else if (archives != null) {
            // add param if it is not present
            params.put( key, archives);
        }
        
        if( codeBase == null )
        {
            key = new String( "CODEBASE" );
            if( params.containsKey( key ) )
                codeBase = (String) params.get( key );
        }

        if (username != null && !username.equals("")) {
            try {
                URL url = new URL(docBase);
                int port = url.getPort();
                if (port < 0)
                    port = url.getDefaultPort();
                authenticator.addURL(new URL(url.getProtocol(), url.getHost(), port, ""), username, password, authname);
            } catch (MalformedURLException muex) {
            }
        }
        try
        {
            String sorted_archives = "";
            TreeSet archive_set = new TreeSet();
            if( archives != null )
            {
                StringTokenizer parser = new StringTokenizer( archives, ",", false );
                while( parser.hasMoreTokens() )
                    archive_set.add ( parser.nextToken().trim() );
            }
            Iterator it = archive_set.iterator();
            while (it.hasNext())
                sorted_archives += (String) it.next();
            KJASAppletClassLoader loader =
                KJASAppletClassLoader.getLoader( docBase, codeBase, sorted_archives );
            it = archive_set.iterator();
            while (it.hasNext())
                loader.addArchiveName( (String) it.next() );
            loader.paramsDone();

            KJASAppletStub stub = new KJASAppletStub
            (
                this, appletID, loader.getCodeBase(),
                loader.getDocBase(), name, className,
                new Dimension( Integer.parseInt(width), Integer.parseInt(height) ),
                params, windowName, loader
            );
            stubs.put( appletID, stub );

            stub.createApplet();
        }
        catch ( Exception e )
        {
            Main.kjas_err( "Something bad happened in createApplet: " + e, e );
        }
    }

    public void initApplet( String appletID )
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if( stub == null )
        {
            Main.debug( "could not init and show applet: " + appletID );
        }
        else
        {
            stub.initApplet();
        }
    }

    public void destroyApplet( String appletID )
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );

        if( stub == null )
        {
            Main.debug( "could not destroy applet: " + appletID );
        }
        else
        {
            //Main.debug( "stopping applet: " + appletID );
            stubs.remove( appletID );

            stub.destroyApplet();
        }
    }

    public void startApplet( String appletID )
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if( stub == null )
        {
            Main.debug( "could not start applet: " + appletID );
        }
        else
        {
            stub.startApplet();
        }
    }

    public void stopApplet( String appletID )
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if( stub == null )
        {
            Main.debug( "could not stop applet: " + appletID );
        }
        else
        {
            stub.stopApplet();
        }
    }

    public void destroy()
    {
        Enumeration e = stubs.elements();
        while ( e.hasMoreElements() )
        {
            KJASAppletStub stub = (KJASAppletStub) e.nextElement();
            stub.destroyApplet();
            stub.loader.getJSReferencedObjects().clear();
        }

        stubs.clear();
        active = false;
    }

    /***************************************************************************
    **** AppletContext interface
    ***************************************************************************/
    public Applet getApplet( String appletName )
    {
        if( active )
        {
            Enumeration e = stubs.elements();
            while( e.hasMoreElements() )
            {
                KJASAppletStub stub = (KJASAppletStub) e.nextElement();

                if( stub.getAppletName().equals( appletName ) )
                    return stub.getApplet();
            }
        }

        return null;
    }

    public Enumeration getApplets()
    {
        if( active )
        {
            Vector v = new Vector();
            Enumeration e = stubs.elements();
            while( e.hasMoreElements() )
            {
                KJASAppletStub stub = (KJASAppletStub) e.nextElement();
                v.add( stub.getApplet() );
            }

            return v.elements();
        }

        return null;
    }

    public AudioClip getAudioClip( URL url )
    {
        Main.debug( "getAudioClip, url = " + url );
        //AudioClip clip = java.applet.Applet.newAudioClip(url); 
        AudioClip clip = new KJASAudioClip(url); 
        Main.debug( "got AudioClip " + clip);
        return clip;
        // return new KJASSoundPlayer( myID, url );
    }

    public void addImage( String url, byte[] data )
    {
        Main.debug( "addImage for url = " + url );
        images.put( url, data );
        if (Main.cacheImages) {
            pendingImages.remove(url);
        }
    }

    public Image getImage( URL url )
    {
        if( active && url != null )
        {
            // directly load images using JVM
            if (true) {
                // Main.info("Getting image using ClassLoader:" + url); 
                if (loader != null) {
                    url = loader.findResource(url.toString());
                    //Main.debug("Resulting URL:" + url);
                }
                Toolkit kit = Toolkit.getDefaultToolkit();
                Image img = kit.createImage(url);
                return img;
            }

            //check with the Web Server
            String str_url = url.toString();
            Main.debug( "getImage, url = " + str_url );
            if (Main.cacheImages && images.containsKey(str_url)) {
                Main.debug("Cached: url=" + str_url);
            }
            else
            {
                if (Main.cacheImages) {
                    if (!pendingImages.contains(str_url)) {
                        Main.protocol.sendGetURLDataCmd( myID, str_url );
                        pendingImages.add(str_url);
                    }
                } else {
                    Main.protocol.sendGetURLDataCmd( myID, str_url );
                }
                while( !images.containsKey( str_url ) && active )
                {
                    try { Thread.sleep( 200 ); }
                    catch( InterruptedException e ){}
                }
            }
            if( images.containsKey( str_url ) )
            {
                byte[] data = (byte[]) images.get( str_url );
                if( data.length > 0 )
                {
            Toolkit kit = Toolkit.getDefaultToolkit();
                    return kit.createImage( data );
                } else return null;
            }
        }

        return null;
    }

    public void showDocument( URL url )
    {
        //Main.debug( "showDocument, url = " + url );

        if( active && (url != null) )
        {
            Main.protocol.sendShowDocumentCmd( myID, url.toString()  );
        }
    }

    public void showDocument( URL url, String targetFrame )
    {
        //Main.debug( "showDocument, url = " + url + " targetFrame = " + targetFrame );

        if( active && (url != null) && (targetFrame != null) )
        {
                Main.protocol.sendShowDocumentCmd( myID, url.toString(), targetFrame );
        }
    }

    public void showStatus( String message )
    {
        if( active && (message != null) )
        {
            Main.protocol.sendShowStatusCmd( myID, message );
        }
    }
    public void evaluateJavaScript(String script, String appletID, JSObject jso) {
        if( active ) {
            if( jso != null ) {
                synchronized (jsobjects) {
                    jsobjects.push(jso);
                }
            }
            int [] types = { KJASAppletStub.JString };
            String [] arglist = { script };
            Main.protocol.sendJavaScriptEventCmd(myID, appletID, 0, "eval", types, arglist);
        }
    }

    public boolean getMember(String appletID, int callid, int objid, String name)
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if (stub == null || !stub.isLoaded())
            return false;
        return stub.getMember(callid, objid, name);
    }

    public boolean putMember(String appletID, int callid, int objid, String name, String value)
    {
        if (name.equals("__lc_ret")) {
            // special case; return value of JS script evaluation
            Main.debug("putValue: applet " + name + "=" + value);
            JSObject jso = null;
            synchronized (jsobjects) {
                if (!jsobjects.empty())
                    jso = (JSObject) jsobjects.pop();
            }
            if (jso == null)
                return false;
            jso.returnvalue = value;
            try {
                jso.thread.interrupt();
            } catch (SecurityException ex) {}
            Main.protocol.sendPutMember( myID, callid, true ); 
        }
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if (stub == null || !stub.isLoaded())
            return false;
        return stub.putMember(callid, objid, name, value);
    }

    public Object getJSReferencedObject(Applet applet, int objid)
    {
        return ((KJASAppletClassLoader)(applet.getClass().getClassLoader())).getJSReferencedObjects().get(new Integer(objid));
    }
    boolean callMember(String appletID, int cid, int oid, String n, java.util.List args)
    {
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if (stub == null || !stub.isLoaded())
            return false;
        return stub.callMember( cid, oid, n, args);
    }
    public void derefObject(String appletID, int objid) {
        if (objid == 0)
            return; // that's an applet
        KJASAppletStub stub = (KJASAppletStub) stubs.get( appletID );
        if (stub == null)
            return;
        Hashtable jsRefs = stub.loader.getJSReferencedObjects();
        if (jsRefs.remove(new Integer(objid)) == null)
            Main.debug("couldn't remove referenced object");
    }

    public void setStream(String key, InputStream stream) throws IOException {
        Main.debug("setStream, key = " + key);
        streams.put(key, stream);
    }
    public InputStream getStream(String key){
        Main.debug("getStream, key = " + key);
        return (InputStream) streams.get(key);
    }
    public Iterator getStreamKeys() {
        Main.debug("getStreamKeys");
        return streams.keySet().iterator();
    }

    
}
