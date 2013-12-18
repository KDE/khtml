package org.kde.kjas.server;

import java.net.*;
import java.io.*;
import java.util.*;
import java.util.zip.*;
import java.util.jar.*;
import java.security.*;
/**
 * ClassLoader used to download and instantiate Applets.
 * <P>
 * NOTE: The class loader extends Java 1.2 specific class.
 */
public final class KJASAppletClassLoader
    extends URLClassLoader
{
    private static Hashtable loaders = new Hashtable();

    public static synchronized void removeLoaders()
    {
        loaders.clear();
    }

    public static synchronized KJASAppletClassLoader getLoader( String docBase, String codeBase, String archives )
    {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkCreateClassLoader();
        }
        URL docBaseURL;
        KJASAppletClassLoader loader = null;
        try
        {
            docBaseURL = new URL( docBase );
        
            URL codeBaseURL = getCodeBaseURL( docBaseURL, codeBase );
            String key = codeBaseURL.toString();
            if (archives != null)
                key += archives;

            Main.debug( "CL: getLoader: key = " + key );

            loader = (KJASAppletClassLoader) loaders.get( key );
            if( loader == null )
            {
                URL [] urlList = {};
                loader = new KJASAppletClassLoader( urlList, docBaseURL, codeBaseURL);
                loaders.put( key, loader );
            }
            else
            {
                Main.debug( "CL: reusing classloader" );
            }
        } catch( MalformedURLException e ) { Main.kjas_err( "bad DocBase URL", e ); }
        return loader;
    }

    public static URL getCodeBaseURL( URL docBaseURL, String codeBase )
    {
        URL codeBaseURL = null;
        try
        {
            //first determine what the real codeBase is: 3 cases
            //#1. codeBase is absolute URL- use that
            //#2. codeBase is relative to docBase, create url from those
            //#3. last resort, use docBase as the codeBase
            if(codeBase != null)
            {
                //we need to do this since codeBase should be a directory
                if( !codeBase.endsWith("/") )
                    codeBase = codeBase + "/";

                try
                {
                    codeBaseURL = new URL( codeBase );
                } catch( MalformedURLException mue )
                {
                    try
                    {
                        codeBaseURL = new URL( docBaseURL, codeBase );
                    } catch( MalformedURLException mue2 ) {}
                }
            }

            if(codeBaseURL == null)
            {
                //fall back to docBase but fix it up...
                String file = docBaseURL.getFile();
                if( file == null || (file.length() == 0)  )
                    codeBaseURL = docBaseURL;
                else if( file.endsWith( "/" ) )
                    codeBaseURL = docBaseURL;
                else
                {
                    //delete up to the ending '/'
                    String urlString = docBaseURL.toString();
                    int dot_index = urlString.lastIndexOf( '/' );
                    String newfile = urlString.substring( 0, dot_index+1 );
                    codeBaseURL = new URL( newfile );
                }
            }
        }catch( Exception e ) { Main.kjas_err( "CL: exception ", e ); }
        return codeBaseURL;    
    }

    public static KJASAppletClassLoader getLoader( String key )
    {
        SecurityManager security = System.getSecurityManager();
        if (security != null) {
            security.checkCreateClassLoader();
        }
        if( loaders.containsKey( key ) )
            return (KJASAppletClassLoader) loaders.get( key );
        
        return null;
    }

    /*********************************************************************************
     ****************** KJASAppletClassLoader Implementation *************************
     **********************************************************************************/
    private URL docBaseURL;
    private URL codeBaseURL;
    private Vector archives;
    private String dbgID;
    private static int globalId = 0;
    private int myId = 0;
    private Vector statusListeners = new Vector();
    private AccessControlContext acc;
    // a mapping JS referenced Java objects
    private Hashtable jsReferencedObjects = new Hashtable();
    final static RuntimePermission kjas_access = new RuntimePermission("accessClassInPackage.org.kde.kjas.server");
    
    public KJASAppletClassLoader( URL[] urlList, URL _docBaseURL, URL _codeBaseURL)
    {
        super(urlList);
        acc = AccessController.getContext();
        synchronized(KJASAppletClassLoader.class) {
            myId = ++globalId;
        }
        docBaseURL   = _docBaseURL;
        codeBaseURL  = _codeBaseURL;
        archives     = new Vector();
        
        dbgID = "CL-" + myId + "(" + codeBaseURL.toString() + "): ";
    }
    
    protected void addURL(URL url) {
        Main.debug(this + " add URL: " + url);
        super.addURL(url);
    }
    
    public void addStatusListener(StatusListener lsnr) {
        statusListeners.add(lsnr);
    }
    public void removeStatusListener(StatusListener lsnr) {
        statusListeners.remove(lsnr);
    }
    public void showStatus(String msg) {
        Enumeration en = statusListeners.elements();
        while (en.hasMoreElements()) {
            StatusListener lsnr = (StatusListener)en.nextElement();
            lsnr.showStatus(msg);
        }
    }
    
    public void paramsDone() {
        // simply builds up the search path
        // put the archives first because they are 
        // cached.
        for( int i = 0; i < archives.size(); ++i ) {
            String jar = (String)archives.elementAt( i );
            try {
                URL jarURL = new URL(codeBaseURL, jar);
                addURL(jarURL);
                Main.debug("added archive URL \"" + jarURL + "\" to KJASAppletClassLoader");
            } catch (MalformedURLException e) {
                Main.kjas_err("Could not construct URL for jar file: " + codeBaseURL + " + " + jar, e);
            }
        }
        // finally add code base url and docbase url
        addURL(codeBaseURL);

        // the docBaseURL has to be fixed.
        // strip file part from end otherwise this
        // will be interpreted as an archive
        // (should this perhaps be done generally ??)       
        String dbs = docBaseURL.toString();
        int idx = dbs.lastIndexOf("/");
        if (idx > 0) {
            dbs = dbs.substring(0, idx+1);
        }
        URL docDirURL = null; 
        try {
            docDirURL = new URL(dbs);
        } catch (MalformedURLException e) {
            Main.debug("Could not make a new URL from docBaseURL=" + docBaseURL);
        }
        if (docDirURL != null && !codeBaseURL.equals(docDirURL)) {
            addURL(docDirURL);
        }
    }

    void addArchiveName( String jarname )
    {
        if( !archives.contains( jarname ) )
        {
            archives.add( jarname );
        }
    }
    

    public URL getDocBase()
    {
        return docBaseURL;
    }

    public URL getCodeBase()
    {
        return codeBaseURL;
    }

    Hashtable getJSReferencedObjects() {
        return jsReferencedObjects;
    }
    /***************************************************************************
     **** Class Loading Methods
     **************************************************************************/
    public synchronized Class findClass( String name ) throws ClassNotFoundException
    {
        Class rval = null;
        //check the loaded classes 
        rval = findLoadedClass( name );
        if( rval == null ) {
            try {
                rval =  super.findClass(name);
            } catch (ClassFormatError cfe) {
                Main.debug(name + ": Catched " + cfe + ". Trying to repair...");
                rval = loadFixedClass( name );
            } catch (Exception ex) {
                Main.debug("findClass " + name + " " + ex.getMessage());
            }
        }
        if (rval == null) {
            throw new ClassNotFoundException("Class: " + name);
        }
        return rval;
    }
    public Class loadClass(String name) throws ClassNotFoundException {
        if (name.startsWith("org.kde.kjas.server")) {
            SecurityManager sec = System.getSecurityManager();
            if (sec != null)
                sec.checkPermission(kjas_access);
       }
        return super.loadClass(name);
    }
    private Hashtable loadedClasses = new Hashtable();

    private synchronized final Class loadFixedClass(String name) throws ClassNotFoundException {
        final String fileName = name.replace('.', '/') + ".class";
        try {
            // try to get the class as resource
            final URL u = getResource(fileName);
            Main.debug(dbgID + name + ": got URL: " + u);
            if (u == null) {
                throw new ClassNotFoundException(fileName + ": invalid resource URL.");
            }
            java.security.cert.Certificate[] certs = {}; // FIXME
            CodeSource cs = new CodeSource(u, certs);
            InputStream instream = (InputStream)AccessController.doPrivileged(
              new PrivilegedAction() {
                public Object run() {
                    try {
                        return u.openStream();
                    } catch (IOException ioe) {
                        ioe.printStackTrace();
                        return null;
                    }
                }
              }, acc
            );
            if (instream == null) {
                throw new ClassNotFoundException(name + ": could not be loaded.");
            }
            ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
            int cnt;
            int total = 0;
            int bufSize = 1024;
            byte [] buffer = new byte[bufSize];
            while ((cnt = instream.read(buffer, 0, bufSize)) > 0) {
                 total += cnt;
                 byteStream.write(buffer, 0, cnt);
            }
            Main.debug(dbgID + name + ": " + total + " bytes");
            
            Class cl = fixAndDefineClass(name, byteStream.toByteArray(), 0, total, cs);
            if (cl != null) {
                loadedClasses.put(name, cl);
            }
            return cl;
            
        } catch (Throwable e) {
            e.printStackTrace();
            throw new ClassNotFoundException("triggered by " + e);
        }
    }
    
    public URL findResource( String name)
    {
        Main.debug( dbgID + "findResource, name = " + name );
        String displayName = name;
        try {
            URL u = new URL(name);
            String filename = u.getFile();
            if (filename != null && filename.length() > 0) {
                displayName = filename;
            }
        } catch (Throwable e) {
        }
        showStatus("Loading: " + displayName);
        URL url =  super.findResource( name );
        Main.debug("findResource for " + name + " returns " + url);
        return url;
    }
   
    protected PermissionCollection getPermissions(CodeSource cs) {
        Main.debug(dbgID + " getPermissions(" + cs + ")");
        PermissionCollection permissions = super.getPermissions(cs);
        Enumeration perms_enum = permissions.elements();
        while (perms_enum.hasMoreElements()) {
            Main.debug(this + " Permission: " + perms_enum.nextElement());
        }
        return permissions;
    }
    
   
    /**
    * define the class <b>name</b>. If  <b>name</b> is broken, try to fix it.
    */
    private final Class fixAndDefineClass(
            String name, 
            byte[] b, 
            int off, 
            int len,
            CodeSource cs) throws ClassFormatError
    {
        KJASBrokenClassFixer fixer = new KJASBrokenClassFixer();
        if (fixer.process(b, off, len)) {
            Main.debug(name + " fixed");
        } else {
            Main.info(name + " could not be fixed");
        }
        return defineClass(name, 
                fixer.getProcessedData(), 
                fixer.getProcessedDataOffset(), 
                fixer.getProcessedDataLength(), 
                cs);
    }
    
    
}
