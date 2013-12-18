package netscape.security;
public class ForbiddenTargetException extends RuntimeException {
    public ForbiddenTargetException()
    {
    }
    public ForbiddenTargetException(String message)
    {
        super(message);
    }
}
