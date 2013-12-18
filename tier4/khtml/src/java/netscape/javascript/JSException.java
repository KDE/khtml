package netscape.javascript;

public class JSException extends Exception {
    public JSException() {}
    public JSException(String s) {
        super(s);
    }
    public JSException(String s, String fn, int ln, String src, int ti) {
        super(s);
        filename = new String(fn);
        linenumber = ln;
        source = src;
        tokenindex = ti;
    }
    private String filename = null;
    private int linenumber;
    private String source = null;
    private int tokenindex;
}
 
