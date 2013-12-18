package org.kde.kjas.server;

import java.applet.*;
import java.net.*;
import java.util.*;
/**
* Background Audioclip Loader and Player.
* @author Till Krech (till@snafu.de)
*/
public class KJASAudioClip implements AudioClip
{
    private AudioClip theClip;
    private final int PLAYING = 1;
    private final int LOOPING = 2;
    private final int STOPPED = 3;
    private int state;
    private static Hashtable cache = new Hashtable();

    /**
    * creates a new Audioclip.
    * The AudioClip is loaded in background. The Constructor returns immediately.
    */
    public KJASAudioClip(URL url)
    {
        state = STOPPED;
        theClip = (AudioClip)cache.get(url);
        if (theClip == null) {
            final URL theUrl = url;
         
            new Thread
            (
                new Runnable() {
                    public void run() {
                        theClip = java.applet.Applet.newAudioClip(theUrl);
                        cache.put(theUrl, theClip);
                        if (state == LOOPING) {
                            theClip.loop();
                        } else if (state == PLAYING) {
                            theClip.play();
                        }
                    }
                }, "AudioClipLoader " + url.getFile()
            ).start();
        }           
    }

    /**
    * play continously when the clip is loaded
    */
    public void loop()
    {
        state = LOOPING;
        if (theClip != null) {
            new Thread
            (
                new Runnable() {
                    public void run() {
                       theClip.loop();
                    }
                }, "AudioClipLooper "
            ).start();           
        }  
    }

    /**
    * play when the clip is loaded
    */
    public void play()
    {
        state = PLAYING;
        if (theClip != null) {
            new Thread
            (
                new Runnable() {
                    public void run() {
                       theClip.play();
                    }
                }, "AudioClipPlayer "
            ).start();           
        }  
    }

    /**
    * stop the clip
    */
    public void stop()
    {
        state = STOPPED;
        if (theClip != null) {
            theClip.stop();
        }
    }
    
    public void finalize() {
        stop();
    }
}

