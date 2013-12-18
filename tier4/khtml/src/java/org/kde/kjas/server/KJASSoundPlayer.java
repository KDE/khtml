package org.kde.kjas.server;

import java.applet.*;
import java.net.*;

public class KJASSoundPlayer implements AudioClip
{
    private String file;
    private String contextId;

    public KJASSoundPlayer( String _contextId, URL _file )
    {
        file = _file.toString();
        contextId = _contextId;
        Main.debug("KJASSoundPlayer( URL '" + _file + "')");
    }

    public void loop()
    {
        Main.debug("KJASSoundPlayer loop() URL='" + file + "'");
        Main.protocol.sendAudioClipLoopCommand(contextId, file);
    }

    public void play()
    {
       Main.debug("KJASSoundPlayer play() URL='" + file + "'");
       Main.protocol.sendAudioClipPlayCommand(contextId, file);
    }

    public void stop()
    {
       Main.debug("KJASSoundPlayer stop() URL='" + file + "'");
       Main.protocol.sendAudioClipStopCommand(contextId, file);
    }
}

