/*
 * Appendable.java
 *
 * Created on 16. Mai 2002, 23:23
 */

package org.kde.kjas.server;

/**
 *
 * @author  till
 */
public interface Console {
    
    public void clear();
    public void append(String text);
    
    public void setVisible(boolean visible);
    
}
