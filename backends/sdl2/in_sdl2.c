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
// in_sdl2.c -- SDL2 mouse support
// 02/28/2014 - inital version - dghost

#include "../../client/client.h"
#include "../sdl2/sdl2quake.h"
#include "../../client/vr/include/vr.h"
#include "in_sdlcont.h"

extern cursor_t cursor;

int32_t	window_center_x, window_center_y;

enum _ControllerType
{
	ControllerNone = 0, ControllerSDLController
};


static cvar_t	*m_noaccel; //sul
static cvar_t	*in_mouse;
static cvar_t	*in_relativemouse;
static cvar_t	*in_controller;
static cvar_t	*autosensitivity;
static cvar_t	*m_filter;
static cvar_t	*v_centermove;
static cvar_t	*v_centerspeed;

static int32_t	mouse_buttons;
static int32_t	mouse_oldbuttonstate;
static int32_t  mouse_x, mouse_y, old_mouse_x, old_mouse_y, mx_accum, my_accum;
static qboolean	mouseactive;	// false when not focus app
static qboolean	mouseinitialized;
static qboolean	mlooking;
static qboolean	in_appactive;


/*
============================================================

MOUSE CONTROL

============================================================
*/

// mouse variables

void IN_MLookDown (void) { mlooking = true; }
void IN_MLookUp (void) {
	mlooking = false;
	if (!freelook->value && lookspring->value)
		IN_CenterView ();
}

/*
===========
IN_ActivateMouse

Called when the window gains focus or changes in some way
===========
*/
void IN_ActivateMouse (void)
{
	
	int32_t		width, height;

	if (!mouseinitialized)
		return;
	if (!in_mouse->value)
	{
		mouseactive = false;
		return;
	}
	if (mouseactive)
		return;

	mouseactive = true;

	/*
	if (m_noaccel->value) 
		newmouseparms[2]=0; //sul XP fix?
	else 
		newmouseparms[2]=1;
		*/
	SDL_GetWindowSize(mainWindow,&width,&height);
//	SDL_SetWindowGrab(mainWindow,SDL_TRUE);

//	SDL_ShowCursor(0);
	window_center_x = width/2;
	window_center_y = height/2;
	old_mouse_x = 0;
	old_mouse_y = 0;
	if (!RelativeMouse)
		SDL_WarpMouseInWindow(mainWindow,window_center_x, window_center_y);
}


/*
===========
IN_DeactivateMouse

Called when the window loses focus
===========
*/
void IN_DeactivateMouse (void)
{
	if (!mouseinitialized)
		return;
	if (!mouseactive)
		return;


	mouseactive = false;

//	SDL_ShowCursor(1);
//	SDL_SetWindowGrab(mainWindow,SDL_FALSE);
}


/*
===========
IN_StartupMouse
===========
*/
void UI_RefreshCursorMenu (void);
void UI_RefreshCursorLink (void);
//void MW_Set_Hook (void);
//void MW_Shutdown (void);
void IN_StartupMouse (void)
{
	cvar_t		*cv;
	cv = Cvar_Get ("in_initmouse", "1", CVAR_NOSET);
	if ( !cv->value ) 
		return; 

	// Knightmare- added Psychospaz's menu mouse support
	UI_RefreshCursorMenu();
	UI_RefreshCursorLink();

	cursor.mouseaction = false;

	mouseinitialized = true;
	mouse_buttons = 5; // was 3

	if (in_relativemouse->value)
	{
		Com_Printf("Enabling suppport for relative mouse movements: ");
		RelativeMouse = (SDL_bool) !SDL_SetRelativeMouseMode(SDL_TRUE);
		Com_Printf("%s!\n",RelativeMouse ? "ok" : "failed");
	} else {
		RelativeMouse = SDL_FALSE;
	}
}

/*
===========
IN_MouseEvent
===========
*/
void UI_Think_MouseCursor (void);
void IN_MouseEvent (int32_t mstate, uint32_t time)
{
	int32_t		i;

	if (!mouseinitialized)
		return;

	// perform button actions
	for (i=0 ; i<mouse_buttons ; i++)
	{
		if ( (mstate & (1<<i)) &&
			!(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, true, time);
		}

		if ( !(mstate & (1<<i)) &&
			(mouse_oldbuttonstate & (1<<i)) )
		{
			Key_Event (K_MOUSE1 + i, false, time);
		}
	}	

	//set menu cursor buttons
	if (cls.key_dest == key_menu)
	{
		int32_t multiclicktime = 750;
		int32_t max = mouse_buttons;
		if (max > MENU_CURSOR_BUTTON_MAX) max = MENU_CURSOR_BUTTON_MAX;

		for (i=0 ; i<max ; i++)
		{
			if ( (mstate & (1<<i)) && !(mouse_oldbuttonstate & (1<<i)))
			{	//mouse press down
				if (time-cursor.buttontime[i] < multiclicktime)
					cursor.buttonclicks[i] += 1;
				else
					cursor.buttonclicks[i] = 1;

				if (cursor.buttonclicks[i]>max)
					cursor.buttonclicks[i] = max;

				cursor.buttontime[i] = time;

				cursor.buttondown[i] = true;
				cursor.buttonused[i] = false;
				cursor.mouseaction = true;
			}
			else if ( !(mstate & (1<<i)) &&	(mouse_oldbuttonstate & (1<<i)) )
			{	//mouse let go
				cursor.buttondown[i] = false;
				cursor.buttonused[i] = false;
				cursor.mouseaction = true;
			}
		}			
	}	

	mouse_oldbuttonstate = mstate;
}


/*
===========
IN_MouseMove
===========
*/
void IN_MouseMove (usercmd_t *cmd)
{
	int32_t		mx, my;
	
	if (!autosensitivity)
		autosensitivity = Cvar_Get ("autosensitivity", "1", CVAR_ARCHIVE);

	if (!mouseactive)
		return;
	if (!RelativeMouse)
	{
		// find mouse movement
		SDL_GetMouseState(&mx,&my);
		// force the mouse to the center, so there's room to move
		SDL_WarpMouseInWindow (mainWindow, window_center_x, window_center_y);

		mx -= window_center_x;
		my -= window_center_y;
	}
	else
	{
		SDL_GetRelativeMouseState(&mx,&my);
	}

	if (m_filter->value)
	{
		mouse_x = (int32_t) ((mx + old_mouse_x) * 0.5);
		mouse_y = (int32_t) ((my + old_mouse_y) * 0.5);
	}
	else
	{
		mouse_x = mx;
		mouse_y = my;
	}

	old_mouse_x = mx;
	old_mouse_y = my;

	//now to set the menu cursor
	if (cls.key_dest == key_menu)
	{
		cursor.oldx = cursor.x;
		cursor.oldy = cursor.y;

		cursor.x += mouse_x * menu_sensitivity->value;
		cursor.y += mouse_y * menu_sensitivity->value;

		if (cursor.x!=cursor.oldx || cursor.y!=cursor.oldy)
			cursor.mouseaction = true;

		if (cursor.x < 0) cursor.x = 0;
		if (cursor.x > viddef.width) cursor.x = viddef.width;
		if (cursor.y < 0) cursor.y = 0;
		if (cursor.y > viddef.height) cursor.y = viddef.height;
	}
	else
	{

		cursor.oldx = 0;
		cursor.oldy = 0;

		//psychospaz - zooming in preserves sensitivity
		if (autosensitivity->value)
		{
			mouse_x *= sensitivity->value * (cl.refdef.fov_x/90.0);
			mouse_y *= sensitivity->value * (cl.refdef.fov_y/90.0);
		}
		else
		{
			mouse_x *= sensitivity->value;
			mouse_y *= sensitivity->value;
		}

//		Com_Printf("(%i,%i) (%i,%i)\n",mx,my,mouse_x,mouse_y);

		// add mouse X/Y movement to cmd
		if ( (in_strafe.state & 1) || (lookstrafe->value && mlooking ))
			cmd->sidemove += m_side->value * mouse_x;
		else
			cl.in_delta[YAW] -= m_yaw->value * mouse_x;

		if ( (mlooking || freelook->value) && !(in_strafe.state & 1))
		{
			cl.in_delta[PITCH] += m_pitch->value * mouse_y;
		}
		else
		{
			cmd->forwardmove -= m_forward->value * mouse_y;
		}
	}

}


/*
=========================================================================

VIEW CENTERING

=========================================================================
*/


/*
===========
IN_Init
===========
*/
void IN_Init (void)
{
	Com_Printf("\n------- Input Initialization -------\n");

	// mouse variables
	autosensitivity			= Cvar_Get ("autosensitivity",			"1",		CVAR_ARCHIVE);
	m_noaccel				= Cvar_Get ("m_noaccel",				"0",		CVAR_ARCHIVE); //sul  enables mouse acceleration XP fix?
	m_filter				= Cvar_Get ("m_filter",					"0",		0);
	in_mouse				= Cvar_Get ("in_mouse",					"1",		CVAR_ARCHIVE);
	in_relativemouse		= Cvar_Get ("in_relativemouse",			"1",		CVAR_ARCHIVE);
	// joystick variables
	in_controller			= Cvar_Get ("in_controller",			"1",		CVAR_ARCHIVE);

	// centering
	v_centermove			= Cvar_Get ("v_centermove",				"0.15",		0);
	v_centerspeed			= Cvar_Get ("v_centerspeed",			"500",		0);

	Cmd_AddCommand ("+mlook", IN_MLookDown);
	Cmd_AddCommand ("-mlook", IN_MLookUp);


	IN_StartupMouse ();
	IN_ControllerInit ();
    Com_Printf("------------------------------------\n");
    
}

/*
===========
IN_Shutdown
===========
*/
void IN_Shutdown (void)
{
	IN_DeactivateMouse ();
	//MW_Shutdown(); // Logitech mouse support
}


/*
===========
IN_Activate

Called when the main window gains or loses focus.
The window may have been destroyed and recreated
between a deactivate and an activate.
===========
*/
void IN_Activate (qboolean active)
{

	in_appactive = active;
	mouseactive = !active;		// force a new window check or turn off
	if (!active)
		Key_ClearStates();
}


/*
==================
IN_Frame

Called every frame, even if not generating commands
==================
*/
void IN_Frame (void)
{
	if (!mouseinitialized)
		return;

	if (!in_mouse || !in_appactive)
	{
		IN_DeactivateMouse ();
		return;
	}

	//	if ( !cl.refresh_prepped
	//		|| cls.key_dest == key_console
	//		|| cls.key_dest == key_menu)
	//Knightmare- added Psychospaz's mouse menu support
	if ( (!cl.refresh_prepped && cls.key_dest != key_menu) || cls.consoleActive) //mouse used in menus...
	{
		// temporarily deactivate if in fullscreen
		if (Cvar_VariableInteger("vid_fullscreen") == 0 && !(vr_enabled->value && vr_force_fullscreen->value))
		{
			IN_DeactivateMouse ();
			return;
		}
	}

	IN_ActivateMouse ();
}

/*
===========
IN_Move
===========
*/
void IN_Move (usercmd_t *cmd)
{
	if (!ActiveApp)
		return;

	if (vr_enabled->value)
	{
		VR_IN_Move(cmd);
	}

	IN_MouseMove (cmd);
//	Sys_SendKeyEvents();

	// Knightmare- added Psychospaz's mouse support
	if (cls.key_dest == key_menu && !cls.consoleActive) // Knightmare added
		UI_Think_MouseCursor();


	switch((int32_t) in_controller->value)
	{
	case ControllerSDLController:
		IN_ControllerMove (cmd);
		return;
	default:
		return;
	}

}


/*
===================
IN_ClearStates
===================
*/
void IN_ClearStates (void)
{
	mx_accum = 0;
	my_accum = 0;
	mouse_oldbuttonstate = 0;
}



/*
===========
IN_Commands
===========
*/
void IN_Commands (void)
{
	if (vr_enabled->value)
	{
		VR_InputCommands();
	}

	switch((int32_t) in_controller->value)
	{
	case ControllerSDLController:
		IN_ControllerCommands ();
		return;
	default:
		return;
	}
}
