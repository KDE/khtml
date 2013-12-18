package org.kde.kjas.server;

import java.awt.*;
import java.awt.event.*;
import java.io.*;

public class KJASConsole
    extends Frame
    implements Console
{
    private TextArea txt;

    public KJASConsole()
    {
        super("Konqueror Java Console");

        txt = new TextArea();
        txt.setEditable(false);
        txt.setBackground(Color.white);
        txt.setForeground(Color.black);

        Panel main = new Panel(new BorderLayout());
        Panel btns = new Panel(new BorderLayout());

        Button clear = new Button("Clear");
        Button close = new Button("Close");
        
        btns.add(clear, "West");
        btns.add(close, "East");

        main.add(txt, "Center");
        main.add(btns, "South");
        
        add( main );

        clear.addActionListener
        (
            new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    txt.setText("");
                }
            }
        );

        close.addActionListener
        (
            new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    setVisible(false);
                }
            }
        );
        
        addWindowListener
        (
            new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    setVisible(false);
                }
            }
        );

        setSize(500, 300);

        PrintStream st = new PrintStream( new KJASConsoleStream(this) );
        System.setOut(st);
        System.setErr(st);
        
        System.out.println( "Java VM version: " +
                            System.getProperty("java.version") );
        System.out.println( "Java VM vendor:  " +
                            System.getProperty("java.vendor") );
    }
    
    public void clear() {
      txt.setText("");
    }

    public void append(String msg) {
        if (msg == null) {
            return;
        }
        int length = msg.length();
        synchronized(txt) {
            //get the caret position, and then get the new position
            int old_pos = txt.getCaretPosition();
            txt.append(msg);
            txt.setCaretPosition( old_pos + length );
        }
    }   
}


