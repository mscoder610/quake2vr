/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// ui_startserver.c -- the start server menu 

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"
#include "../renderer/include/r_local.h"

/*
=============================================================================

START SERVER MENU

=============================================================================
*/
static menuframework_s s_startserver_menu;

static menuaction_s	s_startserver_start_action;
static menuaction_s	s_startserver_dmoptions_action;
static menufield_s	s_timelimit_field;
static menufield_s	s_fraglimit_field;
static menufield_s	s_maxclients_field;
static menufield_s	s_hostname_field;
static menulist_s	s_startmap_list;
menulist_s			s_rules_box;
static menuaction_s	s_startserver_back_action;

#define M_UNSET 0
#define	M_MISSING 1
#define	M_FOUND 2
static	byte *ui_svr_mapshotvalid; // levelshot truth table
static  struct image_s **ui_svr_mapshot;

typedef enum {
	MAP_DM,
	MAP_COOP,
	MAP_CTF,
	MAP_3TCTF,
	NUM_MAPTYPES
} maptype_t;

typedef struct {
	int32_t		index;
	char	*tokens;
} gametype_names_t;

gametype_names_t gametype_names[] = 
{
	{MAP_DM, "dm ffa team teamdm"},
	{MAP_COOP, "coop"},
	{MAP_CTF, "ctf"},
	{MAP_3TCTF, "3tctf"},
};

#define MAX_ARENAS 1024
#define MAX_ARENAS_TEXT 8192

static maptype_t ui_svr_maptype;
static int32_t	ui_svr_nummaps;
static char **ui_svr_mapnames;
static int32_t	ui_svr_listfile_nummaps;
static char **ui_svr_listfile_mapnames;
static int32_t	ui_svr_arena_nummaps[NUM_MAPTYPES];
static char **ui_svr_arena_mapnames[NUM_MAPTYPES];

//=============================================================================

/*
===============
UI_ParseArenaFromFile
Partially from Q3 source
===============
*/
qboolean UI_ParseArenaFromFile (const char *filename, char *shortname, char *longname, char *gametypes)
{
	int32_t				len;
	fileHandle_t	f;
	char			buf[MAX_ARENAS_TEXT];
	char			*s, *token, *dest;

	len = FS_FOpenFile (filename, &f, FS_READ);
	if (!f) {
		Com_Printf (S_COLOR_RED "UI_ParseArenaFromFile: file not found: %s\n", filename);
		return false;
	}
	if (len >= MAX_ARENAS_TEXT) {
		Com_Printf (S_COLOR_RED "UI_ParseArenaFromFile: file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT);
		FS_FCloseFile (f);
		return false;
	}

	FS_Read (buf, len, f);
	buf[len] = 0;
	FS_FCloseFile (f);

	s = buf;
	// get the opening curly brace
	token = COM_Parse (&s);
	if (!token) {
		Com_Printf ("UI_ParseArenaFromFile: unexpected EOF\n");
		return false;
	}
	if (token[0] != '{') {
		Com_Printf ("UI_ParseArenaFromFile: found %s when expecting {\n", token);
		return false;
	}

	// go through all the parms
	while (s < (buf + len))
	{
		dest = NULL;
		token = COM_Parse (&s);
		if (token && (token[0] == '}')) break;
		if (!token || !s) {
			Com_Printf ("UI_ParseArenaFromFile: EOF without closing brace\n");
			break;
		}

		if (!Q_strcasecmp(token, "map"))
			dest = shortname;
		else if (!Q_strcasecmp(token, "longname"))
			dest = longname;
		else if (!Q_strcasecmp(token, "type"))
			dest = gametypes;
		if (dest)
		{
			token = COM_Parse (&s);
			if (!token) {
				Com_Printf ("UI_ParseArenaFromFile: unexpected EOF\n");
				return false;
			}
			if (token[0] == '}') {
				Com_Printf ("UI_ParseArenaFromFile: closing brace without data\n");
				break;
			}
			if (!s) {
				Com_Printf ("UI_ParseArenaFromFile: EOF without closing brace\n");
				break;
			}
			strcpy( dest, token );
		}
	}
	if (!shortname || !strlen(shortname)) {
		Com_Printf (S_COLOR_RED "UI_ParseArenaFromFile: %s: map field not found\n", filename);
		return false;
	}
	if (!strlen(longname))
		strncpy(longname, shortname, strlen(shortname));
	return true;
}


/*
===============
UI_SortArenas
===============
*/
void UI_SortArenas (char **list, int32_t len)
{
	int32_t			i, j;
	char		*temp, *s1, *s2;
	qboolean	moved;

	if (!list || len < 2)
		return;

	for (i=(len-1); i>0; i--)
	{
		moved = false;
		for (j=0; j<i; j++)
		{
			if (!list[j]) break;
			s1 = strchr(list[j], '\n')+1;
			s2 = strchr(list[j+1], '\n')+1;
			if (Q_strcasecmp(s1, s2) > 0)
			{
				temp = list[j];
				list[j] = list[j+1];
				list[j+1] = temp;
				moved = true;
			}
		}
		if (!moved) break; // done sorting
	}
}


/*
===============
UI_LoadArenas
===============
*/
void UI_LoadArenas (void)
{
	int32_t			i, j;
	qboolean	type_supported[NUM_MAPTYPES];
    sset_t arenafiles, templist;
	//
	// free existing lists and malloc new ones
	//
	for (i=0; i<NUM_MAPTYPES; i++)
	{
		if (ui_svr_arena_mapnames[i])
			FS_FreeFileList (ui_svr_arena_mapnames[i], ui_svr_arena_nummaps[i]);
		ui_svr_arena_nummaps[i] = 0;
		ui_svr_arena_mapnames[i] = (char**)Z_TagMalloc( sizeof( char * ) * MAX_ARENAS, TAG_MENU );
	}

    Q_SSetInit(&arenafiles, MAX_ARENAS, MAX_OSPATH, TAG_MENU);
    Q_SSetInit(&templist, MAX_ARENAS, MAX_OSPATH, TAG_MENU);
    
    // check in paks for .arena files
    //
    FS_ListPak("scripts/", &arenafiles);
    for (i=0; i<arenafiles.currentSize && templist.currentSize<MAX_ARENAS; i++)
    {
        const char *p = Q_SSetGetString(&arenafiles, i);
        
        if (!strstr(p, ".arena"))
            continue;
        if (!Q_SSetContains(&templist, p)) // check if already in list
        {
            char	shortname[MAX_TOKEN_CHARS];
            char	longname[MAX_TOKEN_CHARS];
            char	gametypes[MAX_TOKEN_CHARS];
            char	scratch[200];
            if (UI_ParseArenaFromFile (p, shortname, longname, gametypes))
            {
                char *s = gametypes;
                Com_sprintf(scratch, sizeof(scratch), "%s\n%s", longname, shortname);
                
                for (j=0; j<NUM_MAPTYPES; j++)
                    type_supported[j] = false;
                
                while (s != NULL)
                {
                    char *tok = COM_Parse (&s);
                    for (j=0; j<NUM_MAPTYPES; j++)
                    {
                        char *s2 = gametype_names[j].tokens;
                        while (s2 != NULL) {
                            char *tok2 = COM_Parse (&s2);
                            if ( !Q_strcasecmp(tok, tok2) )
                                type_supported[j] = true;
                        }
                    }
                }
                
                for (j=0; j<NUM_MAPTYPES; j++)
                    if (type_supported[j]) {
                        ui_svr_arena_mapnames[j][ui_svr_arena_nummaps[j]] = (char*)Z_TagStrdup( scratch, TAG_MENU );
                        ui_svr_arena_nummaps[j]++;
                    }
                
                //Com_Printf ("UI_LoadArenas: successfully loaded arena file %s: mapname: %s levelname: %s gametypes: %s\n", p, shortname, longname, gametypes);
                Q_SSetInsert(&templist, p);
            }
        }
    }
    Q_SSetFree(&arenafiles);
    Q_SSetFree(&templist);
    for (i=0; i<NUM_MAPTYPES; i++)
        UI_SortArenas (ui_svr_arena_mapnames[i], ui_svr_arena_nummaps[i]);
    
    //	Com_Printf ("UI_LoadArenas: loaded %i arena file(s)\n", narenanames);
    //	for (i=0; i<NUM_MAPTYPES; i++)
    //		Com_Printf ("%s: %i arena file(s)\n", gametype_names[i].tokens, ui_svr_arena_nummaps[i]);
}


/*
===============
UI_LoadMapList
===============
*/
void UI_LoadMapList (void)
{
	char	*buffer, *s;
	char	mapsname[1024];
	int32_t		i, length;
	FILE	*fp;

	//
	// free existing list
	//
	if (ui_svr_listfile_mapnames)
		FS_FreeFileList (ui_svr_listfile_mapnames, ui_svr_listfile_nummaps);
	ui_svr_listfile_nummaps = 0;

	//
	// load the list of map names
	//
	Com_sprintf( mapsname, sizeof( mapsname ), "%s/maps.lst", FS_Gamedir() );
    
    
	if ( ( fp = fopen( mapsname, "rb" ) ) == 0 )
	{
		if ( ( length = FS_LoadFile( "maps.lst", ( void ** ) &buffer ) ) == -1 )
			Com_Error( ERR_DROP, "couldn't find maps.lst\n" );
	}
	else
	{
#ifdef _WIN32
		length = filelength( fileno( fp  ) );
#else
		fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		fseek(fp, 0, SEEK_SET);
#endif
		buffer = (char*)Z_TagMalloc( length, TAG_MENU );
		fread( buffer, length, 1, fp );
	}

	s = buffer;

    for (i = 0; i < length; i++)
	{
		if (s[i] == '\n')
            ui_svr_listfile_nummaps++;
    }
    
    if (ui_svr_listfile_nummaps > 0)
    {
        ui_svr_listfile_mapnames = (char**)Z_TagMalloc( sizeof( char * ) * ( ui_svr_listfile_nummaps + 1 ), TAG_MENU );
        
        for (i = 0; i < ui_svr_listfile_nummaps; i++)
        {
            char	shortname[MAX_TOKEN_CHARS];
            char	*longname;
            char	scratch[2 * MAX_TOKEN_CHARS + 2];
            
            strcpy(shortname,COM_Parse( &s ));
            longname = COM_Parse(&s);
            Com_sprintf( scratch, sizeof( scratch ), "%s\n%s", longname, shortname );

            ui_svr_listfile_mapnames[i] = (char*)Z_TagStrdup( scratch, TAG_MENU );
        }
    } else {
        ui_svr_listfile_mapnames = (char**)Z_TagMalloc( sizeof( char * ) * ( 2 ), TAG_MENU );
        ui_svr_listfile_nummaps = 1;
        ui_svr_listfile_mapnames[0] = (char*)Z_TagStrdup("\"Outer Base\"\nbase1", TAG_MENU);
    }
    
    if ( fp != 0 )
    {
        fp = 0;
        Z_Free( buffer );
    }
    else
        FS_FreeFile( buffer );

	UI_LoadArenas ();

	ui_svr_maptype = MAP_DM; // init maptype
}


/*
===============
UI_BuildMapList
===============
*/
void UI_BuildMapList (maptype_t maptype)
{
	int32_t		i;

    ui_svr_nummaps = ui_svr_listfile_nummaps + ui_svr_arena_nummaps[maptype];

	if (ui_svr_mapnames)
        ui_svr_mapnames = (char**)Z_Realloc(ui_svr_mapnames, sizeof( char * ) * ( ui_svr_nummaps + 1 ));
    else
        ui_svr_mapnames = (char**)Z_TagMalloc( sizeof( char * ) * ( ui_svr_nummaps + 1 ) , TAG_MENU);

	for (i = 0; i < ui_svr_nummaps; i++)
	{
		if (i < ui_svr_listfile_nummaps)
			ui_svr_mapnames[i] = ui_svr_listfile_mapnames[i];
		else
			ui_svr_mapnames[i] = ui_svr_arena_mapnames[maptype][i-ui_svr_listfile_nummaps];
	}
	ui_svr_mapnames[ui_svr_nummaps] = 0;
	ui_svr_maptype = maptype;

	if (s_startmap_list.curvalue >= ui_svr_nummaps) // paranoia
		s_startmap_list.curvalue = 0;
}	

void UI_RefreshMapImages(void) {
    // levelshot found table
    if (ui_svr_mapshotvalid && ui_svr_mapshot) {
        ui_svr_mapshotvalid = (byte*)Z_Realloc(ui_svr_mapshotvalid, sizeof( byte ) * ( ui_svr_nummaps + 1 ));
        ui_svr_mapshot = (struct image_s**)Z_Realloc(ui_svr_mapshot, sizeof( struct image_s *) * ( ui_svr_nummaps + 1 ) );
        
        memset( ui_svr_mapshotvalid, 0, sizeof( byte ) * ( ui_svr_nummaps + 1 ) );
        memset( ui_svr_mapshot, 0, sizeof( struct image_s *) * ( ui_svr_nummaps + 1 ) );
        if ((ui_svr_mapshot[ui_svr_nummaps] = R_DrawFindPic(UI_NOSCREEN_NAME)))
            ui_svr_mapshotvalid[ui_svr_nummaps] = M_FOUND;
        else
            ui_svr_mapshotvalid[ui_svr_nummaps] = M_MISSING;
    }
}
/*
===============
UI_RefreshMapList
===============
*/
void UI_RefreshMapList (maptype_t maptype)
{
	int32_t		i;

	if (maptype == ui_svr_maptype) // no change
		return;

	// reset startmap if it's in the part of the list that changed
	if (s_startmap_list.curvalue >= ui_svr_listfile_nummaps)
		s_startmap_list.curvalue = 0;

	UI_BuildMapList (maptype);
	s_startmap_list.itemnames = (const char **)ui_svr_mapnames;
	for (i=0; s_startmap_list.itemnames[i]; i++);
	s_startmap_list.numitemnames = i;

    UI_RefreshMapImages();
}



//=============================================================================


void DMOptionsFunc (void *self)
{
	if (s_rules_box.curvalue == 1)
		return;
	M_Menu_DMOptions_f();
}

void RulesChangeFunc (void *self)
{
	if (s_rules_box.curvalue == 0) 	// DM
	{
		s_maxclients_field.generic.statusbar = NULL;
		if (atoi(s_maxclients_field.buffer) <= 8) // set default of 8
			strcpy( s_maxclients_field.buffer, "8" );
		s_startserver_dmoptions_action.generic.statusbar = NULL;
		UI_RefreshMapList (MAP_DM);
	}
	else if (s_rules_box.curvalue == 1)		// coop				// PGM
	{
		s_maxclients_field.generic.statusbar = "4 maximum for cooperative";
		if (atoi(s_maxclients_field.buffer) > 4)
			strcpy( s_maxclients_field.buffer, "4" );
		s_startserver_dmoptions_action.generic.statusbar = "N/A for cooperative";
		UI_RefreshMapList (MAP_COOP);
	}
	else if (s_rules_box.curvalue == 2) // CTF
	{
		s_maxclients_field.generic.statusbar = NULL;
		if (atoi(s_maxclients_field.buffer) <= 12) // set default of 12
			strcpy( s_maxclients_field.buffer, "12" );
		s_startserver_dmoptions_action.generic.statusbar = NULL;
		UI_RefreshMapList (MAP_CTF);
	}
	else if (s_rules_box.curvalue == 3) // 3Team CTF
	{
		s_maxclients_field.generic.statusbar = NULL;
		if (atoi(s_maxclients_field.buffer) <= 18) // set default of 18
			strcpy( s_maxclients_field.buffer, "18" );
		s_startserver_dmoptions_action.generic.statusbar = NULL;
		UI_RefreshMapList (MAP_3TCTF);
	}
	// ROGUE GAMES
	else if (roguepath() && s_rules_box.curvalue == 4) // tag	
	{
		s_maxclients_field.generic.statusbar = NULL;
		if (atoi(s_maxclients_field.buffer) <= 8) // set default of 8
			strcpy( s_maxclients_field.buffer, "8" );
		s_startserver_dmoptions_action.generic.statusbar = NULL;
		UI_RefreshMapList (MAP_DM);
	}
}

void StartServerActionFunc (void *self)
{
	char	startmap[1024];
	int32_t		timelimit;
	int32_t		fraglimit;
	int32_t		maxclients;
	char	*spot;

	strcpy( startmap, strchr( ui_svr_mapnames[s_startmap_list.curvalue], '\n' ) + 1 );

	maxclients  = atoi( s_maxclients_field.buffer );
	timelimit	= atoi( s_timelimit_field.buffer );
	fraglimit	= atoi( s_fraglimit_field.buffer );

	Cvar_SetValue( "maxclients", ClampCvar( 0, maxclients, maxclients ) );
	Cvar_SetValue ("timelimit", ClampCvar( 0, timelimit, timelimit ) );
	Cvar_SetValue ("fraglimit", ClampCvar( 0, fraglimit, fraglimit ) );
	Cvar_Set("hostname", s_hostname_field.buffer );

	Cvar_SetValue ("deathmatch", s_rules_box.curvalue != 1);
	Cvar_SetValue ("coop", s_rules_box.curvalue == 1);
	Cvar_SetValue ("ctf", s_rules_box.curvalue == 2);
	Cvar_SetValue ("ttctf", s_rules_box.curvalue == 3);
	Cvar_SetValue ("gamerules", roguepath() ? ((s_rules_box.curvalue == 4) ? 2 : 0) : 0);

	spot = NULL;
	if (s_rules_box.curvalue == 1)		// PGM
	{
 		if(Q_strcasecmp(startmap, "bunk1") == 0)
  			spot = "start";
 		else if(Q_strcasecmp(startmap, "mintro") == 0)
  			spot = "start";
 		else if(Q_strcasecmp(startmap, "fact1") == 0)
  			spot = "start";
 		else if(Q_strcasecmp(startmap, "power1") == 0)
  			spot = "pstart";
 		else if(Q_strcasecmp(startmap, "biggun") == 0)
  			spot = "bstart";
 		else if(Q_strcasecmp(startmap, "hangar1") == 0)
  			spot = "unitstart";
 		else if(Q_strcasecmp(startmap, "city1") == 0)
  			spot = "unitstart";
 		else if(Q_strcasecmp(startmap, "boss1") == 0)
			spot = "bosstart";
	}

	if (spot)
	{
		if (Com_ServerState())
			Cbuf_AddText ("disconnect\n");
		Cbuf_AddText (va("gamemap \"*%s$%s\"\n", startmap, spot));
	}
	else
	{
		Cbuf_AddText (va("map %s\n", startmap));
	}

	UI_ForceMenuOff ();
}

void StartServer_MenuInit (void)
{
	static const char *dm_coop_names[] =
	{
		"deathmatch",
		"cooperative",
		"CTF",
		"3Team CTF",
		0
	};

	static const char *dm_coop_names_rogue[] =
	{
		"deathmatch",
		"cooperative",
		"CTF",
		"3Team CTF",
		"tag",
		0
	};

	int32_t y = 0;
	
	UI_LoadMapList (); // moved to UI_Init
	UI_BuildMapList (ui_svr_maptype); // was MAP_DM

	// levelshot found table
    ui_svr_mapshotvalid = (byte*)Z_TagMalloc( sizeof( byte ) * ( ui_svr_nummaps + 1 ) , TAG_MENU);
    
    ui_svr_mapshot = (struct image_s**)Z_TagMalloc( sizeof( struct image_s *) * ( ui_svr_nummaps + 1 ), TAG_MENU );
    
    if ((ui_svr_mapshot[ui_svr_nummaps] = R_DrawFindPic(UI_NOSCREEN_NAME)))
        ui_svr_mapshotvalid[ui_svr_nummaps] = M_FOUND;
    else
        ui_svr_mapshotvalid[ui_svr_nummaps] = M_MISSING;

    
    //
	// initialize the menu stuff
	//
	s_startserver_menu.x = SCREEN_WIDTH*0.5 - 140;
	//s_startserver_menu.y = 0;
	s_startserver_menu.nitems = 0;

	s_startmap_list.generic.type	= MTYPE_SPINCONTROL;
	s_startmap_list.generic.x		= 0;
	s_startmap_list.generic.y		= y;
	s_startmap_list.generic.name	= "initial map";
	s_startmap_list.itemnames		= (const char **)ui_svr_mapnames;

	s_rules_box.generic.type	= MTYPE_SPINCONTROL;
	s_rules_box.generic.x		= 0;
	s_rules_box.generic.y		= y += 2*MENU_LINE_SIZE;
	s_rules_box.generic.name	= "rules";
//PGM - rogue games only available with rogue DLL.
	if (roguepath())
		s_rules_box.itemnames	= dm_coop_names_rogue;
	else
		s_rules_box.itemnames	= dm_coop_names;
//PGM
	if (Cvar_VariableValue("ttctf"))
		s_rules_box.curvalue = 3;
	else if (Cvar_VariableValue("ctf"))
		s_rules_box.curvalue = 2;
	else if (roguepath() && Cvar_VariableValue("gamerules") == 2)
		s_rules_box.curvalue = 4;
	else if (Cvar_VariableValue("coop"))
		s_rules_box.curvalue = 1;
	else
		s_rules_box.curvalue = 0;
	s_rules_box.generic.callback = RulesChangeFunc;

	s_timelimit_field.generic.type		= MTYPE_FIELD;
	s_timelimit_field.generic.name		= "time limit";
	s_timelimit_field.generic.flags		= QMF_NUMBERSONLY;
	s_timelimit_field.generic.x			= 0;
	s_timelimit_field.generic.y			= y += 2*MENU_FONT_SIZE;
	s_timelimit_field.generic.statusbar	= "0 = no limit";
	s_timelimit_field.length			= 4;
	s_timelimit_field.visible_length	= 4;
	strcpy( s_timelimit_field.buffer, Cvar_VariableString("timelimit") );
	s_timelimit_field.cursor			= strlen( s_timelimit_field.buffer );

	s_fraglimit_field.generic.type		= MTYPE_FIELD;
	s_fraglimit_field.generic.name		= "frag limit";
	s_fraglimit_field.generic.flags		= QMF_NUMBERSONLY;
	s_fraglimit_field.generic.x			= 0;
	s_fraglimit_field.generic.y			= y += 2.25*MENU_FONT_SIZE;
	s_fraglimit_field.generic.statusbar	= "0 = no limit";
	s_fraglimit_field.length			= 4;
	s_fraglimit_field.visible_length	= 4;
	strcpy( s_fraglimit_field.buffer, Cvar_VariableString("fraglimit") );
	s_fraglimit_field.cursor			= strlen( s_fraglimit_field.buffer );

	/*
	** maxclients determines the maximum number of players that can join
	** the game.  If maxclients is only "1" then we should default the menu
	** option to 8 players, otherwise use whatever its current value is. 
	** Clamping will be done when the server is actually started.
	*/
	s_maxclients_field.generic.type			= MTYPE_FIELD;
	s_maxclients_field.generic.name			= "max players";
	s_maxclients_field.generic.flags		= QMF_NUMBERSONLY;
	s_maxclients_field.generic.x			= 0;
	s_maxclients_field.generic.y			= y += 2.25*MENU_FONT_SIZE;
	s_maxclients_field.generic.statusbar	= NULL;
	s_maxclients_field.length				= 3;
	s_maxclients_field.visible_length		= 3;
	if ( Cvar_VariableValue( "maxclients" ) == 1 )
		strcpy( s_maxclients_field.buffer, "8" );
	else 
		strcpy( s_maxclients_field.buffer, Cvar_VariableString("maxclients") );
	s_maxclients_field.cursor				= strlen( s_maxclients_field.buffer );

	s_hostname_field.generic.type			= MTYPE_FIELD;
	s_hostname_field.generic.name			= "hostname";
	s_hostname_field.generic.flags			= 0;
	s_hostname_field.generic.x				= 0;
	s_hostname_field.generic.y				= y += 2.25*MENU_FONT_SIZE;
	s_hostname_field.generic.statusbar		= NULL;
	s_hostname_field.length					= 12;
	s_hostname_field.visible_length			= 12;
	strcpy( s_hostname_field.buffer, Cvar_VariableString("hostname") );
	s_hostname_field.cursor					= strlen( s_hostname_field.buffer );

	s_startserver_dmoptions_action.generic.type = MTYPE_ACTION;
	s_startserver_dmoptions_action.generic.name	= " deathmatch flags";
	s_startserver_dmoptions_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_dmoptions_action.generic.x	= 24;
	s_startserver_dmoptions_action.generic.y	= y += 2.25*MENU_FONT_SIZE;
	s_startserver_dmoptions_action.generic.statusbar = NULL;
	s_startserver_dmoptions_action.generic.callback = DMOptionsFunc;

	s_startserver_start_action.generic.type = MTYPE_ACTION;
	s_startserver_start_action.generic.name	= " begin";
	s_startserver_start_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_start_action.generic.x	= 24;
	s_startserver_start_action.generic.y	= y += 2*MENU_LINE_SIZE;
	s_startserver_start_action.generic.callback = StartServerActionFunc;

	s_startserver_back_action.generic.type = MTYPE_ACTION;
	s_startserver_back_action.generic.name	= " back to multiplayer";
	s_startserver_back_action.generic.flags= QMF_LEFT_JUSTIFY;
	s_startserver_back_action.generic.x	= 24;
	s_startserver_back_action.generic.y	= y += 3*MENU_LINE_SIZE;
	s_startserver_back_action.generic.callback = UI_BackMenu;

	Menu_AddItem( &s_startserver_menu, &s_startmap_list );
	Menu_AddItem( &s_startserver_menu, &s_rules_box );
	Menu_AddItem( &s_startserver_menu, &s_timelimit_field );
	Menu_AddItem( &s_startserver_menu, &s_fraglimit_field );
	Menu_AddItem( &s_startserver_menu, &s_maxclients_field );
	Menu_AddItem( &s_startserver_menu, &s_hostname_field );
	Menu_AddItem( &s_startserver_menu, &s_startserver_dmoptions_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_start_action );
	Menu_AddItem( &s_startserver_menu, &s_startserver_back_action );

	Menu_Center( &s_startserver_menu );

	// call this now to set proper inital state
	RulesChangeFunc ( NULL );
}

void DrawStartSeverLevelshot (void)
{	
	char startmap[MAX_QPATH];
	char mapshotname [MAX_QPATH];
	int32_t i = s_startmap_list.curvalue;
	strcpy( startmap, strchr( ui_svr_mapnames[i], '\n' ) + 1 );

	SCR_DrawFill (SCREEN_WIDTH/2+44, SCREEN_HEIGHT/2-70, 244, 184, ALIGN_CENTER, 60,60,60,255);

	if ( ui_svr_mapshotvalid[i] == M_UNSET) { // init levelshot
		Com_sprintf(mapshotname, sizeof(mapshotname), "/levelshots/%s.pcx", startmap);
		if ((ui_svr_mapshot[i] = R_DrawFindPic(mapshotname)))
			ui_svr_mapshotvalid[i] = M_FOUND;
		else
			ui_svr_mapshotvalid[i] = M_MISSING;
	}
   
	if ( ui_svr_mapshot[i]) {
		SCR_DrawImage (SCREEN_WIDTH/2+46, SCREEN_HEIGHT/2-68, 240, 180, ALIGN_CENTER, ui_svr_mapshot[i], 1.0);
	}
	else if (ui_svr_mapshot[ui_svr_nummaps])
		SCR_DrawImage (SCREEN_WIDTH/2+46, SCREEN_HEIGHT/2-68, 240, 180, ALIGN_CENTER, ui_svr_mapshot[ui_svr_nummaps], 1.0);
	else
		SCR_DrawFill (SCREEN_WIDTH/2+46, SCREEN_HEIGHT/2-68, 240, 180, ALIGN_CENTER, 0,0,0,255);
}

void StartServer_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_start_server" ); // Knightmare added
	Menu_Draw( &s_startserver_menu );
	DrawStartSeverLevelshot (); // added levelshots
}

const char *StartServer_MenuKey (int32_t key)
{
	return Default_MenuKey( &s_startserver_menu, key );
}

void StartServer_Teardown (void)
{
    int i;
    if (ui_svr_mapshot) {
        if (ui_svr_mapshotvalid) {
            int i;
            for (i= 0; i < ui_svr_nummaps; i++) {
                if (ui_svr_mapshotvalid[i] == M_FOUND && ui_svr_mapshot[i]) {
                    R_FreePic(ui_svr_mapshot[i]->name);
                }
            }
            Z_Free(ui_svr_mapshotvalid);
            ui_svr_mapshotvalid = NULL;
        }
        Z_Free(ui_svr_mapshot);
        ui_svr_mapshot = NULL;
    }
    if (ui_svr_mapnames) {
        Z_Free (ui_svr_mapnames);
        ui_svr_mapnames = NULL;
    }
    
    if (ui_svr_listfile_mapnames) {
        FS_FreeFileList (ui_svr_listfile_mapnames, ui_svr_listfile_nummaps);
        ui_svr_listfile_nummaps = 0;
        ui_svr_listfile_mapnames = NULL;
    }
    
    for (i=0; i<NUM_MAPTYPES; i++)
    {
        if (ui_svr_arena_mapnames[i])
            FS_FreeFileList (ui_svr_arena_mapnames[i], ui_svr_arena_nummaps[i]);
        ui_svr_arena_mapnames[i] = NULL;
    }
}

void M_Menu_StartServer_f (void)
{
	StartServer_MenuInit();
	UI_PushMenu( StartServer_MenuDraw, StartServer_MenuKey, StartServer_Teardown );
}
