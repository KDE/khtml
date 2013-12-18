/* This file is part of the KDE project
 *
 * Copyright (C) 2002 Till
 * Copyright (C) 2005 Koos Vriezen <koos ! vriezen () xs4all ! nl>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

package org.kde.kjas.server;

import java.awt.Toolkit;
import java.awt.Image;
import java.awt.BorderLayout;
import java.awt.event.ActionListener;
import java.awt.event.ActionEvent;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Properties;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JButton;
import javax.swing.JTextArea;
import javax.swing.border.EmptyBorder;


public class KJASSwingConsole implements Console {
    private JFrame frame = null;
    private JPanel jPanel1;
    private JScrollPane jScrollPane1;
    private JButton clearButton;
    private JTextArea textField;
    private JButton closeButton;
    private JButton copyButton;
    final static int NR_BUFFERS = 3;
    final static int MAX_BUF_LENGTH = 3000;
    private int queue_pos = 0;
    private StringBuffer [] output_buffer = new StringBuffer[NR_BUFFERS];

    private PrintStream real_stderr = new PrintStream(System.err);
    
    /** Creates new form KJASSwingConsole */
    public KJASSwingConsole() {
        PrintStream st = new PrintStream( new KJASConsoleStream(this) );
        System.setOut(st);
        System.setErr(st);
    }
    
    private void initComponents() {
        frame = new JFrame("Konqueror Java Console");
        jPanel1 = new JPanel();
        clearButton = new JButton();
        closeButton = new JButton();
        copyButton = new JButton();
        jScrollPane1 = new JScrollPane();
        textField = new JTextArea();

        frame.setFont(new java.awt.Font("Monospaced", 0, 10));
        frame.setName("KJAS Console");
        frame.addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent evt) {
                exitForm(evt);
            }
        });

        jPanel1.setLayout(new BorderLayout());
        jPanel1.setBorder(new EmptyBorder(new java.awt.Insets(1, 1, 1, 1)));
        clearButton.setText("clear");
        clearButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                clearButtonActionPerformed(evt);
            }
        });

        jPanel1.add(clearButton, BorderLayout.WEST);

        closeButton.setText("close");
        closeButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                closeButtonActionPerformed(evt);
            }
        });

        jPanel1.add(closeButton, BorderLayout.EAST);

        copyButton.setText("copy");
        copyButton.addActionListener(new ActionListener() {
            public void actionPerformed(ActionEvent evt) {
                copyButtonActionPerformed(evt);
            }
        });

        jPanel1.add(copyButton, BorderLayout.CENTER);

        frame.getContentPane().add(jPanel1, BorderLayout.SOUTH);

        textField.setColumns(40);
        textField.setEditable(false);
        textField.setRows(10);
        textField.addKeyListener(new KeyAdapter() {
            public void keyPressed(KeyEvent evt) {
                textFieldKeyPressed(evt);
            }
        });

        jScrollPane1.setViewportView(textField);

        frame.getContentPane().add(jScrollPane1, BorderLayout.CENTER);

        try {
            java.net.URL iconUrl = getClass().getClassLoader().getResource("images/beanicon.png");
            if (iconUrl != null) {
                Toolkit tk = Toolkit.getDefaultToolkit();
                Image icon = tk.createImage(iconUrl);
                frame.setIconImage(icon);
            }
        } catch (Throwable e) {
        }
        frame.pack();
        frame.setSize(500, 300);
    }

    private void textFieldKeyPressed(java.awt.event.KeyEvent evt) {
        // Add your handling code here:
        char key = evt.getKeyChar();
        switch (key) {
            case 'h':
                showHelp();
                break;
            case 'g':
                append("Running Garbage Collection ...\n", true);
                System.gc();
            case 'm': 
                append("Total Memory: " + Runtime.getRuntime().totalMemory() + " bytes\n", true); 
                append("Free Memory : " + Runtime.getRuntime().freeMemory() + " bytes\n", true);
                break;
            case 'c':
                clear();
                break;
            case 's':
                showSystemProperties();
                break;
            case 't':
                showThreads();
                break;
            case 'x':
                KJASAppletClassLoader.removeLoaders();
                append("Emptied Classloader Cache\n", true);
                break;
        }
    }

    private void showHelp() {
        append("Java VM: " + System.getProperty("java.vendor") + " " + System.getProperty("java.version") + "\n", true);
        String ph = System.getProperty("http.proxyHost");
        if (ph != null) {
            append("Proxy: " + ph + ":" + System.getProperty("java.proxyPort") + "\n", true);
        }
        SecurityManager sec = System.getSecurityManager();
        if (sec == null) {
            append("WARNING: Security Manager disabled!\n", true);
        } else {
            append("SecurityManager=" + sec + "\n", true);
        }
        appendSeparator();
        append("Konqueror Java Console Help\n", true);
        append("  c: clear console\n", true);
        append("  g: run garbage collection\n", true);
        append("  h: show help\n", true);
        append("  m: show memory info\n", true);
        append("  s: print system properties\n", true);
        append("  t: list threads\n", true);
        append("  x: empty classloader cache\n", true);
        appendSeparator();
    }

    private void showSystemProperties() {
        append("Printing System Properties ...\n", true);
        appendSeparator();
        Properties p = System.getProperties();
        for (Enumeration e = p.keys(); e.hasMoreElements();) {
            Object key = e.nextElement();
            if ("line.separator".equals(key)) {
                String value = (String) p.get(key);
                StringBuffer unescaped = new StringBuffer(10);
                for (int i = 0; i < value.length(); i++) {
                    char c = value.charAt(i);
                    if (c == '\n') unescaped.append("\\n");
                    else if (c == '\r') unescaped.append("\\n");
                    else unescaped.append(c);
                }
                append(key + " = " + unescaped + "\n", true);
            } else append(key + " = " + p.get(key) + "\n", true);
        }
        appendSeparator();
    }

    private void showThreads() {
        Thread t = Thread.currentThread();
        ThreadGroup g = t.getThreadGroup();
        ThreadGroup parent;
        while ((parent = g.getParent()) != null) {
            g = parent;
        }
        g.list();
    }

    private void copyButtonActionPerformed(java.awt.event.ActionEvent evt) {
        textField.selectAll();
        textField.copy();
    }

    private void closeButtonActionPerformed(java.awt.event.ActionEvent evt) {
        frame.setVisible(false);
    }

    private void clearButtonActionPerformed(java.awt.event.ActionEvent evt) {
        clear();
    }
    
    /** Exit the Application */
    private void exitForm(java.awt.event.WindowEvent evt) {
        frame.setVisible(false);
    }

    public void setVisible(boolean visible) {
        if (frame == null && visible) {
            initComponents();
            frame.setVisible(visible);
            System.out.println( "Java VM version: " +
                    System.getProperty("java.version") );
            System.out.println( "Java VM vendor:  " +
                    System.getProperty("java.vendor") );
            String ph = System.getProperty("http.proxyHost");
            String pp = System.getProperty("http.proxyPort");
            if (ph != null) {
                System.out.println("Proxy: " + ph + ":" + pp);
            }
            SecurityManager sec = System.getSecurityManager();
            Main.debug("SecurityManager=" + sec);
            if (sec == null) {
                System.out.println( "WARNING: Security Manager disabled!" );
                textField.setForeground(java.awt.Color.red);
            }
            showHelp();
        } else if (frame != null)
            frame.setVisible(visible);

        if (visible) {
            for (int i = 0; i < NR_BUFFERS; i++)
                if (output_buffer[(queue_pos + i + 1) % 3] != null) {
                    textField.append(output_buffer[(queue_pos + i + 1) % 3].toString());
                    output_buffer[(queue_pos + i + 1) % 3] = null;
                }
        }
    }
    
    /**
     * @param args the command line arguments
     */
    public static void main(String args[]) {
        new KJASSwingConsole().setVisible(true);
    }
    
    public void clear() {
        textField.setText("");
    }

    private void appendSeparator() {
        append("----------------------------------------------------\n", true);
    }

    public void append(String txt) {
        append(txt, false);
    }

    public void append(String txt, boolean force) {
        if (txt == null)
            return;
        if (frame == null || !frame.isVisible()) {
            if (Main.Debug)
                real_stderr.print(txt);
            if (output_buffer[queue_pos] != null &&
                    output_buffer[queue_pos].length() > MAX_BUF_LENGTH) {
                queue_pos = (++queue_pos) % NR_BUFFERS;
                if (output_buffer[queue_pos] != null) {
                    // starting overwriting old log, clear textField if exists
                    if (frame != null)
                        textField.setText("");
                    output_buffer[queue_pos] = null;
                }
            }
            if (output_buffer[queue_pos] == null)
                output_buffer[queue_pos] = new StringBuffer(txt);
            else
                output_buffer[queue_pos].append(txt);
            return;
        }
        int length = txt.length();
        synchronized(textField) {
            //get the caret position, and then get the new position
            int old_pos = textField.getCaretPosition();
            textField.append(txt);
            textField.setCaretPosition( old_pos + length );
        }
    }
}

