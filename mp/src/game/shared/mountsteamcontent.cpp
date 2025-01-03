//========== Copyleft � 2011, Team Sandbox, Some rights reserved. ===========//
//
// Purpose:
//
//===========================================================================//

#include "cbase.h"
#include "filesystem.h"
#include "KeyValues.h"
#include "mountsteamcontent.h"
// Andrew; grab only what we need from Open Steamworks.
// #include "SteamTypes.h"
// #include "ISteam006.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool Steam_MountSteamContent( int nExtraAppId )
{
#if 0
	CreateInterfaceFn CreateInterface = Sys_GetFactory( "Steam.dll" );
	if( !CreateInterface )
		return false;

	int nStatus = 0;

	IAppSystem* pAppSystem = (IAppSystem *)CreateInterface( "SteamDLLAppsystem001", &nStatus );
	if( !pAppSystem )
		return false;

	ISteam006* pSteam006 = (ISteam006 *)pAppSystem->QueryInterface( STEAM_INTERFACE_VERSION_006 );
	if( !pSteam006 )
		return false;

	TSteamApp App;
	App.szName = new char[ 255 ];
	App.uMaxNameChars = 255;
	App.szLatestVersionLabel = new char[ 255 ];
	App.uMaxLatestVersionLabelChars = 255;
	App.szCurrentVersionLabel = new char[ 255 ];
	App.uMaxCurrentVersionLabelChars = 255;
	App.szInstallDirName = new char[ 255 ];
	App.uMaxInstallDirNameChars = 255;
	App.szUnkString = new char[ 255 ];

	TSteamError Error;

	if( pSteam006->EnumerateApp( nExtraAppId, &App, &Error ) != 1 || Error.eSteamError != eSteamErrorNone )
	{
		delete[] App.szName;
		delete[] App.szLatestVersionLabel;
		delete[] App.szCurrentVersionLabel;
		delete[] App.szInstallDirName;
		delete[] App.szUnkString;

		return false;
	}

	int bIsAppSubscribed = 0;
	int Reserved = 0;
	pSteam006->IsAppSubscribed( nExtraAppId, &bIsAppSubscribed, &Reserved, &Error );

	if ( !bIsAppSubscribed )
	{
		return false;
	}

#ifdef GAME_DLL
	Msg( "Mounting %s...\n", App.szName );
#endif

	for( unsigned int n = 0; n < App.uNumDependencies; n++ )
	{
		TSteamAppDependencyInfo Info;

		if( pSteam006->EnumerateAppDependency( nExtraAppId, n, &Info, &Error ) != 1 || Error.eSteamError != eSteamErrorNone )
			continue;

		if( !pSteam006->MountFilesystem( Info.AppId, Info.szMountName, &Error ) || Error.eSteamError != eSteamErrorNone )
		{
#ifdef GAME_DLL
			Warning( "%s\n", Error.szDesc );
#endif
		}
	}

	delete[] App.szName;
	delete[] App.szLatestVersionLabel;
	delete[] App.szCurrentVersionLabel;
	delete[] App.szInstallDirName;
	delete[] App.szUnkString;
#endif

	return true;
}

typedef struct
{
	const char *m_pPathName;
	int m_nAppId;
} gamePaths_t;
gamePaths_t g_GamePaths[10] =
{
	{ "hl2",		220 },
	{ "cstrike",	240 },
	{ "hl1",		280 },
	{ "dod",		300 },
	{ "lostcoast",	340 },
	{ "hl1mp",		360 },
	{ "episodic",	380 },
	{ "portal",		400 },
	{ "ep2",		420 },
	{ "tf",			440 }
};

void AddSearchPathByAppId( int nAppId )
{
	for ( int i=0; i < ARRAYSIZE( g_GamePaths ); i++ )
	{
		int iVal = g_GamePaths[i].m_nAppId;
		if ( iVal == 360 )
		{
			//Andrew; Half-Life Deathmatch: Source requires Half-Life: Source's path added!!
			const char *pathName = g_GamePaths[2].m_pPathName;
			filesystem->AddSearchPath( pathName, "GAME", PATH_ADD_TO_TAIL );
		}
		if ( iVal == nAppId )
		{
			const char *pathName = g_GamePaths[i].m_pPathName;
			filesystem->AddSearchPath( pathName, "GAME", PATH_ADD_TO_TAIL );
		}
	}
}

//Andrew; this allows us to mount content the user wants on top of the existing
//game content which is automatically loaded by the engine, and then by the
//game code
void MountUserContent()
{
	KeyValues *pMainFile, *pFileSystemInfo;
#ifdef CLIENT_DLL
	const char *gamePath = engine->GetGameDirectory();
#else
	char gamePath[256];
	engine->GetGameDir( gamePath, 256 );
	Q_StripTrailingSlash( gamePath );
#endif

	pMainFile = new KeyValues( "gamecontent.txt" );
#ifdef CLIENT_DLL
#define UTIL_VarArgs VarArgs //Andrew; yep.
#endif
//On linux because of case sensitiviy we need to check for both.
#ifdef _LINUX
	if ( pMainFile->LoadFromFile( filesystem, UTIL_VarArgs("%s/GameContent.txt", gamePath), "MOD" ) )
	{
		pFileSystemInfo = pMainFile->FindKey( "FileSystem" );
		if (pFileSystemInfo)
		{
			for ( KeyValues *pKey = pFileSystemInfo->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
			{
				if ( strcmp( pKey->GetName(), "AppId" ) == 0 )
				{
					int nExtraContentId = pKey->GetInt();
					if (nExtraContentId)
					{
						AddSearchPathByAppId( nExtraContentId );
						Steam_MountSteamContent( nExtraContentId );
					}
				}
			}
		}
	}
	else
#endif
	if ( pMainFile->LoadFromFile( filesystem, UTIL_VarArgs("%s/gamecontent.txt", gamePath), "MOD" ) )
	{
		pFileSystemInfo = pMainFile->FindKey( "FileSystem" );
		if (pFileSystemInfo)
		{
			for ( KeyValues *pKey = pFileSystemInfo->GetFirstSubKey(); pKey; pKey = pKey->GetNextKey() )
			{
				if ( strcmp( pKey->GetName(), "AppId" ) == 0 )
				{
					int nExtraContentId = pKey->GetInt();
					if (nExtraContentId)
					{
						AddSearchPathByAppId( nExtraContentId );
						Steam_MountSteamContent( nExtraContentId );
					}
				}
			}
		}
	}
	pMainFile->deleteThis();
}
