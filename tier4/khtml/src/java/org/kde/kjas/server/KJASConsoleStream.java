package org.kde.kjas.server;
import java.io.*;

class KJASConsoleStream
    extends OutputStream
{
    private Console console;
    private FileOutputStream dbg_log;

    public KJASConsoleStream(Console console)
    {
        this.console = console;

        try
        {
            if( Main.log )
            {
                dbg_log = new FileOutputStream( "/tmp/kjas.log");
            }
        }
        catch( FileNotFoundException e ) {}
    }

    public void close() {}
    public void flush() {}
    public void write(byte[] b) {}
    public void write(int a) {}

    // Should be enough for the console
    public void write( byte[] bytes, int offset, int length )
    {
        try  // Just in case
        {
            String msg = new String( bytes, offset, length );
            console.append(msg);
                if( Main.log && dbg_log != null )
                {
                    dbg_log.write( msg.getBytes() );
                    dbg_log.flush();
                }
        }
        catch(Throwable t) {}
    }
}


