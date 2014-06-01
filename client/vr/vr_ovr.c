#include "include/vr.h"
#include "include/vr_ovr.h"
#include "OVR_CAPI.h"
#include <SDL.h>

void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos);
void VR_OVR_GetState(vr_param_t *state);
void VR_OVR_Frame();
int32_t VR_OVR_Enable();
void VR_OVR_Disable();
int32_t VR_OVR_Init();
void VR_OVR_Shutdown();
int32_t VR_OVR_getOrientation(float euler[3]);
void VR_OVR_ResetHMDOrientation();
int32_t VR_OVR_SetPredictionTime(float time);

void VR_OVR_CalcRenderParam();
float VR_OVR_GetDistortionScale();

cvar_t *vr_ovr_driftcorrection;
cvar_t *vr_ovr_scale;
cvar_t *vr_ovr_debug;
cvar_t *vr_ovr_distortion;
cvar_t *vr_ovr_lensdistance;
cvar_t *vr_ovr_autoscale;
cvar_t *vr_ovr_autolensdistance;
cvar_t *vr_ovr_filtermode;
cvar_t *vr_ovr_supersample;
cvar_t *vr_ovr_latencytest;
cvar_t *vr_ovr_enable;


ovr_attrib_t ovrConfig = {0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0,}, { 0, 0, 0, 0,}};

vr_param_t ovrState = {0, 0, 0, 0, 0, 0};

ovrHmd hmd;
ovrHmdDesc hmdDesc; 

static double prediction_time;

hmd_interface_t hmd_rift = {
	HMD_RIFT,
	VR_OVR_Init,
	VR_OVR_Shutdown,
	VR_OVR_Enable,
	VR_OVR_Disable,
	VR_OVR_GetHMDPos,
	VR_OVR_Frame,
	VR_OVR_ResetHMDOrientation,
	VR_OVR_getOrientation,
	NULL,
	VR_OVR_SetPredictionTime
};


void VR_OVR_GetFOV(float *fovx, float *fovy)
{
	// retrieve the FOV
}


void VR_OVR_GetHMDPos(int32_t *xpos, int32_t *ypos)
{
	if (hmd)
	{
		*xpos = hmdDesc.WindowsPos.x;
		*ypos = hmdDesc.WindowsPos.y;
	}
}


int32_t VR_OVR_getOrientation(float euler[3])
{
	/*
	if (vr_ovr_latencytest->value)
		LibOVR_ProcessLatencyInputs();
    return LibOVR_GetOrientation(euler);
	*/
	return 0;
}

void VR_OVR_ResetHMDOrientation()
{
	if (hmd)
		ovrHmd_ResetSensor(hmd);
}

int32_t VR_OVR_RenderLatencyTest(vec4_t color) 
{
//	return (vr_ovr_latencytest->value && LibOVR_GetLatencyTestColor(color));
	return 0;
}

void VR_OVR_CalcRenderParam()
{
/*
	if (vr_ovr_settings.initialized)
	{	
		float *dk = vr_ovr_settings.distortion_k;
		float lsd = vr_ovr_autolensdistance->value ? vr_ovr_settings.lens_separation_distance : vr_ovr_lensdistance->value / 1000.0;
		float h = 1.0f - (2.0f * lsd) / vr_ovr_settings.h_screen_size;

		if (lsd > 0)
		{
			float le = (-1.0f - h) * (-1.0f - h);
			float re = (-1.0f + h) * (-1.0f + h);

			if (le > re)
			{
				ovrConfig.minScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
				ovrConfig.maxScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
			} else
			{
				ovrConfig.maxScale = (dk[0] + dk[1] * re + dk[2] * re * re + dk[3] * re * re * re);
				ovrConfig.minScale = (dk[0] + dk[1] * le + dk[2] * le * le + dk[3] * le * le * le);
			}
		} else {
			ovrConfig.maxScale = 2.0;
			ovrConfig.minScale = 1.0;
		}

		if (ovrConfig.maxScale >= 2.0)
			ovrConfig.maxScale = 2.0;

		ovrConfig.ipd = vr_ovr_settings.interpupillary_distance;
		ovrConfig.aspect = vr_ovr_settings.h_resolution / (2.0f * vr_ovr_settings.v_resolution);
		ovrConfig.hmdWidth = vr_ovr_settings.h_resolution;
		ovrConfig.hmdHeight = vr_ovr_settings.v_resolution;

		memcpy(ovrConfig.chrm, vr_ovr_settings.chrom_abr, sizeof(float) * 4);
		memcpy(ovrConfig.dk, vr_ovr_settings.distortion_k, sizeof(float) * 4);
		strncpy(ovrConfig.deviceString,vr_ovr_settings.deviceString,sizeof(ovrConfig.deviceString));
		ovrConfig.projOffset = h;
	}
	*/
}


float VR_OVR_GetDistortionScale()
{
	/*
	if (vr_ovr_distortion->value < 0)
		return 1.0f;

	switch ((int32_t) vr_ovr_autoscale->value)
	{
	case 1:
		return ovrConfig.minScale;
	case 2:
		return ovrConfig.maxScale;
	default:
		return vr_ovr_scale->value;
	}*/

	return 0;
}

int32_t VR_OVR_SetPredictionTime(float time)
{
	prediction_time = time;
	Com_Printf("VR_OVR: Set HMD Prediction time to %.1fms\n", time);
	return 1;
}

void SCR_CenterAlert (char *str);
void VR_OVR_Frame()
{

/*
	if (vr_ovr_latencytest->value && LibOVR_IsLatencyTesterAvailable())
	{
			const char *results = LibOVR_ProcessLatencyResults();
			if (results)
			{
				float ms;
				Com_Printf("VR_OVR: %s\n",results);		
				if (sscanf(results,"RESULT=%f ",&ms))
				{
					char buffer[64];
					SDL_snprintf(buffer,sizeof(buffer),"%.1fms latency",ms);
					SCR_CenterAlert(buffer);
					Cvar_SetInteger("vr_prediction",(int) ms);
				}
			}
	} 
*/

}


int32_t VR_OVR_Enable()
{
	char string[6];
	int32_t failure = 0;
	if (!vr_ovr_enable->value)
		return 0;
	hmd = ovrHmd_Create(0);

	if (!hmd)
	{
		Com_Printf("VR_OVR: Error, no HMD detected!\n");
		failure = 1;
	}
	else {
		Com_Printf("VR_OVR: Initializing HMD:");
		Com_Printf(" ok!\n");
	}

	if (failure && vr_ovr_debug->value)
	{
		ovrHmdType temp = ovrHmd_None;
		switch((int) vr_ovr_debug->value)
		{
			default:
				temp = ovrHmd_DK2;
				break;
			case 1:
				temp = ovrHmd_DK1;
				break;
			case 2:
				temp = ovrHmd_DKHD;
				break;
		}
		hmd = ovrHmd_CreateDebug(temp);
	}

	if (!hmd)
		return 0;

	ovrHmd_GetDesc(hmd, &hmdDesc);
	

	Com_Printf("...detected HMD %s with %ux%u resolution\n", hmdDesc.ProductName,hmdDesc.Resolution.w, hmdDesc.Resolution.h);
	
	if (ovrHmd_StartSensor(hmd,(ovrSensorCap_Orientation | ovrSensorCap_YawCorrection), ovrSensorCap_Orientation))
	{
		Com_Printf("...successfully started sensor\n");
	}

	if (vr_ovr_scale->value < 1)
		Cvar_SetValue("vr_ovr_scale", 1.0);

	VR_OVR_CalcRenderParam();

	/* 8/30/2013 - never auto enable this.
	// TODO: use pixel density instead
	if (vr_ovr_settings.v_resolution > 800)
		Cvar_SetInteger("vr_hud_transparency", 1);
	*/
	Com_Printf("...calculated %.2f minimum distortion scale\n", ovrConfig.minScale);
	Com_Printf("...calculated %.2f maximum distortion scale\n", ovrConfig.maxScale);

	if (hmdDesc.SensorCaps & ovrHmdCap_LatencyTest)
	{
		Com_Printf("...found latency tester\n");
		Cvar_SetInteger("vr_ovr_latencytest",1);
	}
	
	VR_OVR_Disable();
	return 0;
}

void VR_OVR_Disable()
{
	if (hmd)
	{
		ovrHmd_StopSensor(hmd);
		ovrHmd_Destroy(hmd);
	}
	hmd = NULL;
}


int32_t VR_OVR_Init()
{
	ovrBool init = ovr_Initialize();
	vr_ovr_supersample = Cvar_Get("vr_ovr_supersample","1.0",CVAR_ARCHIVE);
	vr_ovr_scale = Cvar_Get("vr_ovr_scale","1.0",CVAR_ARCHIVE);
	vr_ovr_lensdistance = Cvar_Get("vr_ovr_lensdistance","-1",CVAR_ARCHIVE);
	vr_ovr_latencytest = Cvar_Get("vr_ovr_latencytest","0",0);
	vr_ovr_enable = Cvar_Get("vr_ovr_enable","1",CVAR_ARCHIVE);
	vr_ovr_driftcorrection = Cvar_Get("vr_ovr_driftcorrection","1",CVAR_ARCHIVE);
	vr_ovr_distortion = Cvar_Get("vr_ovr_distortion","1",CVAR_ARCHIVE);
	vr_ovr_debug = Cvar_Get("vr_ovr_debug","0",CVAR_ARCHIVE);
	vr_ovr_filtermode = Cvar_Get("vr_ovr_filtermode","0",CVAR_ARCHIVE);
	vr_ovr_autoscale = Cvar_Get("vr_ovr_autoscale","1",CVAR_ARCHIVE);
	vr_ovr_autolensdistance = Cvar_Get("vr_ovr_autolensdistance","1",CVAR_ARCHIVE);

	if (!init)
	{
		Com_Printf("VR_OVR: Fatal error: could not initialize LibOVR!\n");
		return 0;
	}

	Com_Printf("VR_OVR: Oculus Rift support initialized...\n");

	return 0;
}

void VR_OVR_Shutdown()
{
	ovr_Shutdown();
}


