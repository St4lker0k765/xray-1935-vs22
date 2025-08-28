#include "stdafx.h"
#pragma hdrstop

#include "IGame_Persistent.h"
#include "environment.h"

ENGINE_API	IGame_Persistent*		g_pGamePersistent	= NULL;

IGame_Persistent::IGame_Persistent	()
{
	Device.seqAppCycleStart.Add		(this);
	Device.seqAppCycleEnd.Add		(this);
	Device.seqFrame.Add				(this,REG_PRIORITY_HIGH+1);
	Device.seqDevCreate.Add			(this);
	Device.seqDevDestroy.Add		(this);

#ifndef _EDITOR
	pEnvironment = new CEnvironment();
#endif
}

IGame_Persistent::~IGame_Persistent	()
{
	Device.seqDevCreate.Remove		(this);
	Device.seqDevDestroy.Remove		(this);
	Device.seqFrame.Remove			(this);
	Device.seqAppCycleStart.Remove	(this);
	Device.seqAppCycleEnd.Remove	(this);

#ifndef _EDITOR
	xr_delete(pEnvironment);
#endif
}

void IGame_Persistent::OnAppCycleStart()
{
#ifndef _EDITOR
	ObjectPool.load					();
#endif
	pEnvironment->load				();

	if (strstr(Core.Params,"-dedicated"))	bDedicatedServer	= TRUE;
	else									bDedicatedServer	= FALSE;
}

void IGame_Persistent::OnAppCycleEnd()
{
#ifndef _EDITOR
	ObjectPool.unload				();
#endif
}

void IGame_Persistent::OnFrame		()
{
	pEnvironment->OnFrame				();
}

void IGame_Persistent::OnDeviceCreate()
{
	pEnvironment->OnDeviceCreate		();
}

void IGame_Persistent::OnDeviceDestroy()
{
	pEnvironment->OnDeviceDestroy		();
}

