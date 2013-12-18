package org.kde.kjas.server;

import java.applet.Applet;
import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.LayoutManager;
import java.awt.Panel;
import java.net.URL;

/**
 * @author till
 *
 * A panel which embeds the applet and shows some
 * information during class loading.
 */
public class KJASAppletPanel extends javax.swing.JPanel implements StatusListener {
    private final int LOADING = 1;
    private final int RUNNING = 2;
    private final int FAILED = 3;

    private Image load_img = null;
    private Image fail_img = null;
    private int status = LOADING;
	private Font font;
	private String msg = "Loading Applet...";
	
	/**
	 * Constructor for KJASAppletPanel.
	 */
	public KJASAppletPanel() {
		super(new BorderLayout());
		font = new Font("SansSerif", Font.PLAIN, 10);
		URL url =
			getClass().getClassLoader().getResource("images/animbean.gif");
		load_img = getToolkit().createImage(url);
		//setBackground(Color.white);
	}

	void setApplet(Applet applet) {
		add("Center", applet);
		validate();
	}

    public void showStatus(String msg) {
        this.msg = msg;
        if (status != RUNNING)
            repaint();
    }

	public void paint(Graphics g) {
		super.paint(g);
        if (status == RUNNING)
            return;
        Image img = (status == LOADING ? load_img : fail_img);
        int x = getWidth() / 2;
        int y = getHeight() / 2;
        if (img != null) {
            //synchronized (img) {
            int w = img.getWidth(this);
            int h = img.getHeight(this);
            int imgx = x - w / 2;
            int imgy = y - h / 2;
            //g.setClip(imgx, imgy, w, h);
            g.drawImage(img, imgx, imgy, this);
            y += img.getHeight(this) / 2;
            //}
        }
        if (msg != null) {
            //synchronized(msg) {
            g.setFont(font);
            FontMetrics m = g.getFontMetrics();
            int h = m.getHeight();
            int w = m.stringWidth(msg);
            int msgx = x - w / 2;
            int msgy = y + h;
            //g.setClip(0, y, getWidth(), h);
            g.drawString(msg, msgx, msgy);
            //}
        }
	}
	void showFailed() {
		URL url =
			getClass().getClassLoader().getResource("images/brokenbean.gif");
		fail_img = getToolkit().createImage(url);
        status = FAILED;
		msg = "Applet Failed.";
		repaint();
	}

	void showFailed(String message) {
		showFailed();
		showStatus(message);
	}

	public void stopAnimation() {
        status = RUNNING;
	}

    public boolean imageUpdate(Image img, int flags, int x, int y, int w, int h)
    {
        if (img != null && img == load_img && status != LOADING) {
            img.flush();
            load_img = null;
            Main.debug("flushing image");
            return false;
        }
        return super.imageUpdate(img, flags, x, y, w, h);
    }
}
