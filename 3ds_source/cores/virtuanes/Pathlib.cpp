//
// �p�X���C�u�����N���X
//
#include "Pathlib.h"
#include "string.h"
#include <stdio.h>
#include <string.h>

// 3DS-compatible path splitting (replaces Windows _splitpath/_makepath)
static void _splitpath(const char* path, char* drive, char* dir, char* fname, char* ext) {
    if (drive) drive[0] = 0;
    if (dir) dir[0] = 0;
    if (fname) fname[0] = 0;
    if (ext) ext[0] = 0;
    if (!path) return;
    
    const char* lastSlash = strrchr(path, '/');
    const char* lastDot = lastSlash ? strrchr(lastSlash, '.') : strrchr(path, '.');
    
    // Drive - 3DS paths have device prefix like "sdmc:"
    const char* colon = strchr(path, ':');
    if (colon && drive) {
        size_t len = colon - path + 1;
        if (len < 16) { strncpy(drive, path, len); drive[len] = 0; }
    }
    
    // Dir
    if (lastSlash && dir) {
        size_t len = lastSlash - path + 1;
        if (len < 256) { strncpy(dir, path, len); dir[len] = 0; }
    }
    
    // Fname and Ext
    const char* base = lastSlash ? lastSlash + 1 : path;
    if (lastDot && lastDot > base) {
        // Has extension
        if (fname) {
            size_t len = lastDot - base;
            if (len < 256) { strncpy(fname, base, len); fname[len] = 0; }
        }
        if (ext) {
            strncpy(ext, lastDot, 256);
            ext[255] = 0;
        }
    } else {
        if (fname) { strncpy(fname, base, 256); fname[255] = 0; }
    }
}

static void _makepath(char* path, const char* drive, const char* dir, const char* fname, const char* ext) {
    if (!path) return;
    path[0] = 0;
    if (drive) strcat(path, drive);
    if (dir) strcat(path, dir);
    if (fname) strcat(path, fname);
    if (ext) strcat(path, ext);
}


string	CPathlib::SplitPath( LPCSTR lpszPath )
{
	CHAR	szDrive[ _MAX_DRIVE ];
	CHAR	szDir  [ _MAX_DIR ];
	CHAR	szTemp [ _MAX_PATH+1 ];
	string	path;
	::_splitpath( lpszPath, szDrive, szDir, NULL, NULL );
	::_makepath( szTemp, szDrive, szDir, NULL, NULL );
	path = szTemp;
	return	path;
}

string	CPathlib::SplitFname( LPCSTR lpszPath )
{
	CHAR	szTemp [ _MAX_PATH+1 ];
	string	fname;
	::_splitpath( lpszPath, NULL, NULL, szTemp, NULL );
	fname = szTemp;
	return	fname;
}

string	CPathlib::SplitFnameExt( LPCSTR lpszPath )
{
	CHAR	szFname[ _MAX_FNAME ];
	CHAR	szExt  [ _MAX_EXT ];
	CHAR	szTemp [ _MAX_PATH+1 ];
	string	fname;
	::_splitpath( lpszPath, NULL, NULL, szFname, szExt );
	::_makepath( szTemp, NULL, NULL, szFname, szExt );
	fname = szTemp;
	return	fname;
}

string	CPathlib::SplitExt( LPCSTR lpszPath )
{
	CHAR	szExt[ _MAX_EXT ];
//	CHAR	szTemp[ _MAX_PATH+1 ];
	string	fname;
	::_splitpath( lpszPath, NULL, NULL, NULL, szExt );
	fname = szExt;
	return	fname;
}

string	CPathlib::MakePath( LPCSTR lpszPath, LPCSTR lpszFname )
{
	CHAR	szDrive[ _MAX_DRIVE ];
	CHAR	szDir  [ _MAX_DIR ];
	CHAR	szTemp [ _MAX_PATH+1 ];
	string	path;
	::_splitpath( lpszPath, szDrive, szDir, NULL, NULL );
	::_makepath( szTemp, szDrive, szDir, lpszFname, NULL );
	path = szTemp;
	return	path;
}

string	CPathlib::MakePathExt( LPCSTR lpszPath, LPCSTR lpszFname, LPCSTR lpszExt )
{
	CHAR	szDrive[ _MAX_DRIVE ];
	CHAR	szDir  [ _MAX_DIR ];
	CHAR	szTemp [ _MAX_PATH+1 ];
	string	path;
	::_splitpath( lpszPath, szDrive, szDir, NULL, NULL );
	::_makepath( szTemp, szDrive, szDir, lpszFname, lpszExt );
	path = szTemp;
	return	path;
}

/*
string	CPathlib::CreatePath( LPCSTR lpszBasePath, LPCSTR lpszPath )
{
	CHAR	szPath[ _MAX_PATH ];
	string	path;
	if( ::PathIsRelative( lpszPath ) ) {
		CHAR	szTemp[ _MAX_PATH ];
		::strcpy( szTemp, lpszBasePath );
		::PathAppend( szTemp, lpszPath );
		::PathCanonicalize( szPath, szTemp );
	} else {
		::strcpy( szPath, lpszPath );
	}
	path = szPath;
	return	path;
}
*/

/*
INT CALLBACK	CPathlib::BffCallback( HWND hWnd, UINT uMsg, LPARAM lParam, WPARAM wParam )
{
	if( uMsg == BFFM_INITIALIZED && wParam ) {
		::SendMessage( hWnd, BFFM_SETSELECTION, TRUE, wParam );
	}
	return	TRUE;
}
*/

/*
BOOL	CPathlib::SelectFolder( HWND hWnd, LPCSTR lpszTitle, LPSTR lpszFolder )
{
	BROWSEINFO	bi;
	LPITEMIDLIST	pidl;

	ZeroMemory( &bi, sizeof(bi) );
	bi.hwndOwner = hWnd;
	bi.lpszTitle = lpszTitle;
	bi.ulFlags = BIF_RETURNONLYFSDIRS;
	// For Folder setup
	bi.lpfn = (BFFCALLBACK)BffCallback;

	// �Ō���'\'���t���Ă����ƃf�t�H���g�I�����Ă����Ȃ�(Win98)�̂�...
	if( lpszFolder ) {
		if( ::strlen(lpszFolder) > 3 ) {
			if( lpszFolder[::strlen(lpszFolder)-1] == '\\' )
				lpszFolder[::strlen(lpszFolder)-1] = NULL;
		}
		bi.lParam = (LPARAM)lpszFolder;
	} else {
		bi.lParam = NULL;
	}

	string	path;
	if( (pidl = ::SHBrowseForFolder( &bi )) ) {
		path.resize( _MAX_PATH+1 );
		::SHGetPathFromIDList( pidl, lpszFolder );
		if( ::strlen(lpszFolder) > 3 ) {	// �h���C�u���̏ꍇ������
			::strcat( lpszFolder, "\\" );
		}
		IMalloc* pMalloc;
		::SHGetMalloc( &pMalloc );
		if( pMalloc ) {
			pMalloc->Free( pidl );
			pMalloc->Release();
		}
		return	TRUE;
	}

	return	FALSE;
}

*/