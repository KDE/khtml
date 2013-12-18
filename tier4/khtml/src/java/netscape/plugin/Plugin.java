package netscape.plugin;

import netscape.javascript.*;

public class Plugin {

    public Plugin() {
        System.out.println("Plugin.Plugin");
    }
    public JSObject getWindow() throws JSException {
        System.out.println("Plugin.getWindow");
        return JSObject.getWindow(null);
    }
    public void destroy() {
        System.out.println("Plugin.destroy");
    }
    public int getPeer() {
        System.out.println("Plugin.getPeer");
        return 0;
    }
    public void init() {
        System.out.println("Plugin.init");
    }
    public boolean isActive() {
        System.out.println("Plugin.isActive");
        return true;
    }
}
