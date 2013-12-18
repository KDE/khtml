package org.kde.kjas.server;

import java.io.*;
import java.util.*;
import java.awt.*;
import java.net.*;

/**
 * Encapsulates the KJAS protocol and manages the contexts
 *
 */
public class KJASProtocolHandler
{
    // Command codes- always need to be synced up with
    // what's in kjavaappletserver.cpp
    private static final int CreateContextCode   = 1;
    private static final int DestroyContextCode  = 2;
    private static final int CreateAppletCode    = 3;
    private static final int DestroyAppletCode   = 4;
    private static final int StartAppletCode     = 5;
    private static final int StopAppletCode      = 6;
    private static final int InitAppletCode      = 7;
    private static final int ShowDocumentCode    = 8;
    private static final int ShowURLInFrameCode  = 9;
    private static final int ShowStatusCode      = 10;
    private static final int ResizeAppletCode    = 11;
    private static final int GetURLDataCode      = 12;
    private static final int URLDataCode         = 13;
    private static final int ShutdownServerCode  = 14;
    private static final int JavaScriptEvent     = 15;
    static final int GetMember                   = 16;
    static final int CallMember                  = 17;
    private static final int PutMember           = 18;
    private static final int DerefObject         = 19;

    private static final int AudioClipPlayCode   = 20;
    private static final int AudioClipLoopCode   = 21;
    private static final int AudioClipStopCode   = 22;
    
    private static final int AppletStateNotificationCode = 23;
    private static final int AppletFailedCode    = 24;
    private static final int DataCommand         = 25;
    private static final int PutURLDataCode      = 26;
    private static final int PutDataCode         = 27;
    private static final int SecurityConfirmCode = 28;
    private static final int ShowConsole         = 29;

    //Holds contexts in contextID-context pairs
    private Hashtable contexts;

    private PushbackInputStream commands;    //Stream for reading in commands
    private PrintStream         signals;     //Stream for writing out callbacks

    //used for parsing each command as it comes in
    private int cmd_index;
    private final static char sep = (char) 0;

    public KJASProtocolHandler( InputStream  _commands,
                                OutputStream _signals )
    {
        commands = new PushbackInputStream( _commands );
        signals  = new PrintStream( _signals );
        contexts = new Hashtable();
    }

    public void commandLoop()
    {
        try
        {
            while( true )
            {
                try
                {
                    int cmd_length = readPaddedLength( 8 );
                    Main.debug( "PH: cmd_length = " + cmd_length );

                    //We need to have this while loop since we're not guaranteed to get
                    //all the bytes we want back, especially with large jars
                    byte[] cmd_data = new byte[cmd_length];
                    int total_read = 0;
                    while( total_read < cmd_length )
                    {
                        int numread = commands.read( cmd_data, total_read, cmd_length-total_read );
                        Main.debug( "PH: read in " + numread + " bytes for command" );
                        total_read += numread;
                    }

                    //parse the rest of the command and execute it
                    processCommand( cmd_data );
                }
                catch( NumberFormatException e )
                {
                    Main.kjas_err( "Could not parse out message length", e );
                    e.printStackTrace();
                    System.exit( 1 );
                }
                catch( Throwable t )
                {
                    Main.debug( "commandLoop caught a throwable, still going" );
                    t.printStackTrace();
                }
            }
        }
        catch( Exception i )
        {
            Main.kjas_err( "commandLoop exited on exception: ", i );
                    i.printStackTrace();
            System.exit( 1 );
        }
    }

    public void processCommand( byte[] command )
    {
        // Sanity checks
        if ( command == null )
            return;

        //do all the parsing here and pass arguments as individual variables to the
        //handler functions
        int cmd_length = command.length;
        cmd_index = 0;

        int cmd_code_value = (int) command[cmd_index++];
        if( cmd_code_value == CreateContextCode )
        {
            //parse out contextID- 1 argument
            String contextID = getArg( command );
            Main.debug( "createContext, id = " + contextID );

            KJASAppletContext context = new KJASAppletContext( contextID );
            contexts.put( contextID, context );
        } else
        if( cmd_code_value == DestroyContextCode )
        {
            //parse out contextID- 1 argument
            String contextID = getArg( command );
            Main.debug( "destroyContext, id = " + contextID );

            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if( contexts != null )
            {
                context.destroy();
                contexts.remove( contextID );
            }
        } else
        if( cmd_code_value == CreateAppletCode )
        {
            //9 arguments- this order is important...
            final String contextID  = getArg( command );
            final String appletID   = getArg( command );
            final String appletName = getArg( command );
            final String className  = getArg( command );
            final String baseURL    = getArg( command );
            final String username   = getArg( command );
            final String password   = getArg( command );
            final String authname   = getArg( command );
            final String codeBase   = getArg( command );
            final String archives   = getArg( command );
            final String width      = getArg( command );
            final String height     = getArg( command );
            final String title      = getArg( command );

            //get the number of parameter pairs...
            String str_params = getArg( command );
            int num_params = Integer.parseInt( str_params.trim() );
            final Hashtable params = new Hashtable();
            for( int i = 0; i < num_params; i++ )
            {
                String name  = getArg( command ); // note name is in uppercase
                if( name == null )
                    name = new String();

                String value = getArg( command );
                if( value == null )
                    value = new String();
                params.put( name, value );
                //Main.debug( "parameter, name = " + name + ", value = " + value );
            }

            Main.debug( "createApplet, context = " + contextID + ", applet = " + appletID );
            Main.debug( "              name = " + appletName + ", classname = " + className );
            Main.debug( "              baseURL = " + baseURL + ", codeBase = " + codeBase );
            Main.debug( "              archives = " + archives + ", width = " + width + 
                        ", height = " + height );

            final KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if( context != null )
            {
                context.createApplet( appletID, appletName, className,
                                      baseURL, username, password, authname,
                                      codeBase, archives,
                                      width, height, title, params );
            }

        } else
        if( cmd_code_value == DestroyAppletCode )
        {
            //2 arguments
            String contextID = getArg( command );
            String appletID  = getArg( command );
            Main.debug( "destroyApplet, context = " + contextID + ", applet = " + appletID );

            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context != null )
                context.destroyApplet( appletID );
        } else
        if( cmd_code_value == StartAppletCode )
        {
            //2 arguments
            String contextID = getArg( command );
            String appletID  = getArg( command );
            Main.debug( "startApplet, context = " + contextID + ", applet = " + appletID );

            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context != null )
                context.startApplet( appletID );
        } else
        if( cmd_code_value == StopAppletCode )
        {
            //2 arguments
            String contextID = getArg( command );
            String appletID  = getArg( command );
            Main.debug( "stopApplet, context = " + contextID + ", applet = " + appletID );

            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context != null )
                context.stopApplet( appletID );
        } else
        if( cmd_code_value == ShutdownServerCode )
        {
            Main.debug( "shutDownServer received" );
            KJASAppletStub.waitForAppletThreads();
            System.exit( 1 );
        }
        else
        if( cmd_code_value == URLDataCode )
        {
            
            String id = getArg( command );
            String code = getArg( command );
            Main.debug( "KIO URLData received(" + id + ") code:" + code );

            //rest of the command should be the data...
            byte[] data = null;
            if (cmd_length - cmd_index > 0) {
                data = new byte[ cmd_length - cmd_index ];
                System.arraycopy( command, cmd_index, data, 0, data.length );
            }
            KIOConnection.setData(id, Integer.parseInt(code), data);
        } else
        if (cmd_code_value == GetMember)
        {
            int ticketnr = Integer.parseInt( getArg( command ) );
            String contextID = getArg( command );
            String appletID  = getArg( command );
            int objid  = Integer.parseInt( getArg( command ) );
            String name  = getArg( command );
            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context == null || !context.getMember(appletID, ticketnr, objid, name))
                sendMemberValue(contextID, GetMember, ticketnr, -1, 0, ""); 
        } else
        if (cmd_code_value == PutMember)
        {
            int ticketnr = Integer.parseInt( getArg( command ) );
            String contextID = getArg( command );
            String appletID  = getArg( command );
            int objid  = Integer.parseInt( getArg( command ) );
            String name  = getArg( command );
            String value  = getArg( command );
            boolean ret = false;
            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if (context == null || !context.putMember(appletID, ticketnr, objid, name, value))
                sendPutMember(contextID, ticketnr, false); 
        } else
        if (cmd_code_value == CallMember)
        {
            int ticketnr = Integer.parseInt( getArg( command ) );
            String contextID = getArg( command );
            String appletID  = getArg( command );
            int objid  = Integer.parseInt( getArg( command ) );
            String name  = getArg( command );
            java.util.List args = new java.util.Vector();
            try { // fix getArg
                String param = getArg(command);
                while (param != null) {
                    args.add(param);
                    param = getArg(command);
                }
            } catch (Exception e) {}

            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context == null || !context.callMember(appletID, ticketnr, objid, name, args))
                Main.protocol.sendMemberValue(contextID, CallMember, ticketnr, -1, 0, ""); 
        } else
        if (cmd_code_value == DerefObject)
        {
            String contextID = getArg( command );
            String appletID  = getArg( command );
            String objid  = getArg( command );
            KJASAppletContext context = (KJASAppletContext) contexts.get( contextID );
            if ( context != null )
                context.derefObject(appletID, Integer.parseInt(objid));
            Main.debug( "DerefObject " + objid);
        } else
        if (cmd_code_value == SecurityConfirmCode)
        {
            String id = getArg( command );
            String confirm = getArg( command );
            Thread t = (Thread) KJASSecurityManager.confirmRequests.get(id);
            Main.debug( "SecurityConfirmCode " + id + " confirm:" + confirm );
            if (t != null) {
                KJASSecurityManager.confirmRequests.put(id, confirm);
                try {
                    t.interrupt();
                } catch (SecurityException se) {}
            }
        } else
        if (cmd_code_value == ShowConsole)
        {
            Main.console.setVisible(true);
        }
        else
        {
           throw new IllegalArgumentException( "Unknown command code" );
        }
    }

    /**************************************************************
     *****  Methods for talking to the applet server **************
     **************************************************************/

    /**
    * sends get url request
    */
    public void sendGetURLDataCmd( String jobid, String url )
    {
        Main.debug( "sendGetURLCmd(" + jobid + ") url = " + url );
        //length  = length of args plus 1 for code, 2 for seps and 1 for end
        byte [] url_bytes = url.getBytes();
        int length = jobid.length() + url_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ];
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) GetURLDataCode;
        bytes[index++] = sep;

        tmp_bytes = jobid.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( url_bytes, 0, bytes, index, url_bytes.length );
        index += url_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }

    /**
    * sends command for get url request (stop/hold/resume) or put (stop)
    */
    public void sendDataCmd( String id, int cmd )
    {
        Main.debug( "sendDataCmd(" + id + ") command = " + cmd );
        byte [] cmd_bytes = String.valueOf( cmd ).getBytes();
        int length = id.length() + cmd_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ];
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) DataCommand;
        bytes[index++] = sep;

        tmp_bytes = id.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( cmd_bytes, 0, bytes, index, cmd_bytes.length );
        index += cmd_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    /**
    * sends put url request
    */
    public void sendPutURLDataCmd( String jobid, String url )
    {
        Main.debug( "sendPutURLCmd(" + jobid + ") url = " + url );
        //length  = length of args plus 1 for code, 2 for seps and 1 for end
        byte [] url_bytes = url.getBytes();
        int length = jobid.length() + url_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ];
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) PutURLDataCode;
        bytes[index++] = sep;

        tmp_bytes = jobid.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( url_bytes, 0, bytes, index, url_bytes.length );
        index += url_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    /**
    * sends put data
    */
    public void sendPutData( String jobid, byte [] b, int off, int len )
    {
        Main.debug( "sendPutData(" + jobid + ") len = " + len );
        //length  = length of args plus 1 for code, 2 for seps and 1 for end
        int length = jobid.length() + len + 4;
        byte [] bytes = new byte[ length + 8 ];
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) PutDataCode;
        bytes[index++] = sep;

        tmp_bytes = jobid.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( b, off, bytes, index, len );
        index += len;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    /**
    * sends notification about the state of the applet.
    * @see org.kde.kjas.server.KJASAppletStub for valid states
    */
    public void sendAppletStateNotification( String contextID, String appletID, int state )
    {
        Main.debug( "sendAppletStateNotification, contextID = " + contextID + ", appletID = " +
                    appletID + ", state=" + state );

        byte [] state_bytes = String.valueOf( state ).getBytes();

        int length = contextID.length() + appletID.length() + state_bytes.length + 5;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) AppletStateNotificationCode;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = appletID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( state_bytes, 0, bytes, index, state_bytes.length );
        index += state_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
 
    /**
    * sends notification about applet failure.
    * This can happen in any state.
    * @param contextID context ID of the applet's context
    * @param appletID  ID of the applet
    * @param errorMessage any message
    */
    public void sendAppletFailed ( String contextID, String appletID, String errorMessage)
    {
        Main.debug( "sendAppletFailed, contextID = " + contextID + ", appletID = " +
                    appletID + ", errorMessage=" + errorMessage );
        byte [] msg_bytes = errorMessage.getBytes();
        int length = contextID.length() + appletID.length() + msg_bytes.length + 5;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) AppletFailedCode;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = appletID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( msg_bytes, 0, bytes, index, msg_bytes.length );
        index += msg_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
   
    public void sendShowDocumentCmd( String loaderKey, String url )
    {
        Main.debug( "sendShowDocumentCmd from context#" + loaderKey + " url = " + url );

        //length = length of args + 2 for seps + 1 for end + 1 for code
        byte [] url_bytes = url.getBytes();
        byte [] key_bytes = loaderKey.getBytes();
        int length = key_bytes.length + url_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ]; //8 for the length of this message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) ShowDocumentCode;
        bytes[index++] = sep;

        System.arraycopy( key_bytes, 0, bytes, index, key_bytes.length );
        index += key_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( url_bytes, 0, bytes, index, url_bytes.length );
        index += url_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }

    public void sendShowDocumentCmd( String contextID, String url, String frame)
    {
        Main.debug( "sendShowDocumentCmd from context#" + contextID +
                         " url = " + url + ", frame = " + frame );

        //length = length of args plus code, 3 seps, end
        byte [] url_bytes = url.getBytes();
        byte [] frame_bytes = frame.getBytes();
        int length = contextID.length() + url_bytes.length + frame_bytes.length + 5;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) ShowURLInFrameCode;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( url_bytes, 0, bytes, index, url_bytes.length );
        index += url_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( frame_bytes, 0, bytes, index, frame_bytes.length );
        index += frame_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }

    public void sendShowStatusCmd( String contextID, String msg )
    {
        Main.debug( "sendShowStatusCmd, contextID = " + contextID + " msg = " + msg );

        byte [] msg_bytes = msg.getBytes();
        int length = contextID.length() + msg_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        int index = 0;

        byte [] tmp_bytes = getPaddedLengthBytes( length );
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) ShowStatusCode;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( msg_bytes, 0, bytes, index, msg_bytes.length );
        index += msg_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }

    public void sendResizeAppletCmd( String contextID, String appletID,
                                     int width, int height )
    {
        Main.debug( "sendResizeAppletCmd, contextID = " + contextID + ", appletID = " +
                    appletID + ", width = " + width + ", height = " + height );

        byte [] width_bytes = String.valueOf( width ).getBytes();
        byte [] height_bytes = String.valueOf( height ).getBytes();

        //length = length of args plus code, 4 seps, end
        int length = contextID.length() + appletID.length() + width_bytes.length +
                     height_bytes.length + 6;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) ResizeAppletCode;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = appletID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( width_bytes, 0, bytes, index, width_bytes.length );
        index += width_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( height_bytes, 0, bytes, index, height_bytes.length );
        index += height_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    public void sendJavaScriptEventCmd( String contextID, String appletID, int objid, String event, int [] types, String [] args )
    {
        Main.debug( "sendJavaScriptEventCmd, contextID = " + contextID + " event = " + event );
        String objstr = new String("" + objid);
        int length = contextID.length() + appletID.length() + event.length() + objstr.length() + 6;
        byte [][][] arglist = null;
        if (types != null) {
            arglist = new byte[args.length][2][];
            for (int i = 0; i < types.length; i++) {
                arglist[i][0] = (new String("" + types[i])).getBytes();
                arglist[i][1] = args[i].getBytes();
                length += 2 + arglist[i][0].length + arglist[i][1].length;
            }
        }
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        int index = 0;

        byte [] tmp_bytes = getPaddedLengthBytes( length );
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) JavaScriptEvent;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = appletID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = objstr.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = event.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        if (types != null)
            for (int i = 0; i < types.length; i++) {
                System.arraycopy( arglist[i][0], 0, bytes, index, arglist[i][0].length );
                index += arglist[i][0].length;
                bytes[index++] = sep;
                System.arraycopy( arglist[i][1], 0, bytes, index, arglist[i][1].length );
                index += arglist[i][1].length;
                bytes[index++] = sep;
            }

        signals.write( bytes, 0, bytes.length );
    }
    public void sendMemberValue( String contextID, int cmd, int ticketnr, int type, int rid, String value )
    {
        Main.debug( "sendMemberValue, contextID = " + contextID + " value = " + value + " type=" + type + " rid=" + rid );

        String strticket = String.valueOf( ticketnr );
        String strtype = String.valueOf( type );
        String strobj = String.valueOf( rid );
        byte [] value_bytes = value.getBytes();
        int length = contextID.length() + value_bytes.length + strtype.length() + strobj.length() + strticket.length() + 7;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        int index = 0;

        byte [] tmp_bytes = getPaddedLengthBytes( length );
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) cmd;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = strticket.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = strtype.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        tmp_bytes = strobj.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( value_bytes, 0, bytes, index, value_bytes.length );
        index += value_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }

    private void sendAudioClipCommand(String contextId, String url, int cmd) {
        byte [] url_bytes = url.getBytes();
        int length = contextId.length() + url_bytes.length + 4;
        byte [] bytes = new byte[ length + 8 ];
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) cmd;
        bytes[index++] = sep;

        tmp_bytes = contextId.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( url_bytes, 0, bytes, index, url_bytes.length );
        index += url_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    
    public void sendAudioClipPlayCommand(String contextId, String url) {
        sendAudioClipCommand(contextId, url, AudioClipPlayCode);
    }
    public void sendAudioClipLoopCommand(String contextId, String url) {
        sendAudioClipCommand(contextId, url, AudioClipLoopCode);
    }
    public void sendAudioClipStopCommand(String contextId, String url) {
        sendAudioClipCommand(contextId, url, AudioClipStopCode);
    }

    public void sendPutMember( String contextID, int ticketnr, boolean success )
    {
        Main.debug("sendPutMember, contextID = " + contextID + " success = " + success);

        byte [] ticket_bytes = String.valueOf( ticketnr ).getBytes();
        byte [] ret_bytes = String.valueOf( success ? "1" : "0" ).getBytes();
        int length = contextID.length() + ret_bytes.length + ticket_bytes.length + 5;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) PutMember;
        bytes[index++] = sep;

        tmp_bytes = contextID.getBytes();
        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( ticket_bytes, 0, bytes, index, ticket_bytes.length );
        index += ticket_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( ret_bytes, 0, bytes, index, ret_bytes.length );
        index += ret_bytes.length;
        bytes[index++] = sep;

        signals.write( bytes, 0, bytes.length );
    }
    public void sendSecurityConfirm( String [] certs, int certsnr, String perm, String id )
    {
        Main.debug("sendSecurityConfirm, ID = " + id + " certsnr = " + certsnr);

        byte [] id_bytes = id.getBytes();
        byte [] perm_bytes = perm.getBytes();
        byte [] certsnr_bytes = String.valueOf( certsnr ).getBytes();
        int length = perm_bytes.length + id_bytes.length + certsnr_bytes.length + 5;
        for (int i = 0; i < certsnr; i++)
            length += certs[i].length() + 1;
        byte [] bytes = new byte[ length + 8 ]; //for length of message
        byte [] tmp_bytes = getPaddedLengthBytes( length );
        int index = 0;

        System.arraycopy( tmp_bytes, 0, bytes, index, tmp_bytes.length );
        index += tmp_bytes.length;
        bytes[index++] = (byte) SecurityConfirmCode;
        bytes[index++] = sep;

        System.arraycopy( id_bytes, 0, bytes, index, id_bytes.length );
        index += id_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( perm_bytes, 0, bytes, index, perm_bytes.length );
        index += perm_bytes.length;
        bytes[index++] = sep;

        System.arraycopy( certsnr_bytes, 0, bytes, index, certsnr_bytes.length );
        index += certsnr_bytes.length;
        bytes[index++] = sep;

        for (int i = 0; i < certsnr; i++) {
            byte [] cert_bytes = certs[i].getBytes();
            System.arraycopy( cert_bytes, 0, bytes, index, cert_bytes.length );
            index += cert_bytes.length;
            bytes[index++] = sep;
        }

        signals.write( bytes, 0, bytes.length );
    }
    /**************************************************************
     *****  Utility functions for parsing commands ****************
     **************************************************************/
    private String getArg( byte[] command )
    {
        int begin = cmd_index;
        while( 0 != ((int) command[cmd_index++]) );

        if( cmd_index > (begin + 1) )
        {
            String rval = new String( command, begin, (cmd_index - begin - 1) );
            return rval;
        }
        else
            return null;
    }

    private byte[] getPaddedLengthBytes( int length )
    {
        byte[] length_bytes = String.valueOf( length ).getBytes();
        if( length_bytes.length > 8 )
           throw new IllegalArgumentException( "can't create string number of length = 8" );
        byte [] bytes = { (byte) ' ', (byte) ' ', (byte) ' ', (byte) ' ',
                          (byte) ' ', (byte) ' ', (byte) ' ', (byte) ' '};
        System.arraycopy( length_bytes, 0, bytes, 0, length_bytes.length );
        return bytes;
    }
    private int readPaddedLength( int string_size )
        throws IOException
    {
            //read in 8 bytes for command length- length will be sent as a padded string
            byte[] length = new byte[string_size];
            commands.read( length, 0, string_size );

            String length_str = new String( length );
            return Integer.parseInt( length_str.trim() );
    }

}
