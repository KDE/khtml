package org.kde.kjas.server;

import java.io.*;
import java.security.*;
import java.net.*;

/**
 *  KJAS server recognizes these variablers:
 *    kjas.debug - makes server actions verbose
 *    kjas.showConsole - shows Java Console window
 *    kjas.log - save a transcript of the debug output to /tmp/kjas.log
 */

public class Main
{
    //We need to save a reference to the original stdout
    //for sending messages back
    static final KJASProtocolHandler         protocol;
    static       Console                     console = null;
    private static final boolean             show_console;
    public  static final boolean             Debug;
    public  static final boolean             log;
    static final boolean                     cacheImages;
    static float                             java_version = (float) 0.0;
    static String                            proxyHost = null;
    static int                               proxyPort = 0;
    private static boolean                   good_jdk = true;

    /**************************************************************************
     * Initialization
     **************************************************************************/
    static
    {
        Debug = System.getProperty( "kjas.debug" ) != null;

        show_console = System.getProperty( "kjas.showConsole" ) != null;

        if( System.getProperty( "kjas.useKio" ) != null )
            URL.setURLStreamHandlerFactory( new KJASURLStreamHandlerFactory() );

        log = System.getProperty( "kjas.log" ) != null;
        
        cacheImages = System.getProperty( "kjas.noImageCache" ) != null;

        // determine system proxy
        proxyHost = System.getProperty( "http.proxyHost" );
        String proxyPortString = System.getProperty( "http.proxyPort" );
        try {
            proxyPort = Integer.parseInt(proxyPortString);
        } catch (Exception e) {
        }
        //Main.debug( "JVM version = " + System.getProperty( "java.version" ) );
        String version = System.getProperty("java.version").substring( 0, 3 );
        // Hack for SGI Java2 runtime
        if (version == "Jav") {     // Skip over JavaVM-  (the first 7 chars)
            version = System.getProperty("java.version").substring(7,3);
        }
        //Main.debug( "JVM numerical version = " + version );
        try {
            java_version = Float.parseFloat( version );
            if( java_version < 1.2 )
                good_jdk = false;
        } catch( NumberFormatException e ) {
            good_jdk = false;
        }
        PrintStream protocol_stdout = System.out;
        console = new KJASSwingConsole();
        protocol = new KJASProtocolHandler( System.in, protocol_stdout );
    }

    /**************************************************************************
     * Public Utility functions available to the KJAS framework
     **************************************************************************/
    public static void debug( String msg )
    {
        if( Debug )
        {
            System.out.println( "KJAS: " + msg );
        }
    }
    public static void info (String msg ) {
        System.err.println( "KJAS: " + msg );
    }

    public static void kjas_err( String msg, Exception e )
    {
        System.err.println( msg );
        System.err.println( "Backtrace: " );
        e.printStackTrace();
    }

    public static void kjas_err( String msg, Throwable t )
    {
        System.err.println( msg );
        t.printStackTrace();
    }
    private Main() {
    }

    /**************************************************************************
     * Main- create the command loop
     **************************************************************************/
    public static void main( String[] args )
    {
        if( !good_jdk )
        {
            console.setVisible( true );
            System.err.println( "ERROR: This version of Java is not supported for security reasons." );
            System.err.println( "\t\tPlease use Java version 1.2 or higher." );
            return;
        }

        if( show_console )
            console.setVisible( true );

        // set up https
        boolean hasHTTPS = true;

        try {
            // https needs a secure socket provider
            Provider[] sslProviders = Security.getProviders("SSLContext.SSL");
            
            if (sslProviders == null || sslProviders.length == 0) {
                // as a fallback, try to dynamically install Sun's jsse
                Class provider = Class.forName("com.sun.net.ssl.internal.ssl.Provider");
                
                if (provider != null) {
                    Main.debug("adding Security Provider");
                    Provider p = (Provider) provider.newInstance();
                    Security.addProvider(p);
                } else {
                    // Try jessie (http://www.nongnu.org/jessie/) as a fallback
                    // available in the Free World
                    provider = Class.forName("org.metastatic.jessie.provider.Jessie");
                    if (provider != null) {
                        Main.debug("adding Jessie as Security Provider");
                        Provider p = (Provider) provider.newInstance();
                        Security.addProvider(p);
                    } else {
                        Main.debug("could not get class: com.sun.net.ssl.internal.ssl.Provider");
                        hasHTTPS = false;
                    }
                }
            }

            if (hasHTTPS) {
                // allow user to provide own protocol handler
                // -Djava.protocol.handler.pkgs = user.package.name
                // getting and setting of properties might generate SecurityExceptions
                // so this needs to be in a try block
                String handlerPkgs = System.getProperty("java.protocol.handler.pkgs");

                if (handlerPkgs == null) {
                    // set default packages for Sun and IBM
                    handlerPkgs = "com.sun.net.ssl.internal.www.protocol" + 
                                  "|com.ibm.net.ssl.www.protocol";
                } else {
                    // add default packages for Sun and IBM as fallback
                    handlerPkgs += "|com.sun.net.ssl.internal.www.protocol" + 
                                   "|com.ibm.net.ssl.www.protocol";
                }

                System.setProperty("java.protocol.handler.pkgs", handlerPkgs);
            }
        } catch (Exception e) {
            hasHTTPS = false;
        }

        if (hasHTTPS == false) {
            System.out.println("Unable to load JSSE SSL stream handler, https support not available");
            System.out.println("For more information see http://java.sun.com/products/jsse/");
        }

        //start the command parsing
        protocol.commandLoop();
    }

}
