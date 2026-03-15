#ifndef xr_device
#define xr_device
#pragma once

// Note:
// ZNear - always 0.0f
// ZFar  - always 1.0f

class	ENGINE_API	CResourceManager;
class	ENGINE_API	CGammaControl;

#include "pure.h"
#include "hw.h"
#include "ftimer.h"
#include "stats.h"
#include "xr_effgamma.h"
#include "shader.h"
#include "R_Backend.h"

#define VIEWPORT_NEAR 0.2f

// refs
class ENGINE_API CRenderDevice 
{
private:
	// Main objects used for creating and rendering the 3D scene
	u32										m_dwWindowStyle;
	RECT									m_rcWindowBounds;
	RECT									m_rcWindowClient;

	u32										Timer_MM_Delta;

	CTimer_paused							Timer;
	CTimer_paused							TimerGlobal;
	CTimer									TimerMM;

	void									_Create		(LPCSTR shName);
	void									_Destroy	(BOOL	bKeepTextures);
	void									_SetupStates();

public:
	HWND									m_hWnd;
	LRESULT									MsgProc		(HWND,UINT,WPARAM,LPARAM);

	u32										dwFrame;
	u32										dwPrecacheFrame;
	u32										dwPrecacheTotal;

	u32										dwWidth, dwHeight;
	float									fWidth_2, fHeight_2;
	BOOL									bReady;
	BOOL									bActive;

public:
	// Registrators
	CRegistrator	<pureDeviceDestroy	 >	seqDevDestroy;
	CRegistrator	<pureDeviceCreate	 >	seqDevCreate;
	CRegistrator	<pureFrame			 >	seqFrame;
	CRegistrator	<pureFrame			 >	seqFrameMT;
	CRegistrator	<pureRender			 >	seqRender;
	CRegistrator	<pureAppActivate	 >	seqAppActivate;
	CRegistrator	<pureAppDeactivate	 >	seqAppDeactivate;
	CRegistrator	<pureAppCycleStart	 >	seqAppCycleStart;
	CRegistrator	<pureAppCycleEnd	 >	seqAppCycleEnd;

	// Dependent classes
	CResourceManager*						Resources;	  
	CStats									Statistic;
	CGammaControl							Gamma;

	// Engine flow-control
	float									fTimeDelta;
	float									fTimeGlobal;
	u32										dwTimeDelta;
	u32										dwTimeGlobal;
	u32										dwTimeContinual;

	// Cameras & projection
	Fvector									vCameraPosition;
	Fvector									vCameraDirection;
	Fvector									vCameraTop;
	Fvector									vCameraRight;
	Fmatrix									mView;
	Fmatrix									mProject;
	Fmatrix									mFullTransform;
	Fmatrix									mInvFullTransform;
	float									fFOV;
	float									fASPECT;

	CRenderDevice()
	{
		m_hWnd				= NULL;
		bActive				= FALSE;
		bReady				= FALSE;

		Timer.Start();
		TimerGlobal.Start();
		TimerMM.Start();
	};

	// Scene control
	void PreCache							(u32 frames);
	BOOL Begin								();
	void Clear								();
	void End								();
	void FrameMove							();

	void overdrawBegin						();
	void overdrawEnd						();

	// Mode control
	void DumpFlags							();

	IC CTimer_paused* GetTimerGlobal		()
	{
		return &TimerGlobal;
	}

	u32 TimerAsync							()
	{
		return TimerGlobal.GetElapsed_ms();
	}

	u32 TimerAsync_MMT						()
	{
		return TimerMM.GetElapsed_ms() + Timer_MM_Delta;
	}

	// Creation & Destroying
	void Create								(void);
	void Run								(void);
	void Destroy							(void);
	void Reset								(void);

	void Initialize							(void);
	void ShutDown							(void);

	void time_factor(const float& time_factor)
	{
		Timer.time_factor(time_factor);
		TimerGlobal.time_factor(time_factor);
	}

	IC	const float& time_factor() const
	{
		VERIFY(Timer.time_factor() == TimerGlobal.time_factor());
		return					(Timer.time_factor());
	}

	// Multi-threading
	CRITICAL_SECTION	mt_csEnter;
	CRITICAL_SECTION	mt_csLeave;
	volatile BOOL		mt_bMustExit;
};

extern ENGINE_API CRenderDevice Device;
extern ENGINE_API bool g_bBenchmark;

#include "R_Backend_Runtime.h"

#endif
