#define XPL_NO_WRAP 1

#include <xplfile.h>
#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <unistd.h>

#if defined(WINDOWS)
#include<sys/utime.h>
#else
#include<utime.h>
#endif

EXPORT int XplExpandEnv(char *dest, const char *src, size_t size)
{
#ifdef WINDOWS
	int		r;

	if ((r = ExpandEnvironmentStrings(src, dest, size)) > 0) {
		if (r <= size) {
			return(0);
		} else {
			/* The output buffer wasn't large enough */
			return(-1);
		}
	}

	return(-2);
#else
	return(-1);
#endif
}

EXPORT FILE *XplWrapFOpen(const char *path, const char *mode)
{
	char		expath[XPL_MAX_PATH];
#ifdef WINDOWS
	char		exmode[32];

	/*
		Windows makes FILE *'s inheritable by default. This means that any open
		FILE * will be inherited by all child processes. This is a problem since
		windows does not allow renaming, unlinking, etc if a file is still open.

		Passing N in the mode string to fopen will mark the file as not
		inheritable, which is our desired default behavior.

		Pass N as the first argument after the mode itself. This is important
		to prevent messing up charset specifications.

		Windows also disables commits by default, meaning that calling fflush on
		an open file will NOT flush it. We would like fflush to work, so add the
		c flag as well.
	*/
	char	*out;

	out = exmode;
	*out = '\0';

	while ((isalpha(*mode) || '+' == *mode) && (out < exmode + sizeof(exmode) - 2)) {
		switch (*mode) {
			case 'N':
			case 'c': /* Do nothing; we are adding these at the end */
				break;

			default:
				*out = *mode;
				out++;
				break;
		}

		mode++;
	}
	*out = '\0';

	/* Append the N and any remaining characters as is */
	strcatf(exmode, sizeof(exmode), NULL, "Nc%s", mode);
	mode = (const char *) exmode;
#endif

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(fopen(path, mode));
}

EXPORT FILE *XplWrapFOpen64(const char *path, const char *mode)
{
	char		expath[XPL_MAX_PATH];
#ifdef WINDOWS
	char		exmode[32];

	/*
		Windows makes FILE *'s inheritable by default. This means that any open
		FILE * will be inherited by all child processes. This is a problem since
		windows does not allow renaming, unlinking, etc if a file is still open.

		Passing N in the mode string to fopen will mark the file as not
		inheritable, which is our desired default behavior.
	*/
	if (!strchr(mode, 'N')) {
		strprintf(exmode, sizeof(exmode), NULL, "%sN", mode);
		mode = (const char *) exmode;
	}
#endif

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

#ifdef LINUX
	return(fopen64(path, mode));
#else
	return(fopen(path, mode));
#endif
}

EXPORT FILE *XplWrapFReopen(const char *path, const char *mode, FILE *stream)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(freopen(path, mode, stream));
}

EXPORT FILE *XplWrapFReopen64(const char *path, const char *mode, FILE *stream)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

#ifdef LINUX
	return(freopen64(path, mode, stream));
#else
	return(freopen(path, mode, stream));
#endif
}

EXPORT int XplWrapCreat(const char *path, XPL_WRAP_MODE_TYPE mode)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(creat(path, mode));
}

EXPORT int XplWrapOpen(const char *path, int flags, ...)
{
	XPL_WRAP_MODE_TYPE		mode;
	va_list					args;
	char					expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	/*
		The open function takes an optional mode argument based on the flags
		that are passed in. Pull off a mode int even if it won't be used.
	*/
	va_start(args, flags);
	mode = va_arg(args, int);
	va_end(args);

	return(open(path, flags, mode));
}

EXPORT int XplWrapAccess(const char *path, int mode)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(access(path, mode));
}

/*
  stat() fails on windows if the path has a trailing slash.
  This code makes stat( "<path>", &st ) and stat( "<path>/", &st )
  behave identically.
*/

static int fixedStat( const char *path, struct stat *st )
{
	int ret;
#ifdef WIN32
	char *pathPtr;
	char buffer[ XPL_MAX_PATH ];
	size_t len;

	len = strlen( path );
	if( ( path[ len - 1 ] != '/' ) && ( path[ len - 1 ] != '\\' ) ) {
		ret = stat( path, st );
	} else if( len < sizeof( buffer ) ) {
		strcpy( buffer, path );
		buffer[ len - 1 ] = '\0';
		ret = stat( buffer, st );
	} else {
		pathPtr = MemStrdupWait( path );
		pathPtr[ len - 1 ] = '\0';
		ret = stat( pathPtr, st );
		MemFree( pathPtr );
	}
#else
	ret = stat( path, st );
#endif
	return ret;
}

EXPORT int XplWrapStat(const char *path, struct stat *buf)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(fixedStat(path, buf));
}

EXPORT int XplWrapSetMTime(const char *path, uint32 mtime)
{
	char expath[XPL_MAX_PATH];
	struct utimbuf tb;

	tb.actime = ( time_t )mtime;
	tb.modtime = ( time_t )mtime;

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(utime(path, &tb));
}

EXPORT int XplWrapChdir(const char *path)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(chdir(path));
}

EXPORT int XplWrapChmod(const char *path, int pmode)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(chmod(path, pmode));
}

EXPORT int XplWrapUnlink(const char *path)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(unlink(path));
}

EXPORT int XplWrapRename(const char *oldpath, const char *newpath)
{
	char		exoldpath[XPL_MAX_PATH];
	char		exnewpath[XPL_MAX_PATH];

	if (!XplExpandEnv(exoldpath, oldpath, sizeof(exoldpath))) {
		oldpath = (const char *) exoldpath;
	}


	if (!XplExpandEnv(exnewpath, newpath, sizeof(exnewpath))) {
		newpath = (const char *) exnewpath;
	}

	return(rename(oldpath, newpath));
}

#ifdef WINDOWS
EXPORT int XplWrapMkDir(const char *path)
#else
EXPORT int XplWrapMkDir(const char *path, XPL_WRAP_MODE_TYPE mode)
#endif
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

#ifdef WINDOWS
	return(mkdir(path));
#else
	return(mkdir(path, mode));
#endif
}

EXPORT int XplWrapRmDir(const char *path)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

#ifdef WINDOWS
	return(_rmdir(path));
#else
	return(rmdir(path));
#endif
}

#ifdef WINDOWS
EXPORT XplBool XplSetCurrentDirectoryA(const char *path)
{
	char		expath[XPL_MAX_PATH];

	if (!XplExpandEnv(expath, path, sizeof(expath))) {
		path = (const char *) expath;
	}

	return(SetCurrentDirectoryA(path));
}
#endif

