package org.kde.kjas.server;

import java.applet.*;
import java.util.*;
import java.net.*;
import java.awt.*;
import java.awt.event.*;
import javax.swing.JFrame;
import java.security.PrivilegedAction;
import java.security.AccessController;
import java.security.AccessControlContext;
import java.security.ProtectionDomain;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

/**
 * The stub used by Applets to communicate with their environment.
 *
 */
public final class KJASAppletStub
    implements AppletStub
{
    private KJASAppletContext context;    // The containing context.
    private Hashtable         params;     // Maps parameter names to values
    private URL               codeBase;   // The URL directory where files are
    private URL               docBase;    // The document that referenced the applet
    private boolean           active;     // Is the applet active?
    private String            appletName; // The name of this applet instance
    private String            appletID;   // The id of this applet- for use in callbacks
    private Dimension         appletSize;
    private String            windowName;
    private String            className;
    private Class             appletClass;
    private JFrame            frame;

    /**
    * out of bounds applet state :-), perform an action
    */
    public static final int ACTION = -1;
    /**
    * applet state unknown
    */
    public static final int UNKNOWN = 0;
    /**
    * the applet class has been loaded 
    */
    public static final int CLASS_LOADED = 1;
    /**
    * the applet has been instanciated 
    */
    public static final int INSTANCIATED = 2;
    /**
    * the applet has been initialized 
    */
    public static final int INITIALIZED = 3;
    /**
    * the applet has been started 
    */
    public static final int STARTED = 4;
    /**
    * the applet has been stopped 
    */
    public static final int STOPPED = 5;
    /**
    * the applet has been destroyed 
    */
    public static final int DESTROYED = 6;
    /**
    * request for termination of the applet thread 
    */
    private static final int TERMINATE = 7;
    /**
    * like TERMINATE, an end-point state 
    */
    private static final int FAILED = 8;
   
    
    //private KJASAppletClassLoader loader;
    KJASAppletClassLoader loader;
    private KJASAppletPanel       panel;
    private Applet                app;
    KJASAppletStub                me;

    /**
     * Interface for so called LiveConnect actions, put-, get- and callMember
     */
    // keep this in sync with KParts::LiveConnectExtension::Type
    private final static int JError    = -1;
    private final static int JVoid     = 0;
    private final static int JBoolean  = 1;
    private final static int JFunction = 2;
    private final static int JNumber   = 3;
    private final static int JObject   = 4;
    final static int         JString   = 5;

    interface AppletAction {
        void apply();
        void fail();
    }

    private class RunThread extends Thread {
        private int request_state = CLASS_LOADED;
        private int current_state = UNKNOWN;
        private Vector actions = new Vector();
        private AccessControlContext acc = null;

        RunThread() {
            super("KJAS-AppletStub-" + appletID + "-" + appletName);
            setContextClassLoader(loader);
        }
        /**
         * Ask applet to go to the next state
         */
        synchronized void requestState(int nstate) {
            if (nstate > current_state) {
                request_state = nstate;
                notifyAll();
            }
        }
        synchronized void requestAction(AppletAction action) {
            actions.add(action);
            notifyAll();
        }
        /**
         * Get the asked state
         */
        synchronized private int getRequestState() {
            while (request_state == current_state) {
                if (!actions.isEmpty()) {
                    if (current_state >= INITIALIZED && current_state < STOPPED)
                        return ACTION;
                    else {
                        AppletAction action = (AppletAction) actions.remove(0);
                        action.fail();
                    }
                } else {
                    try {
                        wait ();
                    } catch(InterruptedException ie) {
                    }
                }
            }
            if (request_state == DESTROYED && current_state == STARTED)
                return current_state + 1; // make sure we don't skip stop()
            return request_state;
        }
        /**
         * Get the current state
         */
        synchronized int getAppletState() {
            return current_state;
        }
        /**
         * Set the current state
         */
        synchronized private void setState(int nstate) {
            current_state = nstate;
        }
        /**
         * Put applet in asked state
         * Note, kjavaapletviewer asks for create/start/stop/destroy, the
         * missing states instance/init/terminate, we do automatically
         */
        private void doState(int nstate) throws ClassNotFoundException, IllegalAccessException, InstantiationException {
            switch (nstate) {
                case CLASS_LOADED:
                    appletClass = loader.loadClass( className );
                    requestState(INSTANCIATED);
                    break;
                case INSTANCIATED: {
                    Object object = null;
                    try {
                        object = appletClass.newInstance();
                        app = (Applet) object;
                    }
                    catch ( ClassCastException e ) {
                        if ( object != null && object instanceof java.awt.Component) {
                            app = new Applet();
                            app.setLayout(new BorderLayout());
                            app.add( (Component) object, BorderLayout.CENTER);
                        } else
                            throw e;
                    }
                    acc = new AccessControlContext(new ProtectionDomain[] {app.getClass().getProtectionDomain()});
                    requestState(INITIALIZED);
                    break;
                }
                case INITIALIZED:
                    app.setStub( me );
                    app.setVisible(false);
                    panel.setApplet( app );
                    if (appletSize.getWidth() > 0)
                        app.setBounds( 0, 0, appletSize.width, appletSize.height );
                    else
                        app.setBounds( 0, 0, panel.getSize().width, panel.getSize().height );
                    active = true;
                    app.init();
                    loader.removeStatusListener(panel);
                    // stop the loading... animation 
                    panel.stopAnimation();
                    app.setVisible(true);
                    break;
                case STARTED:
                    app.start();
                    frame.validate();
                    app.repaint();
                    break;
                case STOPPED:
                    active = false;
                    app.stop();
                    if (Main.java_version > 1.399) {
                        // kill the windowClosing listener(s)
                        WindowListener[] l = frame.getWindowListeners();
                        for (int i = 0; l != null && i < l.length; i++)
                            frame.removeWindowListener(l[i]);
                    }
                    frame.setVisible(false);
                    break;
                case DESTROYED:
                    if (app != null)
                        app.destroy();
                    frame.dispose();
                    app = null;
                    requestState(TERMINATE);
                    break;
                default:
                    return;
                }
        }
        /**
         * RunThread run(), loop until state is TERMINATE
         */
        public void run() {
            while (true) {
                int nstate = getRequestState();
                if (nstate >= TERMINATE)
                    return;
                if (nstate == ACTION) {
                    AccessController.doPrivileged(
                            new PrivilegedAction() {
                                public Object run() {
                                    AppletAction action = (AppletAction) actions.remove(0);
                                    try {
                                        action.apply();
                                    } catch (Exception ex) {
                                        Main.debug("Error during action " + ex);
                                        action.fail();
                                    }
                                    return null;
                                }
                            },
                            acc);
                } else { // move to nstate
                    try {
                        doState(nstate);
                    } catch (Exception ex) {
                        Main.kjas_err("Error during state " + nstate, ex);
                        if (nstate < INITIALIZED) {
                            setState(FAILED);
                            setFailed(ex.toString());
                            return;
                        }
                    } catch (Throwable tr) {
                        setState(FAILED);
                        setFailed(tr.toString());
                        return;
                    }
                    setState(nstate);
                    stateChange(nstate);
                }
            }
        }
    }
    private RunThread                runThread = null;

    /**
     * Create an AppletStub for the specified applet. The stub will be in
     * the specified context and will automatically attach itself to the
     * passed applet.
     */
    public KJASAppletStub( KJASAppletContext _context, String _appletID,
                           URL _codeBase, URL _docBase,
                           String _appletName, String _className,
                           Dimension _appletSize, Hashtable _params,
                           String _windowName, KJASAppletClassLoader _loader )
    {
        context    = _context;
        appletID   = _appletID;
        codeBase   = _codeBase;
        docBase    = _docBase;
        active     = false;
        appletName = _appletName;
        className  = _className.replace( '/', '.' );
        appletSize = _appletSize;
        params     = _params;
        windowName = _windowName;
        loader     = _loader;
 
        String fixedClassName = _className;
        if (_className.endsWith(".class") || _className.endsWith(".CLASS"))
        {
            fixedClassName = _className.substring(0, _className.length()-6);   
        }
        else if (_className.endsWith(".java")|| _className.endsWith(".JAVA"))
        {
            fixedClassName = _className.substring(0, _className.length()-5);   
        }
        className = fixedClassName.replace('/', '.');
            
        appletClass = null;
        me = this;
        
        
    }

    private void stateChange(int newState) {
        Main.protocol.sendAppletStateNotification(
            context.getID(),
            appletID,
            newState);
    }
    
    private void setFailed(String why) {
        loader.removeStatusListener(panel);
        panel.stopAnimation();
        panel.showFailed();
        Main.protocol.sendAppletFailed(context.getID(), appletID, why); 
    }
    
    void createApplet() {
        panel = new KJASAppletPanel();
        frame = new JFrame(windowName);
        // under certain circumstances, it may happen that the
        // applet is not embedded but shown in a separate window.
        // think of konqueror running under fvwm or gnome.
        // than, the user should have the ability to close the window.
        
        frame.addWindowListener
        (
            new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    me.destroyApplet();
                }
            }
        );
        frame.getContentPane().add( panel, BorderLayout.CENTER );
	try {
            if (Main.java_version > 1.399)
                frame.setUndecorated(true);
	} catch(java.awt.IllegalComponentStateException e) {
            // This happens with gcj 4.0.1, ignore for now...
	}
        frame.setLocation( 0, 0 );
        frame.pack();
        // resize frame for j2sdk1.5beta1..
        if (appletSize.getWidth() > 0)
            frame.setBounds( 0, 0, appletSize.width, appletSize.height );
        else
            frame.setBounds( 0, 0, 50, 50 );
        frame.setVisible(true);
        loader.addStatusListener(panel);
        runThread = new RunThread();
        runThread.start();
    }

    /**
    * starts the applet managed by this stub by calling the applets start() method.
    * Also marks this stub as active.
    * @see java.applet.Applet#start()
    * @see java.applet.AppletStub#isActive()
    * 
    */
    void startApplet()
    {
        runThread.requestState(STARTED);
    }

    /**
    * stops the applet managed by this stub by calling the applets stop() method.
    * Also marks this stub as inactive.
    * @see java.applet.Applet#stop()
    * @see java.applet.AppletStub#isActive()
    * 
    */
    void stopApplet()
    {
        runThread.requestState(STOPPED);
    }

    /**
    * initialize the applet managed by this stub by calling the applets init() method.
    * @see java.applet.Applet#init()
    */
    void initApplet()
    {
        runThread.requestState(INITIALIZED);
   }

    /**
    * destroys the applet managed by this stub by calling the applets destroy() method.
    * Also marks the applet as inactive.
    * @see java.applet.Applet#init()
    */
    synchronized void destroyApplet()
    {
        runThread.requestState(DESTROYED);
    }

    static void waitForAppletThreads()
    {
        Thread [] ts = new Thread[Thread.activeCount() + 5];
        int len = Thread.enumerate(ts);
        for (int i = 0; i < len; i++) {
            try {
                if (ts[i].getName() != null && 
                        ts[i].getName().startsWith("KJAS-AppletStub-")) {
                    try {
                        ((RunThread) ts[i]).requestState(TERMINATE);
                        ts[i].join(10000);
                    } catch (InterruptedException ie) {}
                }
            } catch (Exception e) {}
        }
    }

    /**
    * get the Applet managed by this stub.
    * @return the Applet or null if the applet could not be loaded
    * or instanciated.
    */
    Applet getApplet()
    {
        if (runThread != null && runThread.getAppletState() > CLASS_LOADED)
            return app;
        return null;
    }

    /**
    * get a parameter value given in the &lt;APPLET&gt; tag 
    * @param name the name of the parameter
    * @return the value  or null if no parameter with this name exists.
    */
    
    public String getParameter( String name )
    {
        return (String) params.get( name.toUpperCase() );
    }

    /**
    * implements the isActive method of the AppletStub interface.
    * @return if the applet managed by this stub is currently active. 
    * @see java.applet.AppletStub#isActive()
    */
    public boolean isActive()
    {
        return active;
    }

    /**
    * determines if the applet has been loaded and instanciated
    * and can hence be used.
    * @return true if the applet has been completely loaded.
    */
    boolean isLoaded() {
        return runThread != null && runThread.getAppletState() >= INSTANCIATED;
    }
    
    public void appletResize( int width, int height )
    {
        if( active )
        {
            if ( (width >= 0) && (height >= 0))
            {
                Main.debug( "Applet #" + appletID + ": appletResize to : (" + width + ", " + height + ")" );
                Main.protocol.sendResizeAppletCmd( context.getID(), appletID, width, height );
                appletSize = new Dimension( width, height );
                //pack();
            }
        }
    }

    /**
    * converts Object <b>arg</b> into an object of class <b>cl</b>.
    * @param arg Object to convert
    * @param cl Destination class
    * @return An Object of the specified class with the value specified
    *  in <b>arg</b>
    */
    private static final Object cast(Object arg, Class cl) throws NumberFormatException {
        Object ret = arg;
        if (arg == null) {
            ret = null;
        }
        else if (cl.isAssignableFrom(arg.getClass())) {
            return arg;
        }
        else if (arg instanceof String) {
            String s = (String)arg;
            Main.debug("Argument String: \"" + s + "\"");
            if (cl == Boolean.TYPE || cl == Boolean.class) {
                ret = new Boolean(s);
            } else if (cl == Integer.TYPE || cl == Integer.class) {
                ret = new Integer(s);
            } else if (cl == Long.TYPE || cl == Long.class) {
                ret = new Long(s);
            } else if (cl == Float.TYPE || cl == Float.class) {
                ret = new Float(s);
            } else if (cl == Double.TYPE || cl == Double.class) {
                ret = new Double(s);
            } else if (cl == Short.TYPE || cl == Short.class) {
                ret = new Short(s);
            } else if (cl  == Byte.TYPE || cl == Byte.class) {
                ret = new Byte(s);
            } else if (cl == Character.TYPE || cl == Character.class) {
                ret = new Character(s.charAt(0));
            }
        }
        return ret;
    }
    private Method findMethod(Class c, String name, Class [] argcls) {
        try {
            Method[] methods = c.getMethods();
            for (int i = 0; i < methods.length; i++) {
                Method m = methods[i];
                if (m.getName().equals(name)) {
                    Main.debug("Candidate: " + m);
                    Class [] parameterTypes = m.getParameterTypes();
                    if (argcls == null) {
                        if (parameterTypes.length == 0) {
                           return m;
                        } 
                    } else {
                        if (argcls.length == parameterTypes.length) {
                          for (int j = 0; j < argcls.length; j++) {
                            // Main.debug("Parameter " + j + " " + parameterTypes[j]);
                            argcls[j] = parameterTypes[j];
                          }
                          return m;                        
                        }
                    }
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }
    private int[] getJSTypeValue(Hashtable jsRefs, Object obj, int objid, StringBuffer value) {
        String val = obj.toString();
        int[] rettype = { JError, objid };
        String type = obj.getClass().getName();
        if (type.equals("boolean") || type.equals("java.lang.Boolean"))
            rettype[0] = JBoolean;
        else if (type.equals("int") || type.equals("long") || 
                type.equals("float") || type.equals("double") ||
                type.equals("byte") || obj instanceof java.lang.Number)
            rettype[0] = JNumber;
        else if (type.equals("java.lang.String"))
            rettype[0] = JString;
        else if (!type.startsWith("org.kde.kjas.server") &&
                 !(obj instanceof java.lang.Class &&
                   ((Class)obj).getName().startsWith("org.kde.kjas.server"))) {
            rettype[0] = JObject;
            rettype[1] = obj.hashCode();
            jsRefs.put(new Integer(rettype[1]), obj);
        }
        value.insert(0, val);
        return rettype;
    }
    private class PutAction implements AppletAction {
        int call_id;
        int objid;
        String name;
        String value;
        PutAction(int cid, int oid, String n, String v) {
            call_id = cid;
            objid = oid;
            name = n;
            value = v;
        }
        public void apply() {
            Hashtable jsRefs = loader.getJSReferencedObjects();
            Object o = objid==0 ? getApplet() : jsRefs.get(new Integer(objid));
            if (o == null) {
                Main.debug("Error in putValue: object " + objid + " not found");
                fail();
                return;
            }
            Field f;
            try {
                f = o.getClass().getField(name);
            } catch (Exception e) {
                fail();
                return;
            }
            if (f == null) {
                Main.debug("Error in putValue: " + name + " not found");
                fail();
                return;
            }
            try {
                String type = f.getType().getName();
                Main.debug("putValue: (" + type + ")" + name + "=" + value);
                if (type.equals("boolean"))
                    f.setBoolean(o, Boolean.getBoolean(value));
                else if (type.equals("java.lang.Boolean"))
                    f.set(o, Boolean.valueOf(value));
                else if (type.equals("int"))
                    f.setInt(o, Integer.parseInt(value));
                else if (type.equals("java.lang.Integer"))
                    f.set(o, Integer.valueOf(value));
                else if (type.equals("byte"))
                    f.setByte(o, Byte.parseByte(value));
                else if (type.equals("java.lang.Byte"))
                    f.set(o, Byte.valueOf(value));
                else if (type.equals("char"))
                    f.setChar(o, value.charAt(0));
                else if (type.equals("java.lang.Character"))
                    f.set(o, new Character(value.charAt(0)));
                else if (type.equals("double"))
                    f.setDouble(o, Double.parseDouble(value));
                else if (type.equals("java.lang.Double"))
                    f.set(o, Double.valueOf(value));
                else if (type.equals("float"))
                    f.setFloat(o, Float.parseFloat(value));
                else if (type.equals("java.lang.Float"))
                    f.set(o, Float.valueOf(value));
                else if (type.equals("long"))
                    f.setLong(o, Long.parseLong(value));
                else if (type.equals("java.lang.Long"))
                    f.set(o, Long.valueOf(value));
                else if (type.equals("short"))
                    f.setShort(o, Short.parseShort(value));
                else if (type.equals("java.lang.Short"))
                    f.set(o, Short.valueOf(value));
                else if (type.equals("java.lang.String"))
                    f.set(o, value);
                else {
                    Main.debug("Error putValue: unsupported type: " + type);
                    fail();
                    return;
                }
            } catch (Exception e) {
                Main.debug("Exception in putValue: " + e.getMessage());
                fail();
                return;
            }
            Main.protocol.sendPutMember( context.getID(), call_id, true ); 
        }
        public void fail() {
            Main.protocol.sendPutMember( context.getID(), call_id, false ); 
        }
    }
    private class GetAction implements AppletAction {
        int call_id;
        int objid;
        String name;
        GetAction(int cid, int oid, String n) {
            call_id = cid;
            objid = oid;
            name = n;
        }
        public void apply() {
            Main.debug("getMember: " + name);
            StringBuffer value = new StringBuffer();
            int ret[] = { JError, objid };
            Hashtable jsRefs = loader.getJSReferencedObjects();
            Object o = objid==0 ? getApplet() : jsRefs.get(new Integer(objid));
            if (o == null) {
                fail();
                return;
            }
            Class c = o.getClass();
            try {
                Field field = c.getField(name);
                ret = getJSTypeValue(jsRefs, field.get(o), objid, value);
            } catch (Exception ex) {
                Method [] m = c.getMethods();
                for (int i = 0; i < m.length; i++)
                    if (m[i].getName().equals(name)) {
                        ret[0] = JFunction;
                        break;
                    }
            }
            Main.protocol.sendMemberValue(context.getID(), KJASProtocolHandler.GetMember, call_id, ret[0], ret[1], value.toString()); 
        }
        public void fail() {
            Main.protocol.sendMemberValue(context.getID(), KJASProtocolHandler.GetMember, call_id, -1, 0, ""); 
        }
    }
    private class CallAction implements AppletAction {
        int call_id;
        int objid;
        String name;
        java.util.List args;
        CallAction(int cid, int oid, String n, java.util.List a) {
            call_id = cid;
            objid = oid;
            name = n;
            args = a;
        }
        public void apply() {
            StringBuffer value = new StringBuffer();
            Hashtable jsRefs = loader.getJSReferencedObjects();
            int [] ret = { JError, objid };
            Object o = objid==0 ? getApplet() : jsRefs.get(new Integer(objid));
            if (o == null) {
                fail();
                return;
            }

            try {
                Main.debug("callMember: " + name);
                Object obj;
                Class c = o.getClass();
                String type;
                Class [] argcls = new Class[args.size()];
                for (int i = 0; i < args.size(); i++)
                    argcls[i] = name.getClass(); // String for now, will be updated by findMethod
                Method m = findMethod(c, (String) name, argcls);
                Main.debug("Found Method: " + m);
                if (m != null) {
                    Object [] argobj = new Object[args.size()];
                    for (int i = 0; i < args.size(); i++) {
                        argobj[i] = cast(args.get(i), argcls[i]);
                    }
                    Object retval =  m.invoke(o, argobj);
                    if (retval == null)
                        ret[0] = JVoid;
                    else
                        ret = getJSTypeValue(jsRefs, retval, objid, value);
                }
            } catch (Exception e) {
                Main.debug("callMember threw exception: " + e.toString());
            }
            Main.protocol.sendMemberValue(context.getID(), KJASProtocolHandler.CallMember, call_id, ret[0], ret[1], value.toString()); 
        }
        public void fail() {
            Main.protocol.sendMemberValue(context.getID(), KJASProtocolHandler.CallMember, call_id, -1, 0, ""); 
        }
    }
    boolean putMember(int callid, int objid, String name, String val) {
        if (runThread == null)
            return false;
        runThread.requestAction( new PutAction( callid, objid, name, val) );
        return true;
    }
    boolean getMember(int cid, int oid, String name) {
        if (runThread == null)
            return false;
        runThread.requestAction( new GetAction( cid, oid, name) );
        return true;
    }
    boolean callMember(int cid, int oid, String name, java.util.List args) {
        if (runThread == null)
            return false;
        runThread.requestAction( new CallAction( cid, oid, name, args) );
        return true;
    }
    /*************************************************************************
     ********************** AppletStub Interface *****************************
     *************************************************************************/
    /**
    * implements the getAppletContext method of the AppletStub interface.
    * @return the AppletContext to which this stub belongs.
    * @see java.applet.AppletStub#getAppletContext()
    */
    public AppletContext getAppletContext()
    {
        return context;
    }

    /**
    * implements the getCodeBase method of the AppletStub interface.
    * @return the code base of the applet as given in the &lt;APPLET&gt; tag.
    * @see java.applet.AppletStub#getCodeBase()
    */
    public URL getCodeBase()
    {
        return codeBase;
    }

    /**
    * implements the getDocumentBase method of the AppletStub interface.
    * @return the code base of the applet as given in the 
    * &lt;APPLET&gt; tag or determined by the containing page.
    * @see java.applet.AppletStub#getDocumentBase()
    */
    public URL getDocumentBase()
    {
        return docBase;
    }

    /**
    * get the applet's name
    * @return the name of the applet as given in the 
    * &lt;APPLET&gt; tag or determined by the <em>code</em> parameter.
    */
    public String getAppletName()
    {
        return appletName;
    }

}
