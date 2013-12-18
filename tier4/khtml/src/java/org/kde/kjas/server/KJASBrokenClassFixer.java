package org.kde.kjas.server;
import java.lang.reflect.*;
import java.net.URLClassLoader;
import java.net.URL;
/**
* wrapper for the javaplugin.jar Broken11ClassFixer.
* Uses the reflection api to wrap the class <i>sun.plugin.security.Broken11ClassFixer</i>
* from the javaplugin.jar archive which can be found in the jre/lib directory.
*/
public class KJASBrokenClassFixer {
    private static Class fixerClass = null;
    private static Method _process;
    private static Method _getProcessedData;
    private static Method _getProcessedDataOffset;
    private static Method _getProcessedDataLength;
    private static boolean initialized = false;
    private static final String fixerClassName = "sun.plugin.security.Broken11ClassFixer";
    private Object fixer = null;
    private byte [] bytes;
    private int offset;
    private int length;
    
    /**
     * creates a new KJASBrokenClassFixer.
     * If it is the first one to be created, it tries to load the class
     * <i>sun.plugin.security.Broken11ClassFixer</i> from the jar file
     * <i>lib/javaplugin.jar</i> in the java jre directory.
     */
    public KJASBrokenClassFixer() {
        init();
        if (fixerClass != null) { 
            try {
                fixer = fixerClass.newInstance();
            } catch (Throwable e) {
                e.printStackTrace();
            }
        } 
    }

    /**
    * loads the class <i>sun.plugin.security.Broken11ClassFixer</i>,
    * initializes the methods, ...
    */
    private synchronized void init() {
        if (initialized) {
            return;
        }
        try {
            URL [] urls = { new URL(
                "file", "", 0,
                System.getProperty("java.home")
                + System.getProperty("file.separator")
                + "lib"
                + System.getProperty("file.separator")
                + "javaplugin.jar"), new URL(
                "file", "", 0,
                System.getProperty("java.home")
                + System.getProperty("file.separator")
                + "lib"
                + System.getProperty("file.separator")
                + "plugin.jar") 
            };
            URLClassLoader loader = new URLClassLoader(urls);
            fixerClass = Class.forName(fixerClassName, true, loader);
            Main.debug("Loaded " + fixerClass);
            final Class [] parameterTypes = {
                (new byte[1]).getClass(), 
                Integer.TYPE, 
                Integer.TYPE
            };
            final Class [] noParameter = new Class[0]; 
            _process = fixerClass.getMethod("process", parameterTypes);  
            _getProcessedData = fixerClass.getMethod("getProcessedData", noParameter);
            _getProcessedDataOffset = fixerClass.getMethod("getProcessedDataOffset", noParameter);
            _getProcessedDataLength = fixerClass.getMethod("getProcessedDataLength", noParameter);
        } catch (Throwable e) {
            e.printStackTrace();
        } finally {
            initialized = true;
        }
    }
    /**
    * scan the broken bytes and create new ones.
    * If the wrapped class could not be loaded or
    * no instance of Broken11ClassFixer could be instantiated,
    * this is a noop and later calls to getProcessedData() etc.
    * will return the original data passed as arguments in this
    * call.
    */ 
    public boolean process(byte [] b, int off, int len) {        
        if (fixer != null) {
            try { 
                Object [] args = new Object[3];
                args[0] = b;
                args[1] = new Integer(off);
                args[2] = new Integer(len);
                Object [] none = new Object[0]; 

                _process.invoke(fixer, args);
                this.bytes = (byte[])_getProcessedData.invoke(fixer, none);
                this.offset = ((Integer)_getProcessedDataOffset.invoke(fixer, none)).intValue();
                this.length = ((Integer)_getProcessedDataLength.invoke(fixer, none)).intValue();
                return true;
            }  catch (Throwable e) {
            }
        }
        this.bytes = b;
        this.offset = off;
        this.length = len;
        return false;
    }
    
    /**
    * get the offset in the processed byte array
    */
    public int getProcessedDataOffset() {
        return offset;
    }
    /**
    * get the length of the processed data
    */ 
    public int getProcessedDataLength() {
        return length;
    }
    /**
    * get the processed (fixed) data
    */
    public byte [] getProcessedData() {
        return bytes;
    }
    
}
