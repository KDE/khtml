/* This file is part of the KDE project
 *
 * Copyright (C) 2003 Koos Vriezen <koos ! vriezen () xs4all ! nl>
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

import java.net.*;
import java.io.*;
import java.util.*;
import java.security.*;
/**
 *
 */

class KIOConnection
{
    final static int NOT_CONNECTED = 0;
    final static int CONNECT_WAIT = 1;
    final static int CONNECTED = 2;

    final static int DATA = 0;
    final static int FINISHED = 1;
    final static int ERRORCODE = 2;
    final static int CONNECT = 6;
    final static int REQUESTDATA = 7;

    final static int STOP = 0;
    final static int HOLD = 1;
    final static int RESUME = 2;

    protected static int id = 0;
    static Hashtable jobs = new Hashtable();     // should be thread safe

    static void setData(String jobid, int code, byte [] data) {
        KIOConnection job = (KIOConnection) jobs.get(jobid);
        if (job == null || !job.setData(code, data))
            Main.info("KIO KJASHttpURLConnection gone (timedout/closed)");
        else
            Thread.yield();
    }

    private class KJASOutputStream extends OutputStream {
        KJASOutputStream() {
        }
        public void write(int b) throws IOException {
            byte[] buf = {(byte)b};
            write(buf);
        }
        public synchronized void write(byte b[], int off, int len) throws IOException {
            byte[] buf = new byte[len];
            System.arraycopy(b, off, buf, 0, len);
            sendData(buf, false);
        }
        public void write(byte b[]) throws IOException {
            write(b, 0, b.length);
        }
        public void close() throws IOException {
            disconnect();
        }
        public void flush() throws IOException {
            checkConnected();
            sendData(null, true);
        }
    }

    private class KJASInputStream extends InputStream {

        KJASInputStream() {
        }
        public int read() throws IOException {
            if (getData(true))
                return 0x00ff & in_buf[in_bufpos++];
            return -1;
        }
        public int read(byte[] b, int off, int len) throws IOException {
            int total = 0;
            do {
                if (!getData(true)) break;
                int nr = in_buf.length - in_bufpos;
                if (nr > len)
                    nr = len;
                System.arraycopy(in_buf, in_bufpos, b, off, nr);
                len -= nr;
                total += nr;
                off += nr;
                in_bufpos += nr;
            } while (len > 0);
            return total > 0 ? total : -1;
        }
        public int read(byte[] b) throws IOException {
            return read(b, 0, b.length);
        }
        public int available() throws IOException {
            return inAvailable();
        }
        public boolean markSupported() {
            return false;
        }
        public void close() throws IOException {
            disconnect();
        }
    }

    protected URL url;
    protected int connect_status = 0;
    protected String jobid = null;                // connection id with KIO 
    protected LinkedList data = new LinkedList(); // not thread safe
    protected int errorcode = 0;
    protected boolean finished = false;           // all data has arived
    protected boolean onhold = false;             // KIO job is suspended
    protected boolean request_data = false;       // need data for put job
    private KJASOutputStream out = null;
    private KJASInputStream in = null;
    private byte [] in_buf = null;                // current input buffer
    private int in_bufpos = 0;                    // position in buffer
    private boolean in_eof = false;               // all data is read
    private final int LOW_BUFFER_LIMIT = 5;       // put onhold off
    private final int HIGH_BUFFER_LIMIT = 10;     // put onhold on

    protected KIOConnection(URL u) {
        url = u;
    }
    protected void checkConnected() throws IOException {
        if (connect_status != CONNECTED)
            throw new IOException("not connected");
    }
    protected boolean haveError() {
        return errorcode != 0;
    }
    synchronized protected boolean setData(int code, byte [] d) {
        // is job still there when entering the monitor
        if (jobs.get(jobid) == null) 
            return false;
        if (connect_status == CONNECT_WAIT)
            connect_status = CONNECTED;
        switch (code) {
            case FINISHED:
                if (d != null && d.length > 0)
                    data.addLast(d);
                finished = true;
                onhold = false;
                jobs.remove(jobid);
                Main.debug ("KIO FINISHED (" + jobid + ") " + data.size());
                break;
            case DATA:
                if (d.length > 0)
                    data.addLast(d);
                // Main.debug ("KIO DATA (" + jobid + ") " + data.size());
                if (!onhold && data.size() > HIGH_BUFFER_LIMIT) {
                    Main.protocol.sendDataCmd(jobid, HOLD);
                    onhold = true;
                }
                break;
            case ERRORCODE:
                String codestr = new String(d);
                errorcode = Integer.parseInt(codestr);
                Main.debug ("KIO ERRORECODE(" + jobid + ") " + errorcode);
                break;
            case CONNECT:
                Main.debug ("KIO CONNECT(" + jobid + ") ");
                request_data = true;
                errorcode = 0;
                break;
            case REQUESTDATA:
                Main.debug ("KIO REQUESTDATA(" + jobid + ") ");
                request_data = true;
                break;
        }
        notifyAll();
        return true;
    }

    private synchronized boolean getData(boolean mayblock) throws IOException {
        if (haveError()) {
            //disconnect();
            in_eof = true;
            //throw new IOException("i/o error " + errorcode);
        }
        if (in_eof)
            return false;
        checkConnected();
        if (in_buf != null && in_bufpos < in_buf.length)
            return true;
        int datasize = data.size();
        if (datasize > 0) {
            in_buf = (byte []) data.removeFirst();
            in_bufpos = 0;
        }
        if (onhold && datasize < LOW_BUFFER_LIMIT) {
            Main.protocol.sendDataCmd(jobid, RESUME);
            onhold = false;
        }
        if (datasize > 0)
            return true;
        if (finished) {
            in_eof = true;
            return false;
        }
        if (!mayblock)
            return false;
        try {
            wait();
        } catch (InterruptedException ie) {
            return false;
        }
        return getData(false);
    }
    synchronized private int inAvailable() throws IOException {
        if (in_eof)
            return 0;
        checkConnected();
        if (!getData(false))
            return 0;
        int total = in_buf.length - in_bufpos;
        ListIterator it = data.listIterator(0);
        while (it.hasNext())
            total += ((byte []) it.next()).length;
        return total;
    }
    synchronized private void sendData(byte [] d, boolean force) throws IOException {
        Main.debug ("KIO sendData(" + jobid + ") force:" + force + " request_data:" + request_data);
        if (d != null)
            data.addLast(d);
        if (!request_data && !force) return;
        if (data.size() == 0) return;
        if (force && !request_data) {
            try {
                wait(10000);
            } catch (InterruptedException ie) {
                return;
            }
            if (!request_data) {
                Main.debug ("KIO sendData(" + jobid + ") timeout");
                data.clear();
                disconnect();
                throw new IOException("timeout");
            }
        }
        byte[] buf;
        int total = 0;
        ListIterator it = data.listIterator(0);
        while (it.hasNext())
            total += ((byte []) it.next()).length;
        buf = new byte[total];
        int off = 0;
        it = data.listIterator(0);
        while (it.hasNext()) {
            byte [] b = (byte []) it.next();
            System.arraycopy(b, 0, buf, off, b.length);
            off += b.length;
        }
        data.clear();
        request_data = false;
        Main.protocol.sendPutData(jobid, buf, 0, total);
    }
    synchronized void connect(boolean doInput) throws IOException {
        if (connect_status == CONNECTED)
            return; // javadocs: call is ignored
	//(new Exception()).printStackTrace();
        Main.debug ("KIO connect " + url);
        errorcode = 0;
        finished = in_eof = false;
        jobid = String.valueOf(id++);
        jobs.put(jobid, this);
        if (doInput)
            Main.protocol.sendGetURLDataCmd(jobid, url.toExternalForm());
        else
            Main.protocol.sendPutURLDataCmd(jobid, url.toExternalForm());
        connect_status = CONNECT_WAIT;
        try {
            wait(20000);
        } catch (InterruptedException ie) {
            errorcode = -1;
        }
        boolean isconnected = (connect_status == CONNECTED);
        if (isconnected && !haveError()) {
            if (doInput)
                in = new KJASInputStream();
            else
                out = new KJASOutputStream();
            Main.debug ("KIO connect(" + jobid + ") " + url);
            return;
        }
        connect_status = NOT_CONNECTED;
        jobs.remove(jobid);
        if (isconnected) {
            if (!finished)
                Main.protocol.sendDataCmd(jobid, STOP);
            Main.debug ("KIO connect error " + url);
            throw new ConnectException("connection failed (not found)");
        }
        Main.debug ("KIO connect timeout " + url);
        throw new IOException("connection failed (timeout)");
    }
    synchronized void disconnect() {
        if (connect_status == NOT_CONNECTED)
            return;
        Main.debug ("KIO disconnect " + jobid);
	//(new Exception()).printStackTrace();
        if (out != null) {
            try {
                out.flush();
            } catch (IOException iox) {}
        }
        connect_status = NOT_CONNECTED;
        out = null;
        in = null;
        if (!finished) {
            Main.protocol.sendDataCmd(jobid, STOP);
            jobs.remove(jobid);
        }
        notifyAll();
    }
    InputStream getInputStream() throws IOException {
        Main.debug ("KIO getInputStream(" + jobid + ") " + url);
        return in;
    }
    OutputStream getOutputStream() throws IOException {
        Main.debug ("KIO getOutputStream(" + jobid + ") " + url);
        return out;
    }
}

final class KIOHttpConnection extends KIOConnection
{
    final static int HEADERS = 3;
    final static int REDIRECT = 4;
    final static int MIMETYPE = 5;

    Vector headers = new Vector();
    Hashtable headersmap = new Hashtable();
    String responseMessage = null;
    int responseCode = -1;

    KIOHttpConnection(URL u) {
        super(u);
    }
    protected boolean haveError() {
        return super.haveError() ||
            responseCode != 404 && (responseCode < 0 || responseCode >= 400);
    }
    protected synchronized boolean setData(int code, byte [] d) {
        switch (code) {
            case HEADERS:
                StringTokenizer tokenizer = new StringTokenizer(new String(d), "\n");
                while (tokenizer.hasMoreTokens()) {
                    String token = tokenizer.nextToken();
                    int pos = token.indexOf(":");
                    String [] entry = {
                        token.substring(0, pos > -1 ? pos : token.length()).toLowerCase(), token.substring(pos > -1 ? pos+1: token.length()).trim()
                    };
                    headers.add(entry);
                    headersmap.put(entry[0], entry[1]);
                    // Main.debug ("KIO header " + entry[0] + "=" + entry[1]);
                }
                responseCode = 0;
                if (headersmap.size() > 0) {
                    String token = ((String []) headers.get(0))[0];
                    if (!token.startsWith("http/1.")) break;
                    int spos = token.indexOf(' ');
                    if (spos < 0) break;
                    int epos = token.indexOf(' ', spos + 1);
                    if (epos < 0) break;
                    responseCode = Integer.parseInt(token.substring(spos+1, epos));
                    responseMessage = token.substring(epos+1);
                    Main.debug ("KIO responsecode=" + responseCode);
                }
                break;
        }
        return super.setData(code, d);
    }
}

final class KIOSimpleConnection extends KIOConnection
{
    KIOSimpleConnection(URL u) {
        super(u);
    }
}

final class KJASHttpURLConnection extends HttpURLConnection
{
    private KIOHttpConnection kioconnection;

    KJASHttpURLConnection(URL u) {
        super(u);
        kioconnection = new KIOHttpConnection(u);
    }
    public Map getHeaderFields() {
	try {
            connect();
	} catch (IOException e) {
            Main.debug ("Error on implicit connect()");
	}
        Main.debug ("KIO getHeaderFields");
        return kioconnection.headersmap;
    }
    public String getHeaderField(String name) {
	try {
            connect();
	} catch (IOException e) {
            Main.debug ("Error on implicit connect()");
	}
        String field = (String) kioconnection.headersmap.get(name);
        Main.debug ("KIO getHeaderField:" + name + "=" + field);
	//(new Exception()).printStackTrace();
        return field;
    }
    public String getHeaderField(int n) {
	try {
            connect();
	} catch (IOException e) {
            Main.debug ("Error on implicit connect()");
	}
        Main.debug ("KIO getHeaderField(" + n + ") size=" + kioconnection.headersmap.size());
        if (n >= kioconnection.headersmap.size())
            return null;
        String [] entry = (String []) kioconnection.headers.get(n);
        String line = entry[0];
        if (entry[1].length() > 0)
            line += ":" + entry[1];
        Main.debug ("KIO getHeaderField(" + n + ")=#" + line + "#");
        return line;
    }
    public String getHeaderFieldKey(int n) {
	try {
            connect();
	} catch (IOException e) {
            Main.debug ("Error on implicit connect()");
	}
        Main.debug ("KIO getHeaderFieldKey " + n);
        if (n >= kioconnection.headersmap.size())
            return null;
        return ((String []) kioconnection.headers.get(n))[0];
    }
    public int getResponseCode() throws IOException {
        Main.debug ("KIO getResponseCode");
        if (kioconnection.responseCode == -1) {
            try {
                connect();
            } catch (IOException e) {
                if (kioconnection.responseCode == -1)
                    throw e;
            }
        }
        responseMessage = kioconnection.responseMessage;
        return kioconnection.responseCode;
    }
    public boolean usingProxy() {
        return false; // FIXME
    }
    public void connect() throws IOException {
        if (connected)
            return;
        Main.debug ("KIO KJASHttpURLConnection.connect " + url);
        SecurityManager security = System.getSecurityManager();
        if (security != null)
            security.checkPermission(getPermission());
        kioconnection.connect(doInput);
        connected = true;
        if (kioconnection.responseCode == 404)
            throw new FileNotFoundException(url.toExternalForm());
    }
    public void disconnect() {
        kioconnection.disconnect();
        connected = false;
    }
    public InputStream getInputStream() throws IOException {
        doInput = true;
        doOutput = false;
        connect();
        return kioconnection.getInputStream();
    }
    public OutputStream getOutputStream() throws IOException {
        doInput = false;
        doOutput = true;
        connect();
        return kioconnection.getOutputStream();
    }
    public InputStream getErrorStream() {
        Main.debug("KIO KJASHttpURLConnection.getErrorStream" + url);
        try {
            if (connected && kioconnection.responseCode == 404)
                return kioconnection.getInputStream();
        } catch (Exception ex) {}
        return null;
    }
}

final class KJASSimpleURLConnection extends URLConnection
{
    private KIOSimpleConnection kioconnection = null;
    private int default_port;

    KJASSimpleURLConnection(URL u, int p) {
        super(u);
        default_port = p;
    }
    public boolean usingProxy() {
        return false; // FIXME
    }
    public Permission getPermission() throws IOException {
        int p = url.getPort();
        if (p < 0)
            p = default_port;
        return new SocketPermission(url.getHost() + ":" + p, "connect");
    }
    public void connect() throws IOException {
        if (kioconnection != null)
            return;
        Main.debug ("KIO KJASSimpleURLConnection.connection " + url);
        SecurityManager security = System.getSecurityManager();
        if (security != null)
            security.checkPermission(getPermission());
        kioconnection = new KIOSimpleConnection(url);
        kioconnection.connect(doInput);
        connected = true;
    }
    public void disconnect() {
        if (kioconnection == null)
            return;
        kioconnection.disconnect();
        kioconnection = null;
        connected = false;
    }
    public InputStream getInputStream() throws IOException {
        doInput = true;
        doOutput = false;
        if (kioconnection == null)
            connect();
        return kioconnection.getInputStream();
    }
    public OutputStream getOutputStream() throws IOException {
        doInput = false;
        doOutput = true;
        if (kioconnection == null)
            connect();
        return kioconnection.getOutputStream();
    }
}


final class KJASHttpURLStreamHandler extends URLStreamHandler
{
    KJASHttpURLStreamHandler(int port) {
        default_port = port;
    }
    protected URLConnection openConnection(URL u) throws IOException {
        URL url = new URL(u.toExternalForm());
        return new KJASHttpURLConnection(url);
    }
    protected int getDefaultPort() {
        return default_port;
    }
    private int default_port;
}

final class KJASSimpleURLStreamHandler extends URLStreamHandler
{
    KJASSimpleURLStreamHandler(int port) {
        default_port = port;
    }
    protected URLConnection openConnection(URL u) throws IOException {
        URL url = new URL(u.toExternalForm());
        return new KJASSimpleURLConnection(url, default_port);
    }
    protected int getDefaultPort() {
        return default_port;
    }
    private int default_port;
}

public final class KJASURLStreamHandlerFactory 
    implements URLStreamHandlerFactory
{
    public URLStreamHandler createURLStreamHandler(String protocol) {
        if (protocol.equals("jar") || protocol.equals("file"))
            return null;
        //outputs to early: Main.debug ("createURLStreamHandler " + protocol);
        Main.debug ("KIO createURLStreamHandler " + protocol);
        if (protocol.equals("http"))
            return new KJASHttpURLStreamHandler(80);
        else if (protocol.equals("https"))
            return new KJASHttpURLStreamHandler(443);
        else if (protocol.equals("ftp"))
            return new KJASSimpleURLStreamHandler(21);
        else if (protocol.equals("smb"))
            return new KJASSimpleURLStreamHandler(139);
        else if (protocol.equals("fish"))
            return new KJASSimpleURLStreamHandler(22);
        return null;
    }
}
