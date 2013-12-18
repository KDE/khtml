package org.kde.kjas.server;

import java.security.*;
import java.security.cert.*;
import java.net.*;
import java.util.*;


public class KJASSecurityManager extends SecurityManager
{
    static Hashtable confirmRequests = new Hashtable();
    static int confirmId = 0;
    Hashtable grantedPermissions = new Hashtable();
    HashSet grantAllPermissions = new HashSet();
    HashSet rejectAllPermissions = new HashSet();

    private static final char [] base64table = {
      'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
      'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
      'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
    };
    static String encode64( byte [] data)
    {
        StringBuffer buf = new StringBuffer( 4*((data.length + 2)/3) );
        int i = 0, b1, b2, b3;
        while (i < data.length - 2) {
            b1 = data[i++];
            b2 = data[i++];
            b3 = data[i++];
            buf.append( base64table[(b1 >>> 2) & 0x3F] );
            buf.append( base64table[((b1 << 4) & 0x30) | ((b2 >>> 4) & 0xF)] );
            buf.append( base64table[((b2 << 2) & 0x3C) | ((b3 >>> 6) & 0x03)] );
            buf.append( base64table[b3 & 0x3F] );
        }
        if ( i < data.length ) {
            b1 = data[i++];
            buf.append( base64table[(b1 >>> 2) & 0x3F] );
            if ( i < data.length ) {
                b2 = data[i++];
                buf.append( base64table[((b1 << 4) & 0x30) | ((b2 >>> 4) & 0xF)] );
                buf.append( base64table[(b2 << 2) & 0x3C] );
            } else {
                buf.append( base64table[(b1 << 4) & 0x30] );
                buf.append( "=" );
            }
            buf.append( '=' );
        }
        return buf.toString();
    }
    public KJASSecurityManager()
    {
    }
    /**
     * checks for an applets permission to access certain resources
     * currently, only a check for SocketPermission is done, that the
     * applet cannot connect to any other but the host, where it comes from.
     * Anything else seems to be handled automagically
     */
    public void checkPermission(Permission perm) throws SecurityException, NullPointerException {
        // ClassLoader cl = Thread.currentThread().getContextClassLoader();
        try {
            super.checkPermission(perm);
        } catch (SecurityException se) {
            // Don't annoy users with these
            if (/*perm instanceof java.lang.RuntimePermission || */
                    perm instanceof java.awt.AWTPermission)
                throw se;

            // Collect certificates
            HashSet signers = new HashSet();
            Class [] cls = getClassContext();
            for (int i = 1; i < cls.length; i++) {
                Object[] objs = cls[i].getSigners();
                if (objs != null && objs.length > 0) {
                    for (int j = 0; j < objs.length; j++)
                        if (objs[j] instanceof X509Certificate)
                            signers.add( ((X509Certificate) objs[j]) );
                }
            }
            Main.debug("Certificates " + signers.size() + " for " + perm);

            // Check granted/denied permission
            if ( grantAllPermissions.contains(signers) )
                return;
            if ( rejectAllPermissions.contains(signers) )
                throw se;
            Permissions permissions = (Permissions) grantedPermissions.get(signers);
            if (permissions != null && permissions.implies(perm))
                return;

            // Ok, ask user what to do
            String [] certs = new String[signers.size()];
            int certsnr = 0;
            for (Iterator i = signers.iterator(); i.hasNext(); ) {
                try {
                    certs[certsnr] = encode64( ((X509Certificate) i.next()).getEncoded() );
                    certsnr++;
                } catch (CertificateEncodingException cee) {}
            }
            if (certsnr == 0)
                throw se;
            String id = "" + confirmId++;
            confirmRequests.put(id, Thread.currentThread());
            Main.protocol.sendSecurityConfirm(certs, certsnr, perm.toString(), id);
            boolean granted = false;
            try {
                Thread.currentThread().sleep(300000);
            } catch (InterruptedException ie) {
                if (((String) confirmRequests.get(id)).equals("yes")) {
                    granted = true;
                    permissions = (Permissions) grantedPermissions.get(signers);
                    if (permissions == null) {
                        permissions = new Permissions();
                        grantedPermissions.put(signers, permissions);
                    }
                    permissions.add(perm);
                } else if (((String) confirmRequests.get(id)).equals("grant")) {
                    grantAllPermissions.add( signers );
                    granted = true;
                } else if (((String) confirmRequests.get(id)).equals("reject")) {
                    rejectAllPermissions.add( signers );
                } // else "no", "nossl" or "invalid"
            } finally {
                confirmRequests.remove(id);
            }
            if (!granted) {
                Main.debug("Permission denied" + perm);
                throw se;
            }
        }
    }

    // keytool -genkey -keystore mystore -alias myalias
    // keytool -export -keystore mystore -alias myalias -file mycert
    // keytool -printcert -file mycert
    // keytool -import -keystore myotherstore -alias myalias -file mycert
    // jarsigner -keystore mystore myjar.jar myalias
    // jarsigner -verify -keystore myotherstore myjar.jar
    //
    // policy file (use policytool and check java.security):
    // keystore "file:myotherstore", "JKS"
    // grant signedBy "myalias"
    // {
    //     permission java.io.FilePermission "<<ALL FILES>>", "read"
    // }
    // 
    // java code:
    // KeyStore store = KeyStore.getInstance("JKS", "SUN");
    public void disabled___checkPermission(Permission perm) throws SecurityException, NullPointerException
    {
        // does not seem to work as expected, Problems with proxy - and it seems that the default
        // implementation already does all that well, what I wanted to do here.
        // It is likely that this method will hence disappear soon again.
        Object context = getSecurityContext();
        Thread thread = Thread.currentThread();
        if (perm instanceof SocketPermission) {
            // check if this is a connection back to the originating host
            // if not, fall through and call super.checkPermission
            // this gives normally access denied
            Main.debug("*** checkPermission " + perm + " in context=" + context + " Thread=" + thread);
            // use the context class loader to determine if this is one
            // of our applets
            ClassLoader contextClassLoader = thread.getContextClassLoader();
            Main.debug("*   ClassLoader=" + contextClassLoader);
            try {
                // try to cast ...
                KJASAppletClassLoader loader = (KJASAppletClassLoader)contextClassLoader;
                // ok. cast succeeded. Now get the codebase of the loader
                // because it contains the host name
                URL codebase = loader.getCodeBase();
                URL docbase = loader.getDocBase();
                Main.debug("*   Class Loader docbase=" + docbase + " codebase=" + codebase);
                String hostname = perm.getName();
                // extract the hostname from the permission name
                // which is something like "some.host.domain:XX"
                // with XX as the port number
                int colonIdx = hostname.indexOf(':');
                if (colonIdx > 0) {
                    // strip of the port
                    hostname = hostname.substring(0, colonIdx);
                }
                // Main.info("Checking " + hostname + "<->" + codebase.getHost());
                
                if (hostsAreEqual(hostname, codebase.getHost())) {
                    // ok, host matches
                    String actions = perm.getActions();
                    // just check if listen is specified which we do not want
                    // to allow
                    if (actions != null && actions.indexOf("listen") >= 0) {
                        Main.debug("*   Listen is not allowed.");
                    } else {
                        // ok, just return and throw _no_ exception
                        Main.debug("*   Hostname equals. Permission granted.");
                        return;
                    }
                } else {
                    Main.info("Host mismatch: " + perm + " != " + codebase.getHost());
                }
            } catch (ClassCastException e) {
                Main.debug("*   ClassLoader is not a KJASAppletClassLoader");
            }
            Main.debug("*   Fall through to super.checkPermission()");
        }
        super.checkPermission(perm);
    }
    
    private static final boolean hostsAreEqual(String host1, String host2) {
        if (host1 == null || host2 == null) {
            return false;
        }
        if (host1.length() == 0 || host2.length() == 0) {
            return false;
        }
        if (host1.equalsIgnoreCase(host2)) {
            return true;
        }
       
        if ( Main.proxyHost != null && Main.proxyPort != 0) {
            // if we use a proxy, we certainly cannot use DNS
            return false;
        }

        InetAddress inet1=null, inet2=null;
        try {
            inet1 = InetAddress.getByName(host1);
        } catch (UnknownHostException e) {
            Main.kjas_err("Unknown host:" + host1, e);
            return false;
        }
        try {
            inet2 = InetAddress.getByName(host2);
        } catch (UnknownHostException e) {
            Main.kjas_err("Unknown host:" + host2, e);
            return false;
        }
        if (inet1.equals(inet2)) {
            return true;
        }       
        return false;
    }
}
