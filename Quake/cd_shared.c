/*
	cd_shared.c

	Copyright (C) 1996-1997  Id Software, Inc.

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		51 Franklin St, Fifth Floor,
		Boston, MA  02110-1301  USA
*/

#include "quakedef.h"

static qboolean enabled = false;
static qboolean	usingBackend = false;

static byte	remap[100];
static char	playTrackName[MAX_QPATH];

static void CDAudio_Next(void);

static qboolean CDAudio_IsNumberedTrack(const char *trackName)
{
	int len = strlen(trackName);
	int i;
	for ( i = 0; i < len; i++ )
	{
		if (!isdigit(trackName[i]))
		{
			return false;
		}
	}
	return true;
}

static void CDAudio_FinishedCallback(void *userdata)
{
	CDAudio_Next();
}

static qboolean CDAudio_TryPlayNamed(const char *name, qboolean looping)
{
	qboolean success = S_Base_StartBackgroundTrack(name, looping, CDAudio_FinishedCallback, NULL);
	return success;
}

void CDAudio_PlayNamed(const char *name, qboolean looping)
{
	if (!enabled)
		return;

    // already playing the correct track?
	if ((0 == strcmp(name, playTrackName)) &&
		(S_BackgroundTrackIsPlaying() || (usingBackend && CDAudioBackend_IsPlaying())))
	{
        return;
	}

	CDAudio_Stop();
	
	int track = 0;
	if (CDAudio_IsNumberedTrack(name))
	{
		track = atoi(name);
        if (track < 100)
        {
            track = remap[track];
        }
		q_snprintf(playTrackName, sizeof(playTrackName), "%03d", track);
	}
    else
    {
        q_snprintf(playTrackName, sizeof(playTrackName), "%s", name);
    }

	// First try to play a music file (path code from Darkplaces)

	char filename[MAX_QPATH];
	
	if (track > 0) // We were given a numbered track
	{
		q_snprintf(filename, sizeof(filename), "sound/cdtracks/track%03u", track);
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		q_snprintf(filename, sizeof(filename), "sound/cdtracks/track%02u", track);
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		q_snprintf(filename, sizeof(filename), "music/cdtracks/track%03u", track);// added by motorsep
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		q_snprintf(filename, sizeof(filename), "music/cdtracks/track%02u", track);// added by motorsep
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		q_snprintf(filename, sizeof(filename), "music/track%03u", track);// added by motorsep
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		q_snprintf(filename, sizeof(filename), "music/track%02u", track);// added by motorsep		
		if (CDAudio_TryPlayNamed(filename, looping)) return;
		
		// No music file, so try using the hardware CD player
		
		CDAudioBackend_Play(track, looping);
		if (CDAudioBackend_IsPlaying())
		{
			usingBackend = true;
		}
		return;
	}
	
	// We were given a named track
	
	q_snprintf(filename, sizeof(filename), "%s", playTrackName);
	if (CDAudio_TryPlayNamed(filename, looping)) return;		

	q_snprintf(filename, sizeof(filename), "sound/%s", playTrackName);
	if (CDAudio_TryPlayNamed(filename, looping)) return;		

	q_snprintf(filename, sizeof(filename), "sound/cdtracks/%s", playTrackName);
	if (CDAudio_TryPlayNamed(filename, looping)) return;		

	q_snprintf(filename, sizeof(filename), "music/%s", playTrackName);
	if (CDAudio_TryPlayNamed(filename, looping)) return;		

	q_snprintf(filename, sizeof(filename), "music/cdtracks/%s", playTrackName);
	if (CDAudio_TryPlayNamed(filename, looping)) return;		

	Con_Printf("WARNING: Couldn't find music track \"%s\"\n", playTrackName);
}

void CDAudio_Play(byte track, qboolean looping)
{
    char name[MAX_QPATH];
    sprintf(name, "%d", (int)track);
    CDAudio_PlayNamed(name, looping);
}

void CDAudio_Stop(void)
{
	if (!enabled)
		return;

	if (usingBackend)
	{
		CDAudioBackend_Stop();
		usingBackend = false;
	}
	else
	{
		S_Base_StopBackgroundTrack();
	}
}

static void CDAudio_Next(void)
{
	byte track;

	if (!enabled)
		return;
	
	if (usingBackend)
	{
		CDAudioBackend_Next();
	}
	else
	{
		if (!CDAudio_IsNumberedTrack(playTrackName))
		{
			CDAudio_Stop();
			return;
		}
		
		track = atoi(playTrackName) + 1;
		
		CDAudio_Play (track, S_BackgroundTrackIsLooping());		
	}
}

static void CDAudio_Prev(void)
{
	byte track;

	if (!enabled)
		return;

	if (usingBackend)
	{
		CDAudioBackend_Prev();
	}
	else
	{
		if (!CDAudio_IsNumberedTrack(playTrackName))
		{
			CDAudio_Stop();
			return;
		}

		track = atoi(playTrackName) - 1;
		if (track < 1)
			track = 1;

		CDAudio_Play(track, S_BackgroundTrackIsLooping());
	}
}

void CDAudio_Pause(void)
{
	if (!enabled)
		return;

	if (usingBackend)
	{	
		CDAudioBackend_Pause();
	}
	else
	{
		S_PauseBackgroundTrack();
	}
}

void CDAudio_Resume(void)
{
	if (!enabled)
		return;

	if (usingBackend)
	{
		CDAudioBackend_Resume();
	}
	else
	{
		S_ResumeBackgroundTrack();
	}
}

static void CD_f (void)
{
	const char	*command,*arg2;
	int		ret, n;

	if (Cmd_Argc() < 2)
	{
		Con_Printf("commands:\n");
		Con_Printf("  on, off, reset, remap, \n");
		Con_Printf("  play, stop, next, prev, loop,\n");
		Con_Printf("  pause, resume, eject, info\n");
		return;
	}

	command = Cmd_Argv (1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		CDAudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "reset") == 0)
	{
		CDAudio_Stop();
		
		for (n = 0; n < 100; n++)
			remap[n] = n;

		// FIXME: backend get disc info?

		return;
	}

	if (Q_strcasecmp(command, "remap") == 0)
	{
		ret = Cmd_Argc () - 2;
		if (ret <= 0)
		{
			for (n = 1; n < 100; n++)
				if (remap[n] != n)
					Con_Printf ("  %u -> %u\n", n, remap[n]);
			return;
		}
		for (n = 1; n <= ret; n++)
			remap[n] = atoi(Cmd_Argv (n + 1));
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		arg2 = Cmd_Argv (2);
        if (*arg2)
        {
            CDAudio_PlayNamed(Cmd_Argv(2), false);
        }
		else
		{
		    CDAudio_Play((byte)1, false);
		}
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		arg2 = Cmd_Argv (2);
        if (*arg2)
        {
            CDAudio_PlayNamed(Cmd_Argv(2), true);
        }
		else
		{
		    CDAudio_Play((byte)1, true);
		}
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		CDAudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		CDAudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		CDAudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "next") == 0)
	{
		CDAudio_Next();
		return;
	}

	if (Q_strcasecmp(command, "prev") == 0)
	{
		CDAudio_Prev();
		return;
	}

	if (Q_strcasecmp(command, "eject") == 0)
	{
		CDAudioBackend_Eject();
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		if (usingBackend)
		{
			CDAudioBackend_Info();
		}
		else
		{
			if (S_BackgroundTrackIsPlaying())
				Con_Printf ("Currently %s track %s\n", S_BackgroundTrackIsLooping() ? "looping" : "playing", playTrackName);
			else if (S_BackgroundTrackIsPaused())
				Con_Printf ("Paused %s track %s\n", S_BackgroundTrackIsLooping() ? "looping" : "playing", playTrackName);
			
			Con_Printf ("Volume is %f\n", bgmvolume.value);			
		}
		return;
	}
	Con_Printf ("cd: no such command. Use \"cd\" for help.\n");
}


void CDAudio_Update(void)
{
	if (!enabled)
		return;
	
	CDAudioBackend_Update();
}

int CDAudio_Init(void)
{
	int	i;

	enabled = true;
	CDAudioBackend_Init();

	for (i = 0; i < 100; i++)
		remap[i] = i;

	Cmd_AddCommand ("cd", CD_f);

	return 0;
}

void CDAudio_Shutdown(void)
{
	CDAudio_Stop();
	
	CDAudioBackend_Shutdown();
}