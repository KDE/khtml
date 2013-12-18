package netscape.security;

public class PrivilegeManager extends Object {
    public static final int PROPER_SUBSET = 1;
    public static final int EQUAL = 2;
    public static final int NO_SUBSET = 3;
    public static final int SIGNED_APPLET_DBNAME = 4;
    public static final int TEMP_FILENAME = 5;
    
    private static PrivilegeManager thePrivilegeManager = null;

    protected PrivilegeManager() 
    {
    }
    public void checkPrivilegeEnabled(netscape.security.Target target) throws netscape.security.ForbiddenTargetException
    {
    }
    public void checkPrivilegeEnabled(netscape.security.Target target, Object o) throws netscape.security.ForbiddenTargetException
    {
    }
    public static void enablePrivilege(String privilegeString) throws netscape.security.ForbiddenTargetException
    {
    }
    public void enablePrivilege(netscape.security.Target target) throws netscape.security.ForbiddenTargetException
    {
    }
    public void enablePrivilege(netscape.security.Target target, netscape.security.Principal principal) throws netscape.security.ForbiddenTargetException
    {
    }
    public void enablePrivilege(netscape.security.Target target, netscape.security.Principal principal, Object o) throws netscape.security.ForbiddenTargetException
    {
    }
    public void revertPrivilege(netscape.security.Target target)
    {
    }
    public static void revertPrivilege(String privilegeString)
    {
    }
    public void disablePrivilege(netscape.security.Target target)
    {
    }
    public void disablePrivilege(String privilegeString)
    {
    }
    public static void checkPrivilegeGranted(String privilegeString) throws netscape.security.ForbiddenTargetException
    {
    }
    public void checkPrivilegeGranted(netscape.security.Target target) throws netscape.security.ForbiddenTargetException
    {
    }
    public void checkPrivilegeGranted(netscape.security.Target target, Object o) throws netscape.security.ForbiddenTargetException
    {
    }
    public void checkPrivilegeGranted(netscape.security.Target target, netscape.security.Principal principal, Object o) throws netscape.security.ForbiddenTargetException
    {
    }
    public boolean isCalledByPrincipal(netscape.security.Principal principal, int dontknow)
    {
        return false;
    }
    public boolean isCalledByPrincipal(netscape.security.Principal principal)
    {
        return false;
    }
    public static netscape.security.Principal getSystemPrincipal()
    {
        return null;    
    }
    public static netscape.security.PrivilegeManager getPrivilegeManager()
    {
        if (thePrivilegeManager == null) {
            thePrivilegeManager = new PrivilegeManager();
        }
        return thePrivilegeManager;
    }
    public boolean hasPrincipal(Class cl, netscape.security.Principal principal)
    {
        return true;
    }
    public int comparePrincipalArray(netscape.security.Principal[] a, netscape.security.Principal[] b)
    {
         return 1;
    }
    public boolean checkMatchPrincipal(Class cl, int dontknow)
    {
        return true;
    }
    public boolean checkMatchPrincipal(netscape.security.Principal principal, int dontknow)
    {
        return true;
    }
    public boolean checkMatchPrincipal(Class cl)
    {
        return true;
    }
    public boolean checkMatchPrincipalAlways()
    {
        return true;
    }
    public netscape.security.Principal[] getClassPrincipalsFromStack(int n)
    {
        return null;
    }
    /*
    public netscape.security.PrivilegeTable getPrivilegeTableFromStack();
    {
    }
    */
}
