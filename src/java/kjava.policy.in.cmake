grant codeBase "file:${KF5_DATA_INSTALL_DIR}/kjava/-"
{
    permission java.security.AllPermission;
};
grant {
	permission java.AudioPermission "play";
	permission java.lang.RuntimePermission "accessClassInPackage.sun.audio";
};
