#include "quakedef.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#ifdef WIN32
#define stat _stat
#endif

// Map file watch
char Dev_MapToWatch[512];
long long Dev_MapWatchLastModTime  = 0;
double Dev_MapWatchLastPollTime    = 0;
double Dev_MapWatchPollInterval    = 1; // Check once a second
BOOL Dev_MapIsPendingReload        = false;
vec3_t Dev_PlayerPos;
vec3_t Dev_PlayerViewAngles;

void Dev_Init_f()
{
	Cmd_AddCommand("mapwatch", Dev_MapWatch);
}

void Dev_Update()
{
	Dev_CheckForMapFileChange();
}

// Arkii: Windows file change api sucks :<
// So I went with polling once a second and checking the modified time, should be fine and easyier cross platform
void Dev_CheckForMapFileChange()
{
	struct _stat buf;
	char timebuf[26];

	if (Dev_MapWatchLastPollTime + Dev_MapWatchPollInterval <= realtime)
	{
		Dev_MapWatchLastPollTime = realtime;

		int error = stat(Dev_MapToWatch, &buf);
		if (error == 0)
		{
			Con_Printf("%i %i %i \n", buf.st_mtime, buf.st_atime, buf.st_ctime);
			long long fileMod = buf.st_mtime + buf.st_size;
			if (fileMod != Dev_MapWatchLastModTime)
			{
				Dev_MapWatchLastModTime = fileMod;
				Con_Printf("Mapwatch: map has changed, reloading");
				Dev_ReloadMap();
			}
		}
	}

	// Reset the players position after a reload and wait a second before moving back
	if (Dev_MapIsPendingReload && cl.entities != NULL && cl.time >= 0.1)
	{
		VectorCopy(Dev_PlayerPos, sv_player->v.oldorigin);
		VectorCopy(Dev_PlayerPos, sv_player->v.origin);
		VectorCopy(Dev_PlayerPos, cl.entities[cl.viewentity].currentorigin);
		VectorCopy(Dev_PlayerViewAngles, sv_player->v.angles);

		Dev_MapIsPendingReload = false;
	}
}

// Watchs the map dir for changes
void Dev_WatchMapsDir()
{
#ifdef WIN32
	
	//Dev_MapDirHDL = CreateFile(com_gamedir, FILE_LIST_DIRECTORY | FILE_FLAG_OVERLAPPED, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
#endif
}

// Loads a map and watches the file for changes, if the file is changed, reloads but keeps the players position
// Arkii
void Dev_MapWatch()
{
	if (Cmd_Argc() < 2)
	{
		Con_Printf("mapwatch <levelname>: load a new map and reload it on changes\n");
		return;
	}

	//q_strlcpy(Dev_MapToWatch, Cmd_Argv(1), sizeof(Dev_MapToWatch));
	strcpy(Dev_MapToWatch, com_gamedir);
	strcat(Dev_MapToWatch, "/maps/");
	strcat(Dev_MapToWatch, Cmd_Argv(1));
	strcat(Dev_MapToWatch, ".bsp");
	Con_Printf("Watching map %s for changes :>", Dev_MapToWatch);

	Host_Map_f();
	Dev_WatchMapsDir();
}

// Reload the current map but keep the players position
void Dev_ReloadMap()
{
	VectorCopy(cl.entities[cl.viewentity].origin, Dev_PlayerPos);
	VectorCopy(cl.viewangles, Dev_PlayerViewAngles);
	Dev_MapIsPendingReload = true;

	//Host_Restart_f();
	if (*sv.name)
		Cmd_ExecuteString(va("map \"%s\"\n", sv.name), src_command);
}