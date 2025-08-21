#include "stdafx.h"
#pragma hdrstop

#include <msacm.h>

#include "soundrender_core.h"
#include "soundrender_source.h"

//	SEEK_SET	0	File beginning
//	SEEK_CUR	1	Current file pointer position
//	SEEK_END	2	End-of-file
int ov_seek_func(void *datasource, s64 offset, int whence)	
{
	switch (whence){
	case SEEK_SET: ((IReader*)datasource)->seek((int)offset);	 break;
	case SEEK_CUR: ((IReader*)datasource)->advance((int)offset); break;
	case SEEK_END: ((IReader*)datasource)->seek((int)offset + ((IReader*)datasource)->length()); break;
	}
	return 0; 
}
size_t ov_read_func(void *ptr, size_t size, size_t nmemb, void *datasource)
{ 
	IReader* F			= (IReader*)datasource; 
	size_t exist_block	= _max(0ul,iFloor(F->elapsed()/(float)size));
	size_t read_block	= _min(exist_block,nmemb);
	F->r				(ptr,(int)(read_block*size));	
	return read_block;
}
int ov_close_func(void *datasource)									
{	
	return 0; 
}
long ov_tell_func(void *datasource)									
{	
	return ((IReader*)datasource)->tell(); 
}

void CSoundRender_Source::LoadWave(LPCSTR pName, BOOL b3D)
{
	wave = FS.r_open(pName);
	R_ASSERT3(wave && wave->length(), "Can't open wave file:", pName);

	// читаем заголовок WAV
	WAVEFORMATEX wfxdest = SoundRender->wfm;
	wave->r(&wfxdest, sizeof(WAVEFORMATEX)); // или парсинг RIFF вручную

	R_ASSERT3(b3D ? wfxdest.nChannels == 1 : wfxdest.nChannels == 2,
		"Invalid source num channels:", pName);
	R_ASSERT3(wfxdest.nSamplesPerSec == 44100,
		"Invalid source rate:", pName);

	dwBytesTotal = wave->length();
	dwBytesPerMS = wfxdest.nAvgBytesPerSec / 1000;
	dwTimeTotal = u32(sdef_source_footer +
		(u64(dwBytesTotal) * 1000ull / u64(wfxdest.nAvgBytesPerSec)));

	m_fMinDist = 1.f;
	m_fMaxDist = 300.f;
	m_fVolume = 1.f;
	m_uGameType = 0;
}

void CSoundRender_Source::load(LPCSTR name,	BOOL b3D)
{
	string256			fn,N;
	strcpy				(N,name);
	strlwr				(N);
	if (strext(N))		*strext(N) = 0;

	fname				= N;
	_3D					= b3D;

	strconcat			(fn,N,".ogg");
	if (!FS.exist("$level$",fn))	FS.update_path	(fn,"$game_sounds$",fn);

#ifdef _EDITOR
	if (!FS.exist(fn)){ 
		FS.update_path	(fn,"$game_sounds$","$no_sound.ogg");
    }
#endif
	LoadWave			(fn,_3D);	R_ASSERT(wave);
	SoundRender->cache.cat_create	(CAT, dwBytesTotal);

	if (dwTimeTotal<100)					{
		Msg	("! WARNING: Invalid wave length (must be at least 100ms), file: %s",fn);
	}
}

void CSoundRender_Source::unload()
{
	FS.r_close(wave);
	SoundRender->cache.cat_destroy(CAT);
	dwTimeTotal = 0;
	dwBytesTotal = 0;
	dwBytesPerMS = 0;
}

