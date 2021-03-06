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

// ui_options_vr.c -- the interface options menu

#include <ctype.h>
#ifdef _WIN32
#include <io.h>
#endif
#include "../client.h"
#include "include/ui_local.h"

/*
=======================================================================

INTERFACE MENU

=======================================================================
*/

static menuframework_s	s_options_vr_advanced_menu;
static menuseparator_s	s_options_vr_advanced_header;
static menulist_s		s_options_vr_advanced_autoenable_box;
static menulist_s		s_options_vr_advanced_force_fullscreen;
static menufield_s		s_options_vr_advanced_supersampling;
static menulist_s		s_options_vr_advanced_chroma_box;
static menufield_s		s_options_vr_advanced_prediction_field;
static menuslider_s		s_options_vr_advanced_hud_depth_slider;
static menuslider_s		s_options_vr_advanced_hud_fov_slider;
static menulist_s		s_options_vr_advanced_hud_resolution_box;
static menulist_s		s_options_vr_advanced_hudtrans_box;
static menulist_s		s_options_vr_advanced_hudbounce_box;
static menulist_s		s_options_vr_advanced_positiontracking_box;
static menulist_s		s_options_vr_advanced_neckmodel_box;
static menufield_s		s_options_vr_advanced_neckmodel_up_field;
static menufield_s		s_options_vr_advanced_neckmodel_forward_field;
static menulist_s		s_options_vr_advanced_autofov_box;

static menuaction_s		s_options_vr_advanced_defaults_action;
static menuaction_s		s_options_vr_advanced_back_action;

extern cvar_t *vr_prediction;

extern cvar_t *vr_hud_bounce_falloff;
extern cvar_t *vr_neckmodel_up;
extern cvar_t *vr_neckmodel_forward;

static void AutoFunc( void *unused )
{
	Cvar_SetInteger("vr_autoenable",s_options_vr_advanced_autoenable_box.curvalue);
}

static void BounceFunc( void *unused )
{
	Cvar_SetInteger("vr_hud_bounce",s_options_vr_advanced_hudbounce_box.curvalue);
}

static void TransFunc( void *unused )
{
	Cvar_SetInteger("vr_hud_transparency",s_options_vr_advanced_hudtrans_box.curvalue);
}

static void HUDFunc( void *unused )
{
	Cvar_SetValue( "vr_hud_depth", s_options_vr_advanced_hud_depth_slider.curvalue / 20.0f);
	Cvar_SetInteger( "vr_hud_fov", s_options_vr_advanced_hud_fov_slider.curvalue);
}

static void NeckFunc( void *unused )
{
	Cvar_SetInteger( "vr_neckmodel", s_options_vr_advanced_neckmodel_box.curvalue );
}

static void NeckmodelFunc ( void *unused )
{
	float temp;
	char string[10];

	temp = ClampCvar(0,1,atof(s_options_vr_advanced_neckmodel_up_field.buffer));
	strncpy(string, va("%.3f",temp), sizeof(string));
	Cvar_Set("vr_neckmodel_up", string);

	temp = ClampCvar(0,1,atof(s_options_vr_advanced_neckmodel_forward_field.buffer));
	strncpy(string, va("%.3f",temp), sizeof(string));
	Cvar_Set("vr_neckmodel_forward", string);

	strcpy( s_options_vr_advanced_neckmodel_up_field.buffer, vr_neckmodel_up->string );
	s_options_vr_advanced_neckmodel_up_field.cursor = strlen( vr_neckmodel_up->string );
	strcpy( s_options_vr_advanced_neckmodel_forward_field.buffer, vr_neckmodel_forward->string );
	s_options_vr_advanced_neckmodel_forward_field.cursor = strlen( vr_neckmodel_forward->string );
}

static void ChromaFunc( void *unused )
{
	Cvar_SetInteger( "vr_chromatic", s_options_vr_advanced_chroma_box.curvalue);
}

static void PositionFunc( void *unused )
{
	Cvar_SetInteger( "vr_positiontracking", s_options_vr_advanced_positiontracking_box.curvalue);
}



static void AutoFOVFunc(void *unused)
{
	Cvar_SetInteger("vr_autofov", s_options_vr_advanced_autofov_box.curvalue);
}


static void CustomPredictionFunc(void *unused)
{
	float temp;

	temp = ClampCvar(0,75,atof(s_options_vr_advanced_prediction_field.buffer));
	Cvar_SetInteger("vr_prediction",temp);
	strcpy( s_options_vr_advanced_prediction_field.buffer, vr_prediction->string );
	s_options_vr_advanced_prediction_field.cursor = strlen( vr_prediction->string );
}

static void ResolutionFunc(void *unused)
{
	int temp = s_options_vr_advanced_hud_resolution_box.curvalue;
	int height, width;
	height = 480;
	width = 640;

	if (temp > 1)
	{
		width = 1024;
		height = 768;
	}
	else if (temp > 0)
	{
		width = 800;
		height = 600;
	}

	Cvar_SetInteger("vr_hud_width",width);
	Cvar_SetInteger("vr_hud_height",height);
}

static void ForceFullscreenFunc(void *unused)
{
	Cvar_SetInteger("vr_force_fullscreen",s_options_vr_advanced_force_fullscreen.curvalue);
}

static void SupersamplingFunc(void *unused)
{
	float temp;
	char string[10];

	temp = ClampCvar(0.4, 3, atof(s_options_vr_advanced_supersampling.buffer));
	strncpy(string, va("%.2f", temp), sizeof(string));
	Cvar_Set("vr_supersampling", string);
}


static void VRAdvSetMenuItemValues( void )
{
	s_options_vr_advanced_autofov_box.curvalue = ( ClampCvar(0,1,Cvar_VariableValue("vr_autofov")));

	s_options_vr_advanced_autoenable_box.curvalue = ( ClampCvar(0,1,Cvar_VariableInteger("vr_autoenable") ) );
	s_options_vr_advanced_hud_depth_slider.curvalue = ( Cvar_VariableValue("vr_hud_depth") * 20.0f);
	s_options_vr_advanced_hud_fov_slider.curvalue = ( Cvar_VariableValue("vr_hud_fov") );
	s_options_vr_advanced_hudtrans_box.curvalue = ( ClampCvar(0,1,Cvar_VariableInteger("vr_hud_transparency") ) );
	s_options_vr_advanced_hudbounce_box.curvalue = ( ClampCvar(0,2,Cvar_VariableInteger("vr_hud_bounce") ) );
	s_options_vr_advanced_neckmodel_box.curvalue = ( ClampCvar(0,1,Cvar_VariableValue("vr_neckmodel") ) );
	s_options_vr_advanced_positiontracking_box.curvalue = ( Cvar_VariableValue("vr_positiontracking"));
	strcpy( s_options_vr_advanced_neckmodel_up_field.buffer, vr_neckmodel_up->string );
	s_options_vr_advanced_neckmodel_up_field.cursor = strlen( vr_neckmodel_up->string );
	strcpy( s_options_vr_advanced_neckmodel_forward_field.buffer, vr_neckmodel_forward->string );
	s_options_vr_advanced_neckmodel_forward_field.cursor = strlen( vr_neckmodel_forward->string );
	s_options_vr_advanced_chroma_box.curvalue = ( ClampCvar(0,1,Cvar_VariableInteger("vr_chromatic") ) );

	strcpy( s_options_vr_advanced_prediction_field.buffer, vr_prediction->string );
	s_options_vr_advanced_prediction_field.cursor = strlen( vr_prediction->string );
	{
		int height = Cvar_VariableInteger("vr_hud_height");
		int width = Cvar_VariableInteger("vr_hud_width");
		int value = 0;
		if (height > 600 || width > 800) {
			value = 2;
		}
		else if (height > 480 || width > 640) {
			value = 1;
		}
		
		s_options_vr_advanced_hud_resolution_box.curvalue = ClampCvar(0,3,value);

	}
	
	s_options_vr_advanced_force_fullscreen.curvalue = ClampCvar(0,1,Cvar_VariableInteger("vr_force_fullscreen"));
	strcpy(s_options_vr_advanced_supersampling.buffer, vr_supersampling->string);
	s_options_vr_advanced_supersampling.cursor = strlen(vr_supersampling->string);
}

static void VRAdvResetDefaultsFunc ( void *unused )
{
	Cvar_SetToDefault("vr_autofov");
	Cvar_SetToDefault("vr_autoenable");
	Cvar_SetToDefault("vr_chromatic");
	Cvar_SetToDefault("vr_hud_depth");
	Cvar_SetToDefault("vr_hud_fov");
	Cvar_SetToDefault("vr_hud_transparency");
	Cvar_SetToDefault("vr_hud_bounce");
	Cvar_SetToDefault("vr_hud_bounce_falloff");
	Cvar_SetToDefault("vr_prediction");
	Cvar_SetToDefault("vr_neckmodel");
	Cvar_SetToDefault("vr_neckmodel_up");
	Cvar_SetToDefault("vr_neckmodel_forward");
	Cvar_SetToDefault("vr_positiontracking");
	Cvar_SetToDefault("vr_hud_width");
	Cvar_SetToDefault("vr_hud_height");
	Cvar_SetToDefault("vr_force_fullscreen");
	Cvar_SetToDefault("vr_supersampling");
	VRAdvSetMenuItemValues();
}

static void VRAdvancedConfigAccept (void)
{
	NeckFunc(NULL);
	CustomPredictionFunc(NULL);
}

void Options_VR_Advanced_MenuInit ( void )
{
	static const char *yesno_names[] =
	{
		"no",
		"yes",
		0
	};

	static const char *smoothfunc_names[] =
	{
		"off",
		"simple",
		"linear",
		0
	};

	static const char *fov_names[] =
	{
		"Quake II",
		"HMD",
		0
	};

	static const char *res_names[] =
	{
		"640x480",
		"800x600",
		"1024x768",
		0
	};

	int32_t y = 3*MENU_LINE_SIZE;

	s_options_vr_advanced_menu.x = SCREEN_WIDTH*0.5;
	s_options_vr_advanced_menu.y = SCREEN_HEIGHT*0.5 - 80;
	s_options_vr_advanced_menu.nitems = 0;

	s_options_vr_advanced_header.generic.type		= MTYPE_SEPARATOR;
	s_options_vr_advanced_header.generic.name		= "vr advanced configuration";
	s_options_vr_advanced_header.generic.x		= MENU_FONT_SIZE/2 * strlen(s_options_vr_advanced_header.generic.name);
	s_options_vr_advanced_header.generic.y		= 0;

	s_options_vr_advanced_autoenable_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_advanced_autoenable_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_autoenable_box.generic.y			= y;
	s_options_vr_advanced_autoenable_box.generic.name		= "auto-enable vr mode";
	s_options_vr_advanced_autoenable_box.generic.callback	= AutoFunc;
	s_options_vr_advanced_autoenable_box.itemnames			= yesno_names;
	s_options_vr_advanced_autoenable_box.generic.statusbar	= "automatically enable VR support what starting Quake II VR";

	s_options_vr_advanced_force_fullscreen.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_force_fullscreen.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_force_fullscreen.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_force_fullscreen.generic.name			= "force fullscreen";
	s_options_vr_advanced_force_fullscreen.generic.callback		= ForceFullscreenFunc;
	s_options_vr_advanced_force_fullscreen.itemnames			= yesno_names;
	s_options_vr_advanced_force_fullscreen.generic.statusbar	= "forces Quake II VR to run in fullscreen in VR mode";

	s_options_vr_advanced_supersampling.generic.type = MTYPE_FIELD;
	s_options_vr_advanced_supersampling.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_advanced_supersampling.generic.name = "supersampling";
	s_options_vr_advanced_supersampling.generic.statusbar = "a.k.a. render target multiplier";
	s_options_vr_advanced_supersampling.generic.callback = SupersamplingFunc;
	s_options_vr_advanced_supersampling.generic.x = MENU_FONT_SIZE;
	s_options_vr_advanced_supersampling.generic.y = y += 2 * MENU_LINE_SIZE;
	s_options_vr_advanced_supersampling.length = 6;
	s_options_vr_advanced_supersampling.visible_length = 6;
	strcpy(s_options_vr_advanced_supersampling.buffer, vr_supersampling->string);
	s_options_vr_advanced_supersampling.cursor = strlen(vr_supersampling->string);

	s_options_vr_advanced_chroma_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_chroma_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_chroma_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_chroma_box.generic.name			= "chromatic correction";
	s_options_vr_advanced_chroma_box.generic.callback		= ChromaFunc;
	s_options_vr_advanced_chroma_box.itemnames			= yesno_names;
	s_options_vr_advanced_chroma_box.generic.statusbar	= "applies chromatic aberration correction to the distortion shader";

	s_options_vr_advanced_autofov_box.generic.type = MTYPE_SPINCONTROL;
	s_options_vr_advanced_autofov_box.generic.x = MENU_FONT_SIZE;
	s_options_vr_advanced_autofov_box.generic.y = y += MENU_LINE_SIZE;
	s_options_vr_advanced_autofov_box.generic.name = "field of view";
	s_options_vr_advanced_autofov_box.generic.callback = AutoFOVFunc;
	s_options_vr_advanced_autofov_box.itemnames = fov_names;
	s_options_vr_advanced_autofov_box.generic.statusbar = "choose whether to use auto or custom field of view";

	s_options_vr_advanced_prediction_field.generic.type = MTYPE_FIELD;
	s_options_vr_advanced_prediction_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_advanced_prediction_field.generic.name = "motion prediction";
	s_options_vr_advanced_prediction_field.generic.statusbar	= "sets the amount of motion prediction to apply in milliseconds";
	s_options_vr_advanced_prediction_field.generic.callback = CustomPredictionFunc;
	s_options_vr_advanced_prediction_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_advanced_prediction_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_prediction_field.length	= 5;
	s_options_vr_advanced_prediction_field.visible_length = 5;
	strcpy( s_options_vr_advanced_prediction_field.buffer, vr_prediction->string );
	s_options_vr_advanced_prediction_field.cursor = strlen( vr_prediction->string );


	s_options_vr_advanced_hud_depth_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_advanced_hud_depth_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hud_depth_slider.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_hud_depth_slider.generic.name		= "hud depth";
	s_options_vr_advanced_hud_depth_slider.generic.callback	= HUDFunc;
	s_options_vr_advanced_hud_depth_slider.minvalue			= 10;
	s_options_vr_advanced_hud_depth_slider.maxvalue			= 100;
	s_options_vr_advanced_hud_depth_slider.generic.statusbar	= "changes hud depth";

	s_options_vr_advanced_hud_fov_slider.generic.type		= MTYPE_SLIDER;
	s_options_vr_advanced_hud_fov_slider.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hud_fov_slider.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_hud_fov_slider.generic.name		= "hud fov";
	s_options_vr_advanced_hud_fov_slider.generic.callback	= HUDFunc;
	s_options_vr_advanced_hud_fov_slider.minvalue			= 30;
	s_options_vr_advanced_hud_fov_slider.maxvalue			= 90;
	s_options_vr_advanced_hud_fov_slider.generic.statusbar	= "changes size of the hud";

	s_options_vr_advanced_hud_resolution_box.generic.type		= MTYPE_SPINCONTROL;
	s_options_vr_advanced_hud_resolution_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hud_resolution_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_hud_resolution_box.generic.name		= "hud resolution";
	s_options_vr_advanced_hud_resolution_box.generic.callback	= ResolutionFunc;
	s_options_vr_advanced_hud_resolution_box.itemnames			= res_names;
	s_options_vr_advanced_hud_resolution_box.generic.statusbar	= "sets the resolution of the hud";

	s_options_vr_advanced_hudtrans_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_hudtrans_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hudtrans_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_hudtrans_box.generic.name			= "transparent hud";
	s_options_vr_advanced_hudtrans_box.generic.callback		= TransFunc;
	s_options_vr_advanced_hudtrans_box.itemnames			= yesno_names;
	s_options_vr_advanced_hudtrans_box.generic.statusbar	= "enables or disables hud transparency";

	s_options_vr_advanced_hudbounce_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_hudbounce_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_hudbounce_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_hudbounce_box.generic.name			= "hud bouncing";
	s_options_vr_advanced_hudbounce_box.generic.callback		= BounceFunc;
	s_options_vr_advanced_hudbounce_box.itemnames			= smoothfunc_names;
	s_options_vr_advanced_hudbounce_box.generic.statusbar	= "enables or disables hud counter rotation";
	
	s_options_vr_advanced_positiontracking_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_positiontracking_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_positiontracking_box.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_positiontracking_box.generic.name			= "enable position tracking";
	s_options_vr_advanced_positiontracking_box.generic.callback		= PositionFunc;
	s_options_vr_advanced_positiontracking_box.itemnames			= yesno_names;
	s_options_vr_advanced_positiontracking_box.generic.statusbar	= "enable or disable position tracking if available";

	s_options_vr_advanced_neckmodel_box.generic.type			= MTYPE_SPINCONTROL;
	s_options_vr_advanced_neckmodel_box.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_neckmodel_box.generic.y			= y+=MENU_LINE_SIZE;
	s_options_vr_advanced_neckmodel_box.generic.name			= "enable head/neck model";
	s_options_vr_advanced_neckmodel_box.generic.callback		= NeckFunc;
	s_options_vr_advanced_neckmodel_box.itemnames			= yesno_names;
	s_options_vr_advanced_neckmodel_box.generic.statusbar	= "enable or disable the head/neck model";

	s_options_vr_advanced_neckmodel_up_field.generic.type = MTYPE_FIELD;
	s_options_vr_advanced_neckmodel_up_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_advanced_neckmodel_up_field.generic.name = "neck length";
	s_options_vr_advanced_neckmodel_up_field.generic.statusbar	= "sets the length of the user's neck in meters";
	s_options_vr_advanced_neckmodel_up_field.generic.callback = NeckmodelFunc;
	s_options_vr_advanced_neckmodel_up_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_advanced_neckmodel_up_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_neckmodel_up_field.length	= 6;
	s_options_vr_advanced_neckmodel_up_field.visible_length = 6;
	strcpy( s_options_vr_advanced_neckmodel_up_field.buffer, vr_neckmodel_up->string );
	s_options_vr_advanced_neckmodel_up_field.cursor = strlen( vr_neckmodel_up->string );
	
	s_options_vr_advanced_neckmodel_forward_field.generic.type = MTYPE_FIELD;
	s_options_vr_advanced_neckmodel_forward_field.generic.flags = QMF_LEFT_JUSTIFY;
	s_options_vr_advanced_neckmodel_forward_field.generic.name = "forward eye distance";
	s_options_vr_advanced_neckmodel_forward_field.generic.statusbar	= "sets the forward distance for the eyes in meters";
	s_options_vr_advanced_neckmodel_forward_field.generic.callback = NeckmodelFunc;
	s_options_vr_advanced_neckmodel_forward_field.generic.x		= MENU_FONT_SIZE;
	s_options_vr_advanced_neckmodel_forward_field.generic.y		= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_neckmodel_forward_field.length	= 6;
	s_options_vr_advanced_neckmodel_forward_field.visible_length = 6;
	strcpy( s_options_vr_advanced_neckmodel_forward_field.buffer, vr_neckmodel_forward->string );
	s_options_vr_advanced_neckmodel_forward_field.cursor = strlen( vr_neckmodel_forward->string );

	s_options_vr_advanced_defaults_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_defaults_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_defaults_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_defaults_action.generic.name		= "reset defaults";
	s_options_vr_advanced_defaults_action.generic.callback	= VRAdvResetDefaultsFunc;
	s_options_vr_advanced_defaults_action.generic.statusbar	= "resets all advanced virtual reality settings to internal defaults";

	s_options_vr_advanced_back_action.generic.type		= MTYPE_ACTION;
	s_options_vr_advanced_back_action.generic.x			= MENU_FONT_SIZE;
	s_options_vr_advanced_back_action.generic.y			= y+=2*MENU_LINE_SIZE;
	s_options_vr_advanced_back_action.generic.name		= "back to options";
	s_options_vr_advanced_back_action.generic.callback	= UI_BackMenu;

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_header );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_autoenable_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_force_fullscreen );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_supersampling );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_chroma_box );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_autofov_box);

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_prediction_field );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hud_depth_slider );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hud_fov_slider );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hud_resolution_box );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hudtrans_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_hudbounce_box );
	
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_positiontracking_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_neckmodel_box );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_neckmodel_up_field );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_neckmodel_forward_field );

	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_defaults_action );
	Menu_AddItem( &s_options_vr_advanced_menu, ( void * ) &s_options_vr_advanced_back_action );

	VRAdvSetMenuItemValues();
}

void Options_VR_Advanced_MenuDraw (void)
{
	Menu_DrawBanner( "m_banner_options" );
	Menu_AdjustCursor( &s_options_vr_advanced_menu, 1 );
	Menu_Draw( &s_options_vr_advanced_menu );
}

const char *Options_VR_Advanced_MenuKey( int32_t key )
{
	return Default_MenuKey( &s_options_vr_advanced_menu, key );
}

void M_Menu_Options_VR_Advanced_f (void)
{
	Options_VR_Advanced_MenuInit();
	UI_PushMenu ( Options_VR_Advanced_MenuDraw, Options_VR_Advanced_MenuKey, VRAdvancedConfigAccept );
}
