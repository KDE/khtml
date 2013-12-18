package netscape.javascript;

import java.applet.Applet;

public abstract class JSObject extends Object {
    protected JSObject()
    {
    }
    public abstract Object call(String function, Object[] arguments) throws JSException;
    public abstract Object eval(String script) throws JSException;
    public abstract Object getMember(String name) throws JSException;
    public abstract void setMember(String name, Object o) throws JSException;
    public abstract void removeMember(String name) throws JSException;
    public abstract Object getSlot(int index) throws JSException;
    public abstract void setSlot(int index, Object o) throws JSException;
    public static JSObject getWindow(Applet applet) throws JSException
    {
        return org.kde.javascript.JSObject.getWindow(applet, 0);
    }
}

