#include "sithSoundMixer.h"

#include "AI/sithAIAwareness.h"
#include "Engine/sithControl.h"
#include "Engine/sithSound.h"
#include "Engine/sithTime.h"
#include "Engine/sithNet.h"
#include "Engine/sithCamera.h"
#include "Win95/stdSound.h"
#include "Win95/stdMci.h"
#include "Gui/jkGUISound.h"
#include "World/sithWorld.h"
#include "World/sithSector.h"
#include "World/jkPlayer.h"
#include "Dss/sithDSSThing.h"
#include "jk.h"

int sithSoundMixer_Startup()
{
    if ( !stdMci_Startup() )
        return 1;

    if ( sithSoundMixer_bInitted )
    {
        sithSoundMixer_musicVolume = 1.0;
        stdMci_SetVolume(sithSoundMixer_globalVolume);
    }

    sithSoundMixer_bInitted = 1;
    return 1;
}

void sithSoundMixer_Shutdown()
{
    if ( sithSoundMixer_bInitted )
    {
        stdMci_Shutdown();
        sithSoundMixer_bInitted = 0;
    }
}

int sithSoundMixer_PlaySong(unsigned int trackFrom, unsigned int trackTo, unsigned int trackNum, int a4)
{
    unsigned int trackTo_; // esi
    unsigned int trackFrom_; // edi
    int result; // eax

    if ( sithSoundMixer_bPlayingMci )
        stdMci_Stop();
    trackTo_ = trackTo;
    if (trackTo <= trackFrom)
        trackTo_ = trackFrom;
    trackFrom_ = trackNum;
    if ( trackNum < trackFrom)
    {
        trackFrom_ = trackFrom;
    }
    else if ( trackNum > trackTo_ )
    {
        trackFrom_ = trackTo_;
    }
    sithSoundMixer_trackFrom = trackFrom;
    sithSoundMixer_bPlayingMci = 1;
    sithSoundMixer_trackTo = trackTo_;
    sithSoundMixer_dword_835FCC = a4;
    if ( !sithSoundMixer_bIsMuted )
    {
        if ( sithSoundMixer_bInitted )
        {
            sithSoundMixer_musicVolume = 1.0;
            stdMci_SetVolume(sithSoundMixer_globalVolume);
        }

        if ( !stdMci_Play(trackFrom_, trackTo_) )
        {
            sithSoundMixer_bPlayingMci = 0;
            return 0;
        }
        if ( a4 )
            sithSoundMixer_flt_835FD8 = sithTime_curSeconds - -5.0;
    }
    return 1;
}

void sithSoundMixer_StopSong()
{
    if ( sithSoundMixer_bPlayingMci )
    {
        stdMci_Stop();
        sithSoundMixer_bPlayingMci = 0;
    }
}

void sithSoundMixer_UpdateMusicVolume(float musicVolume)
{
    float vol; // [esp+0h] [ebp-4h]

    if ( musicVolume < 0.0 )
    {
        sithSoundMixer_globalVolume = 0.0;
    }
    else if ( musicVolume > 1.0 )
    {
        sithSoundMixer_globalVolume = 1.0;
    }
    else
    {
        sithSoundMixer_globalVolume = musicVolume;
    }
    if ( sithSoundMixer_globalVolume == 0.0 )
    {
        sithSoundMixer_bIsMuted = 1;
        if ( !sithSoundMixer_bInitted )
            return;
        stdMci_Stop();
    }
    else
    {
        if ( !sithSoundMixer_bInitted )
            return;
        sithSoundMixer_bIsMuted = 0;
        if ( sithSoundMixer_bPlayingMci )
        {
            if ( sithSoundMixer_dword_835FCC )
            {
                sithSoundMixer_flt_835FD8 = sithTime_curSeconds - -5.0;
                if ( !stdMci_CheckStatus() && !stdMci_Play(sithSoundMixer_trackFrom, sithSoundMixer_trackTo) )
                {
                    sithSoundMixer_bPlayingMci = 0;
                    sithSoundMixer_dword_835FCC = 0;
                }
            }
        }
    }
    
    // inlined
    sithSoundMixer_SetMusicVol(sithSoundMixer_musicVolume);
}

void sithSoundMixer_SetMusicVol(float volume)
{
    if ( sithSoundMixer_bInitted )
    {
        if ( volume < 0.0 )
        {
            sithSoundMixer_musicVolume = 0.0;
        }
        else if ( volume > 1.0 )
        {
            sithSoundMixer_musicVolume = 1.0;
        }
        else
        {
            sithSoundMixer_musicVolume = volume;
        }

        stdMci_SetVolume(sithSoundMixer_globalVolume * sithSoundMixer_musicVolume);
    }
}

void sithSoundMixer_ResumeMusic(int a1)
{
    if ( sithSoundMixer_bPlayingMci
      && !sithSoundMixer_bIsMuted
#ifndef QOL_IMPROVEMENTS
      && (a1 || sithControl_msIdle >= 0x7D0)
#endif
      && sithSoundMixer_dword_835FCC
      && (a1 || sithSoundMixer_flt_835FD8 <= (double)sithTime_curSeconds) )
    {
        sithSoundMixer_flt_835FD8 = sithTime_curSeconds - -5.0;
        if ( !stdMci_CheckStatus() && !stdMci_Play(sithSoundMixer_trackFrom, sithSoundMixer_trackTo) )
        {
            sithSoundMixer_bPlayingMci = 0;
            sithSoundMixer_dword_835FCC = 0;
        }
    }
}

int sithSoundMixer_Open()
{
    if ( sithSoundMixer_bOpened )
        return 0;

    if ( !sithSound_bInit )
        return 0;
    
    sithSoundMixer_ClearAll();

    sithSoundMixer_bOpened = 1;
    return 1;
}

void sithSoundMixer_Close()
{
    unsigned int v0; // ebp
    sithPlayingSound* soundIter;
    sithPlayingSound *v3; // edi

    if (!sithSoundMixer_bOpened)
        return;

    v0 = 0;
    if ( sithSoundMixer_numSoundsAvailable )
    {
        soundIter = &sithSoundMixer_aPlayingSounds[0];
        do
        {
            if ( soundIter->sound )
            {
                sithSoundMixer_StopSound(soundIter);
            }
            ++v0;
            soundIter++;
        }
        while ( v0 < sithSoundMixer_numSoundsAvailable );
    }
    sithSoundMixer_pPlayingSoundIdk = 0;
    sithSoundMixer_dword_836BFC = 0;
    sithSoundMixer_pLastSectorSoundSector = 0;
    stdSound_IA3D_idk(0.0);
    sithSoundMixer_dword_836C00 = 0;
    sithSoundMixer_pLastSectorSoundSector = 0;
    sithSoundMixer_pPlayingSoundIdk = 0;
    sithSoundMixer_dword_836BFC = 0;
    sithSoundMixer_bOpened = 0;
}

void sithSoundMixer_ClearAll()
{
    _memset(sithSoundMixer_aPlayingSounds, 0, sizeof(sithPlayingSound) * 32);
    for (int i = 0; i < 32; i++)
    {
        sithSoundMixer_aPlayingSounds[i].idx = i;
    }

    sithSoundMixer_numSoundsAvailable = 32;
    sithSoundMixer_numSoundsAvailable2 = 32;

    // Someone please help whoever programmed this    
    for (int i = 31; i >= 0; i--)
    {
        // Setting the index?
        sithSoundMixer_aPlayingSounds[i].idx = i;

        // Ok nvm clearing the struct
        _memset(&sithSoundMixer_aPlayingSounds[i], 0, sizeof(sithPlayingSound));

        // but gotta set that index again
        sithSoundMixer_aPlayingSounds[i].idx = i;
        sithSoundMixer_aIdk[31 - i] = i;
    }

    sithSoundMixer_dword_836BF8 = 0;
    sithSoundMixer_dword_836BFC = 0;
    sithSoundMixer_pLastSectorSoundSector = 0;
}

void sithSoundMixer_StopAll()
{
    unsigned int v0; // edi

    if ( !sithNet_isMulti )
    {
        for (v0 = 0; v0 < sithSoundMixer_numSoundsAvailable; v0++)
        {
            if ( (sithSoundMixer_aPlayingSounds[v0].flags & SITHSOUNDFLAG_PLAYING) != 0 )
            {
                stdSound_BufferStop(sithSoundMixer_aPlayingSounds[v0].pSoundBuf);
                sithSoundMixer_aPlayingSounds[v0].flags |= SITHSOUNDFLAG_PAUSED;
            }
        }
    }
}

void sithSoundMixer_ResumeAll()
{
    unsigned int v0; // edi

    if ( !sithNet_isMulti )
    {
        for (v0 = 0; v0 < sithSoundMixer_numSoundsAvailable; v0++)
        {
            if ( (sithSoundMixer_aPlayingSounds[v0].flags & SITHSOUNDFLAG_PAUSED) != 0 )
            {
                if ( sithSoundMixer_aPlayingSounds[v0].pSoundBuf )
                    stdSound_BufferPlay(sithSoundMixer_aPlayingSounds[v0].pSoundBuf, sithSoundMixer_aPlayingSounds[v0].flags & SITHSOUNDFLAG_LOOP);
                sithSoundMixer_aPlayingSounds[v0].flags &= ~SITHSOUNDFLAG_PAUSED;
            }
        }
    }
}

sithPlayingSound* sithSoundMixer_PlayingSoundFromSound(sithSound *sound, int flags)
{
    sithPlayingSound *result; // eax
    int v3; // eax
    int v5; // edx

    if ( !sithSoundMixer_bOpened )
        return 0;
    if ( sithSoundMixer_numSoundsAvailable2 )
        v3 = sithSoundMixer_aIdk[--sithSoundMixer_numSoundsAvailable2];
    else
        v3 = -1;
    if ( v3 < 0 )
        return 0;

    result = &sithSoundMixer_aPlayingSounds[v3];
    result->sound = sound;
    result->flags = flags;
    result->volumeVelocity = 1.0;
    result->pitch = 1.0;
    if ( sithSoundMixer_nextSoundIdx == 0 )
        sithSoundMixer_nextSoundIdx = 1;
    v5 = sithSoundMixer_nextSoundIdx++ | ((playerThingIdx + 1) << 16);
    result->refid = v5;
    return result;
}

sithPlayingSound* sithSoundMixer_cog_playsound_internal(sithSound *sound, float volume, float pan, int flags)
{
    sithPlayingSound *result; // eax
    sithPlayingSound *v6; // ebx
    int v8; // ecx
    double v12; // st7
    int v13; // ecx
    int v15; // esi
    sithPlayingSound* v16;
    sithPlayingSound *v17; // esi
    int v19; // ecx
    sithPlayingSound *v20; // esi
    int v22; // eax
    int v23; // edx
    int v24; // eax
    float a2; // [esp+0h] [ebp-10h]
    float a2a; // [esp+0h] [ebp-10h]
    stdSound_buffer_t *a2b; // [esp+0h] [ebp-10h]
    stdSound_buffer_t *a2d; // [esp+0h] [ebp-10h]

#ifdef LINUX
    //printf("STUBBED: play %s absolute 2 flags %x\n", sound->sound_fname, flags);
    //return NULL;
#endif

#ifdef OPENAL_SOUND
    jkGuiSound_numChannels = 256;
#endif

    v6 = sithSoundMixer_PlayingSoundFromSound(sound, flags);
    
    if ( !v6 )
        return 0;

    v6->pSoundBuf = sithSound_LoadData(v6->sound);
    if ( !v6->pSoundBuf )
        goto LABEL_50;
    ++v6->sound->field_40;
    a2 = v6->vol_2 * 0.75;
    stdSound_BufferSetVolume(v6->pSoundBuf, a2);
    if ( jkGuiSound_b3DSound )
    {
        v6->p3DSoundObj = stdSound_BufferQueryInterface(v6->pSoundBuf);
        if ( v6->p3DSoundObj )
            stdSound_BufferSetVolume(v6->pSoundBuf, v6->vol_2);
    }
    if ( v6->p3DSoundObj )
        stdSound_3DBufferIdk(v6->p3DSoundObj, 2);
    if ( volume < 0.0 )
    {
        v12 = 0.0;
    }
    else if ( volume > 1.5 )
    {
        v12 = 1.5;
    }
    else
    {
        v12 = volume;
    }
    v6->vol_2 = v12;
    if ( v6->pSoundBuf )
    {
        v13 = v6->flags;
        if ( (v13 & SITHSOUNDFLAG_NO_3D) != 0 || (v13 & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) == 0 )
        {
            a2a = v12 * 0.75;
            stdSound_BufferSetVolume(v6->pSoundBuf, a2a);
        }
    }
    stdSound_BufferSetPan(v6->pSoundBuf, pan);
    if ( sithSoundMixer_activeChannels >= (unsigned int)jkGuiSound_numChannels )
    {
        v15 = 0;
        while ( sithSoundMixer_dword_836C04 >= 0x20 )
        {
LABEL_35:
            if ( v15 )
                goto LABEL_46;
            sithSoundMixer_dword_836C04 = 0;
            v15 = 1;
        }

        v16 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
        while ( (v16->flags & SITHSOUNDFLAG_PLAYING) == 0 || (v16->flags & (SITHSOUNDFLAG_HIGHEST_PRIO|SITHSOUNDFLAG_HIGHPRIO|SITHSOUNDFLAG_LOOP)) != 0 )
        {
            v16++;
            sithSoundMixer_dword_836C04++;
            if ( v16 >= &sithSoundMixer_aPlayingSounds[32] )
                goto LABEL_35;
        }

        v17 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
        sithSoundMixer_PlayingSoundReset(v17);

        if ( (sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04].flags & SITHSOUNDFLAG_LOOP) == 0 )
        {
            v20 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
            sithSoundMixer_StopSound(v20);
        }
        sithSoundMixer_dword_836C04++;
    }
LABEL_46:
    if ( sithSoundMixer_activeChannels < (unsigned int)jkGuiSound_numChannels )
    {
        if ( jkGuiSound_b3DSound )
            stdSound_CommitDeferredSettings();
        stdSound_BufferPlay(v6->pSoundBuf, v6->flags & SITHSOUNDFLAG_LOOP);
        v6->flags |= SITHSOUNDFLAG_PLAYING;
        sithSoundMixer_activeChannels++;
        result = v6;
    }
    else
    {
LABEL_50:
        sithSoundMixer_StopSound(v6);
        return NULL;
    }
    return result;
}

sithPlayingSound* sithSoundMixer_PlaySoundPosAbsolute(sithSound *a1, rdVector3 *a2, sithSector *a3, float a4, float a5, float a6, int a7)
{
    int32_t v7; // ebx
    double v8; // st6
    double v9; // st7
    double v10; // st7
    sithPlayingSound *v11; // ecx
    int v12; // eax
    sithPlayingSound *v14; // eax
    int v15; // edx
    rdVector3 v16; // [esp+8h] [ebp-Ch] BYREF

#ifdef LINUX
    //printf("play %s absolute\n", a1->sound_fname);
#endif

    if ( sithSoundMixer_bOpened )
    {
        if ( a4 < 0.0 )
        {
            a4 = 0.0;
        }
        else if ( a4 > 1.5 )
        {
            a4 = 1.5;
        }
        v7 = a7 & ~SITHSOUNDFLAG_PLAYING | SITHSOUNDFLAG_ABSOLUTE;
        if ( a3 && (a3->flags & SITH_SECTOR_UNDERWATER) != 0 )
            v7 |= SITHSOUNDFLAG_UNDERWATER;
        if ( sithCamera_currentCamera )
        {
            v8 = a2->y;
            v9 = a2->z;
            v16.x = a2->x - sithCamera_currentCamera->vec3_1.x;
            v16.y = v8 - sithCamera_currentCamera->vec3_1.y;
            v16.z = v9 - sithCamera_currentCamera->vec3_1.z;
            v10 = rdVector_Normalize3QuickAcc(&v16);
            if ( (v7 & SITHSOUNDFLAG_LOOP) != 0 || v10 <= a6 )
            {
                if ( sithSoundMixer_bOpened )
                {
                    if ( sithSoundMixer_numSoundsAvailable2 )
                        v12 = sithSoundMixer_aIdk[--sithSoundMixer_numSoundsAvailable2];
                    else
                        v12 = -1;
                    if ( v12 >= 0 )
                    {
                        // TODO inlined func
                        v14 = &sithSoundMixer_aPlayingSounds[v12];
                        v14->sound = a1;
                        v14->flags = v7;
                        v14->volumeVelocity = 1.0;
                        v14->pitch = 1.0;
                        if ( sithSoundMixer_nextSoundIdx == 0 )
                            sithSoundMixer_nextSoundIdx = 1;
                        v15 = sithSoundMixer_nextSoundIdx++ | ((playerThingIdx + 1) << 16);
                        v14->refid = v15;
                        v11 = v14;
                    }
                    else
                    {
                        v11 = 0;
                    }
                }
                else
                {
                    v11 = 0;
                }
                if ( v11 )
                {
                    v11->pos = *a2;
                    v11->vol_2 = a4;
                    v11->anonymous_5 = a5;
                    v11->maxPosition = a6;
                    v11->posRelative = v16;
                    v11->anonymous_13 = v10;
                    if ( a5 == a6 )
                        v11->anonymous_7 = 0.0;
                    else
                        v11->anonymous_7 = 1.0 / (a6 - a5);
                    return v11;
                }
            }
        }
    }
    return NULL;
}

sithPlayingSound* sithSoundMixer_PlaySoundPosThing(sithSound *sound, sithThing *a2, float a3, float a4, float a5, int flags)
{
    sithPlayingSound *v11; // esi
    int v12; // eax
    int v14; // eax
    float v15; // edx
    stdSound_buffer_t *v16; // eax
    stdSound_buffer_t *v17; // eax
    stdSound_buffer_t *v18; // eax
    unsigned int v19; // ecx
    int v20; // edi
    sithPlayingSound* v21; // eax
    sithPlayingSound *v22; // edi
    int v23; // ecx
    int v27; // edx
    int v28; // eax
    stdSound_buffer_t *v29; // eax
    float a4a; // [esp+0h] [ebp-28h]
    float a2a; // [esp+4h] [ebp-24h]
    stdSound_buffer_t *a2b; // [esp+4h] [ebp-24h]
    stdSound_buffer_t *a2c; // [esp+4h] [ebp-24h]
    float v34; // [esp+18h] [ebp-10h]
    rdVector3 a1; // [esp+1Ch] [ebp-Ch] BYREF

#ifdef LINUX
    //printf("play %s at thing flags %x\n", sound->sound_fname, flags);
#endif

#ifdef OPENAL_SOUND
    jkGuiSound_numChannels = 256;
#endif

    // Added: Prevent undef usage
    rdVector_Zero3(&a1);

    v34 = 50.0;
    if ( sithSoundMixer_bOpened )
    {
        flags = flags & ~SITHSOUNDFLAG_PLAYING;
        if ( a3 < 0.0 )
        {
            a3 = 0.0;
        }
        else if ( a3 > 1.5 )
        {
            a3 = 1.5;
        }
        flags = flags | SITHSOUNDFLAG_FOLLOWSTHING;
        if ( (flags & SITHSOUNDFLAG_LOOP) != 0
          || sithCamera_currentCamera
          && (a1.x = a2->position.x - sithCamera_currentCamera->vec3_1.x,
              a1.y = a2->position.y - sithCamera_currentCamera->vec3_1.y,
              a1.z = a2->position.z - sithCamera_currentCamera->vec3_1.z,
              v34 = rdVector_Normalize3QuickAcc(&a1),
              v34 <= a5) )
        {
            if ( (a2->type == SITH_THING_ACTOR || a2->type == SITH_THING_PLAYER) && (flags & SITHSOUNDFLAG_VOICE) != 0 && (flags & SITHSOUNDFLAG_HIGHEST_PRIO|SITHSOUNDFLAG_HIGHPRIO) == 0 )
            {
                if ( a2->actorParams.field_1BC > sithTime_curMs )
                    return 0;
                a2->actorParams.field_1BC = sithTime_curMs + sound->sound_len;
            }
            if ( a2 == g_localPlayerThing || a2->moveType == SITH_MT_PATH )
            {
                a4a = a5 * 0.60000002;
                sithAIAwareness_AddEntry(a2->sector, &a2->position, 0, a4a, a2);
            }

            v11 = sithSoundMixer_PlayingSoundFromSound(sound, flags);

            if ( v11 )
            {
                v11->thing = a2;
                v11->vol_2 = a3;
                v11->anonymous_5 = a4;
                v11->maxPosition = a5;
                v11->pos.x = a2->position.x;
                v15 = a2->position.z;
                v11->pos.y = a2->position.y;
                v11->pos.z = v15;
                v11->posRelative = a1;
                v11->anonymous_13 = v34;
                if ( a4 == a5 )
                    v11->anonymous_7 = 0.0;
                else
                    v11->anonymous_7 = 1.0 / (a5 - a4);
                if ( a2 == sithSoundMixer_pFocusedThing )
                {
                    v16 = sithSound_LoadData(v11->sound);
                    v11->pSoundBuf = v16;
                    if ( v16 )
                    {
                        ++v11->sound->field_40;
                        a2a = v11->vol_2 * 0.75;
                        stdSound_BufferSetVolume(v11->pSoundBuf, a2a);
                        if ( jkGuiSound_b3DSound )
                        {
                            v17 = (stdSound_buffer_t *)stdSound_BufferQueryInterface(v11->pSoundBuf);
                            v11->p3DSoundObj = v17;
                            if ( v17 )
                                stdSound_BufferSetVolume(v11->pSoundBuf, v11->vol_2);
                        }
                        v11->flags |= SITHSOUNDFLAG_NO_3D;
                        v18 = v11->p3DSoundObj;
                        if ( v18 )
                            stdSound_3DBufferIdk(v18, 2);
                        if ( sithSoundMixer_activeChannels >= (unsigned int)jkGuiSound_numChannels )
                        {
                            v19 = sithSoundMixer_dword_836C04;
                            v20 = 0;
                            while ( v19 >= 0x20 )
                            {
LABEL_46:
                                if ( v20 )
                                    goto LABEL_51;
                                v19 = 0;
                                v20 = 1;
                                sithSoundMixer_dword_836C04 = 0;
                            }
                            v21 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
                            while ( (v21->flags & SITHSOUNDFLAG_PLAYING) == 0
                                 || (v21->flags & (SITHSOUNDFLAG_HIGHEST_PRIO|SITHSOUNDFLAG_HIGHPRIO|SITHSOUNDFLAG_LOOP)) != 0 )
                            {
                                v21++;
                                sithSoundMixer_dword_836C04++;
                                if ( v21 >= &sithSoundMixer_aPlayingSounds[32] )
                                    goto LABEL_46;
                            }
                            v22 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];

                            sithSoundMixer_PlayingSoundReset(v22);

                            if ( (sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04].flags & SITHSOUNDFLAG_LOOP) == 0 )
                            {
                                sithSoundMixer_StopSound(&sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04]);
                            }
                            sithSoundMixer_dword_836C04++;
                        }
LABEL_51:
                        if ( sithSoundMixer_activeChannels < (unsigned int)jkGuiSound_numChannels )
                        {
                            if ( jkGuiSound_b3DSound )
                                stdSound_CommitDeferredSettings();
                            stdSound_BufferPlay(v11->pSoundBuf, v11->flags & SITHSOUNDFLAG_LOOP);
                            v11->flags |= SITHSOUNDFLAG_PLAYING;
                            sithSoundMixer_activeChannels++;
                            return v11;
                        }
                        if ( (flags & SITHSOUNDFLAG_LOOP) != 0 )
                            return v11;
                        if ( (v11->flags & SITHSOUNDFLAG_PLAYING) == 0 )
                            goto LABEL_59;
                        stdSound_BufferReset(v11->pSoundBuf);
                    }
                    else
                    {
                        if ( (v11->flags & SITHSOUNDFLAG_PLAYING) == 0 )
                            goto LABEL_59;
                        //stdSound_BufferReset(0); //???
                    }
                    sithSoundMixer_PlayingSoundReset(v11);
LABEL_59:
                    if ( v11->pSoundBuf )
                    {
                        stdSound_BufferRelease(v11->pSoundBuf);
                        v11->pSoundBuf = 0;
                    }
                    if ( v11->p3DSoundObj )
                    {
                        stdSound_3DBufferRelease(v11->p3DSoundObj);
                        v11->p3DSoundObj = 0;
                    }
                    v27 = v11->idx;
                    _memset(v11, 0, sizeof(sithPlayingSound));
                    v28 = sithSoundMixer_numSoundsAvailable2;
                    v11->idx = v27;
                    sithSoundMixer_aIdk[v28] = v27;
                    sithSoundMixer_numSoundsAvailable2 = v28 + 1;
                    return v11;
                }
                v29 = v11->p3DSoundObj;
                if ( v29 )
                    stdSound_3DBufferIdk(v29, 0);
                return v11;
                
            }
        }
    }
    return NULL;
}

void sithSoundMixer_PlayingSoundReset(sithPlayingSound *sound)
{
    stdSound_buffer_t *v2; // [esp-4h] [ebp-8h]

    stdSound_BufferReset(sound->pSoundBuf);
    sound->flags &= ~SITHSOUNDFLAG_PLAYING;
    v2 = sound->pSoundBuf;
    sithSoundMixer_activeChannels--;
    stdSound_BufferRelease(v2);
    sound->pSoundBuf = 0;
    --sound->sound->field_40;
}

void sithSoundMixer_Reset()
{
    unsigned int v0; // ebp
    int v2; // eax
    sithPlayingSound *v3; // edi
    int v4; // edx
    int v5; // eax
    stdSound_buffer_t *v6; // [esp-Ch] [ebp-14h]

    v0 = 0;
    for (v0 = 0; v0 < sithSoundMixer_numSoundsAvailable; v0++ )
    {
        sithPlayingSound* v1 = &sithSoundMixer_aPlayingSounds[v0];
        if ( v1->sound )
        {
            sithSoundMixer_StopSound(v1);
        }
    }
    sithSoundMixer_pPlayingSoundIdk = 0;
    sithSoundMixer_pLastSectorSoundSector = 0;
    sithSoundMixer_dword_836BFC = 0;
}

void sithSoundMixer_SetPitch(sithPlayingSound *sound, float pitch, float changetime)
{
    sithSound *v3; // ecx
    char v5; // c3
    int v6; // eax

    if ( changetime > 0.0 )
    {
        sound->pitchVel = (pitch - sound->pitch) / changetime;
        if ( sound->pitchVel != 0.0 )
        {
            v6 = sound->flags;
            v6 |= SITHSOUNDFLAG_PER_VEL_PITCH_BEND;
            sound->nextPitch = pitch;
            sound->flags = v6;
        }
    }
    else if ( sithSoundMixer_bOpened )
    {
        v3 = sound->sound;
        sound->pitch = pitch;
        if ( v3 )
        {
            if ( sound->pSoundBuf )
                stdSound_BufferSetFrequency(sound->pSoundBuf, (int)((double)v3->sampleRateHz * pitch));
        }
    }
}

int sithSoundMixer_SetFrequency(sithPlayingSound *sound, float pitch)
{
    sithSound *v2; // ecx

    if ( !sithSoundMixer_bOpened )
        return 0;
    v2 = sound->sound;
    sound->pitch = pitch;
    if ( !v2 || !sound->pSoundBuf )
        return 0;
    stdSound_BufferSetFrequency(sound->pSoundBuf, (int)((double)v2->sampleRateHz * pitch));
    return 1;
}

void sithSoundMixer_FadeSound(sithPlayingSound *sound, float vol_, float fadeintime_)
{
    double v3; // st7
    int v4; // eax
    double v5; // st7
    double v7; // st6
    float a2; // [esp+0h] [ebp-4h]
    
    //printf("fade actual %s %f\n", sound->sound->sound_fname, fadeintime_);

    if ( vol_ < 0.0 )
    {
        vol_ = 0.0;
    }
    else if ( vol_ > 1.5 )
    {
        vol_ = 1.5;
    }
    v3 = vol_ - sound->vol_2;
    if ( v3 != 0.0 )
    {
        v4 = sound->flags;
        v4 = v4 & ~(SITHSOUNDFLAG_FADE_OUT|SITHSOUNDFLAG_FADE_IN|SITHSOUNDFLAG_FADING);
        sound->flags = v4;
        if ( fadeintime_ == 0.0 )
        {
            if ( vol_ < 0.0 )
            {
                v5 = 0.0;
            }
            else if ( vol_ > 1.5 )
            {
                v5 = 1.5;
            }
            else
            {
                v5 = vol_;
            }
            sound->vol_2 = v5;
            if ( sound->pSoundBuf && ((v4 & SITHSOUNDFLAG_NO_3D) != 0 || (v4 & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) == 0) )
            {
                a2 = v5 * 0.75;
                stdSound_BufferSetVolume(sound->pSoundBuf, a2);
            }
        }
        else
        {
            v7 = v3;
            if ( v7 < 0.0 ) // TODO verify? fadeintime_ < 0.0?
                v7 = -v3;
            sound->volumeVelocity = v7 / fadeintime_;
            sound->volume = vol_;
            if ( v3 < 0.0 ) // TODO verify? sound->volumeVelocity < 0.0? fadeintime_ > 0.0?
                sound->flags = v4 | SITHSOUNDFLAG_FADE_OUT;
            else
                sound->flags = v4 | SITHSOUNDFLAG_FADE_IN;
        }
    }
}

void sithSoundMixer_SetVolume(sithPlayingSound *sound, float volume)
{
    double v2; // st7
    stdSound_buffer_t *v3; // ecx
    int v4; // eax
    float a2a; // [esp+0h] [ebp-4h]

    if ( volume < 0.0 )
    {
        v2 = 0.0;
    }
    else if ( volume > 1.5 )
    {
        v2 = 1.5;
    }
    else
    {
        v2 = volume;
    }
    v3 = sound->pSoundBuf;
    sound->vol_2 = v2;
    if ( v3 )
    {
        v4 = sound->flags;
        if ( (v4 & SITHSOUNDFLAG_NO_3D) != 0 || (v4 & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) == 0 )
        {
            a2a = v2 * 0.75;
            stdSound_BufferSetVolume(v3, a2a);
        }
    }
}

void sithSoundMixer_Tick(float deltaSecs)
{
    sithSector *v1; // eax
    sithSound *v2; // esi
    sithPlayingSound *v3; // ecx
    double v4; // st7
    double v7; // st6
    double v13; // st7
    double v14; // st6
    double v17; // st6
    double v20; // st7
    double v23; // st6
    sithPlayingSound *v28; // eax
    double v31; // st6
    sithPlayingSound *v37; // eax
    sithPlayingSound *v38; // esi
    unsigned int v40; // edi
    sithPlayingSound *soundIter; // esi
    float v42; // [esp+4h] [ebp-10h]
    float v43; // [esp+4h] [ebp-10h]
    rdVector3 v44; // [esp+8h] [ebp-Ch] BYREF

#ifdef OPENAL_SOUND
    jkGuiSound_numChannels = 256;
#endif

    if ( !sithCamera_currentCamera )
        return;
    if ( (sithCamera_currentCamera->cameraPerspective & 0xFC) != 0 )
        sithSoundMixer_pFocusedThing = 0;
    else
        sithSoundMixer_pFocusedThing = sithWorld_pCurrentWorld->cameraFocus;
    v1 = sithCamera_currentCamera->sector;
    if ( v1 == sithSoundMixer_pLastSectorSoundSector )
        goto LABEL_72;
    sithSoundMixer_pLastSectorSoundSector = sithCamera_currentCamera->sector;
    if ( sithSoundMixer_dword_836C00 )
        goto LABEL_10;
    if ( (v1->flags & 2) != 0 )
    {
        stdSound_IA3D_idk(2.0);
        sithSoundMixer_dword_836C00 = 1;
        goto LABEL_12;
    }
    if ( sithSoundMixer_dword_836C00 )
    {
LABEL_10:
        if ( (v1->flags & 2) == 0 )
        {
            stdSound_IA3D_idk(1.0);
            sithSoundMixer_dword_836C00 = 0;
        }
    }
LABEL_12:
    v2 = sithSoundMixer_pLastSectorSoundSector->sectorSound;
    if ( sithSoundMixer_pLastSectorSoundSector->sectorSoundVol == 0.0 )
        v2 = 0;
    v3 = sithSoundMixer_pPlayingSoundIdk;
    if ( !v2 && sithSoundMixer_pPlayingSoundIdk )
    {
        v4 = -sithSoundMixer_pPlayingSoundIdk->vol_2;
        if ( v4 == 0.0 )
        {
            sithSoundMixer_pPlayingSoundIdk->flags |= SITHSOUNDFLAG_FADING;
            sithSoundMixer_pPlayingSoundIdk = 0;
            
        }
        else
        {
            v3->flags &= ~(SITHSOUNDFLAG_FADE_OUT|SITHSOUNDFLAG_FADE_IN|SITHSOUNDFLAG_FADING);
            v7 = v4;
            if ( v7 < 0.0 )
                v7 = -v4;
            v3->volume = 0.0;
            v3->volumeVelocity = v7 + v7;
            if ( v4 < 0.0 ) // TODO verify? v4
                v3->flags |= SITHSOUNDFLAG_FADE_OUT;
            else
                v3->flags |= SITHSOUNDFLAG_FADE_IN;
            sithSoundMixer_pPlayingSoundIdk = 0;
            v3->flags |= SITHSOUNDFLAG_FADING;
        }
        goto LABEL_72;
    }
    if ( v2 )
    {
        if ( sithSoundMixer_pPlayingSoundIdk )
        {
            if ( sithSoundMixer_pPlayingSoundIdk->sound == v2 )
            {
                v13 = sithSoundMixer_pLastSectorSoundSector->sectorSoundVol;
                if ( v13 < 0.0 )
                {
                    v13 = 0.0;
                }
                else if ( v13 > 1.5 )
                {
                    v13 = 1.5;
                }
                v14 = v13 - sithSoundMixer_pPlayingSoundIdk->vol_2;
                if ( v14 == 0.0 )
                    goto LABEL_72;
                v42 = v14;
                v17 = v42;

                // added copy for later
                v43 = v14;

                sithSoundMixer_pPlayingSoundIdk->flags &= ~(SITHSOUNDFLAG_FADE_OUT|SITHSOUNDFLAG_FADE_IN|SITHSOUNDFLAG_FADING);
                if ( v17 < 0.0 )
                    v17 = -v17;
            }
            else
            {
                v20 = -sithSoundMixer_pPlayingSoundIdk->vol_2;
                if ( v20 != 0.0 )
                {
                    sithSoundMixer_pPlayingSoundIdk->flags &= ~(SITHSOUNDFLAG_FADE_OUT|SITHSOUNDFLAG_FADE_IN|SITHSOUNDFLAG_FADING);
                    v23 = v20;
                    if ( v23 < 0.0 )
                        v23 = -v20;
                    v3->volume = 0.0;
                    v3->volumeVelocity = v23 + v23;
                    if ( v20 < 0.0 ) // TODO verify? v20 <
                        v3->flags |= SITHSOUNDFLAG_FADE_OUT;
                    else
                        v3->flags |= SITHSOUNDFLAG_FADE_IN;
                }
                //printf("%s fade\n", v3->sound->sound_fname);
                v3->flags |= SITHSOUNDFLAG_FADING;
                if ( sithSoundMixer_bOpened == 0 )
                {
                    v3 = 0;
                }
                else
                {
                    v28 = sithSoundMixer_PlayingSoundFromSound(v2, 1);
                    if ( v28 )
                    {
                        if ( !sithSoundMixer_sub_4DD5D0(v28) )
                            goto LABEL_49;

                        //printf("asdf %s\n", v28->sound->sound_fname);
                        if ( v28->p3DSoundObj )
                            stdSound_3DBufferIdk(v28->p3DSoundObj, 2);
                        sithSoundMixer_SetVolume(v28, 0.0);
                        stdSound_BufferSetPan(v28->pSoundBuf, 0.0);
                        if ( sithSoundMixer_sub_4DD3F0(v28) )
                        {
                            v3 = v28;
                        }
                        else
                        {
LABEL_49:
                            sithSoundMixer_StopSound(v28);
                            v3 = 0;
                        }
                    }
                    else
                    {
                        v3 = 0;
                    }
                }
                sithSoundMixer_pPlayingSoundIdk = v3;
                if ( !v3 )
                    goto LABEL_72;
                v13 = sithSoundMixer_pLastSectorSoundSector->sectorSoundVol;
                if ( v13 < 0.0 )
                {
                    v13 = 0.0;
                }
                else if ( v13 > 1.5 )
                {
                    v13 = 1.5;
                }
                v31 = v13 - v3->vol_2;
                if ( v31 == 0.0 )
                    goto LABEL_72;
                v43 = v31;
                v17 = v43;
                v3->flags &= ~(SITHSOUNDFLAG_FADE_OUT|SITHSOUNDFLAG_FADE_IN|SITHSOUNDFLAG_FADING);
                if ( v17 < 0.0 )
                    v17 = -v17;
            }
            v3->volumeVelocity = v17 + v17;
            v3->volume = v13;
            if ( v43 < 0.0 ) // TODO verify? v43 > 0.0
                v3->flags |= SITHSOUNDFLAG_FADE_OUT;
            else
                v3->flags |= SITHSOUNDFLAG_FADE_IN;
            
            goto LABEL_72;
        }

        if ( sithSoundMixer_bOpened )
        {
            v37 = sithSoundMixer_PlayingSoundFromSound(v2, 1);
            v38 = v37;
            if ( v37 )
            {
                if ( sithSoundMixer_sub_4DD5D0(v37) )
                {
                    if ( v38->p3DSoundObj )
                        stdSound_3DBufferIdk(v38->p3DSoundObj, 2);
                    sithSoundMixer_SetVolume(v38, 0.0);
                    stdSound_BufferSetPan(v38->pSoundBuf, 0.0);
                    if ( sithSoundMixer_sub_4DD3F0(v38) )
                        goto LABEL_70;
                }
                sithSoundMixer_StopSound(v38);
            }
        }
        v38 = 0;
LABEL_70:
        sithSoundMixer_pPlayingSoundIdk = v38;

        if ( sithSoundMixer_pPlayingSoundIdk )
            sithSoundMixer_FadeSound(sithSoundMixer_pPlayingSoundIdk, sithSoundMixer_pLastSectorSoundSector->sectorSoundVol, 0.5);
    }
LABEL_72:
    for (v40 = 0; v40 < sithSoundMixer_numSoundsAvailable; v40++ )
    {
        soundIter = &sithSoundMixer_aPlayingSounds[v40];
        if ( soundIter->sound )
        {
            //jk_printf("tick %u: %s %x, %f %f vol %f\n", v40, soundIter->sound->sound_fname, soundIter->flags, soundIter->anonymous_13, soundIter->maxPosition, soundIter->vol_2);
            sithSoundMixer_TickPlayingSound(soundIter, deltaSecs);
        }
    }
    //printf("--- %u\n", sithSoundMixer_activeChannels);
    v44.x = sithCamera_currentCamera->vec3_1.x * 10.0;
    v44.y = sithCamera_currentCamera->vec3_1.y * 10.0;
    v44.z = sithCamera_currentCamera->vec3_1.z * 10.0;
    stdSound_SetPositionOrientation(&v44, &sithCamera_currentCamera->viewMat.lvec, &sithCamera_currentCamera->viewMat.uvec);
}

void sithSoundMixer_TickPlayingSound(sithPlayingSound *sound, float deltaSecs)
{
    double v4; // st7
    double v5; // st7
    stdSound_buffer_t *v6; // ecx
    int v8; // eax
    sithSound *v9; // eax
    stdSound_buffer_t *v10; // eax
    double v11; // st7
    stdSound_buffer_t *v12; // ecx
    sithSound *v18; // eax
    int v20; // eax
    sithSound *v21; // eax
    int v23; // ecx
    sithSound *v24; // eax
    int v26; // edx
    int v27; // eax
    stdSound_buffer_t *v29; // eax
    stdSound_buffer_t *v30; // eax
    float a2; // [esp+0h] [ebp-18h]
    stdSound_buffer_t *a2a; // [esp+0h] [ebp-18h]
    float a2b; // [esp+0h] [ebp-18h]
    stdSound_buffer_t *a2c; // [esp+0h] [ebp-18h]
    float a2e; // [esp+0h] [ebp-18h]
    float a1a; // [esp+1Ch] [ebp+4h]
    float deltaSecsa; // [esp+20h] [ebp+8h]

    if (!sound->sound)
        return;


    if ( (sound->flags & SITHSOUNDFLAG_FADE_OUT) != 0 )
    {
        v4 = sound->vol_2 - sound->volumeVelocity * deltaSecs;
        a1a = v4;
        if ( v4 <= sound->volume )
        {
            a1a = sound->volume;
            sound->flags &= ~SITHSOUNDFLAG_FADE_OUT;
        }
        if ( a1a < 0.0 )
        {
            v5 = 0.0;
        }
        else if ( a1a > 1.5 )
        {
            v5 = 1.5;
        }
        else
        {
            v5 = a1a;
        }
        v6 = sound->pSoundBuf;
        sound->vol_2 = v5;
        if ( v6 )
        {
            if ( (sound->flags & SITHSOUNDFLAG_NO_3D) != 0
              || (sound->flags & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) == 0 )
            {
                a2 = v5 * 0.75;
                stdSound_BufferSetVolume(v6, a2);
            }
        }

        if ( (sound->flags & SITHSOUNDFLAG_FADING) != 0 && a1a <= 0.0 )
        {
            sithSoundMixer_StopSound(sound);
        }
    }

    if ( (sound->flags & SITHSOUNDFLAG_FADE_IN) != 0 )
    {
        v11 = sound->volumeVelocity * deltaSecs + sound->vol_2;
        if ( v11 >= sound->volume )
        {
            v11 = sound->volume;
            sound->flags &= ~SITHSOUNDFLAG_FADE_IN;
        }
        if ( v11 < 0.0 )
        {
            v11 = 0.0;
        }
        else if ( v11 > 1.5 )
        {
            v11 = 1.5;
        }
        v12 = sound->pSoundBuf;
        sound->vol_2 = v11;
        if ( v12 )
        {
            if ( (sound->flags & SITHSOUNDFLAG_NO_3D) != 0
              || (sound->flags & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) == 0 )
            {
                a2b = v11 * 0.75;
                stdSound_BufferSetVolume(v12, a2b);
            }
        }
    }

    if ( (sound->flags & SITHSOUNDFLAG_PER_VEL_PITCH_BEND) != 0 )
    {
        deltaSecsa = sound->pitchVel * deltaSecs + sound->pitch;
        if ( sound->pitchVel > 0.0 && deltaSecsa > (double)sound->nextPitch || sound->pitchVel < 0.0 && deltaSecsa < (double)sound->nextPitch ) // TODO verify sound->pitchVel > 0
        {
            sound->flags &= ~SITHSOUNDFLAG_PER_VEL_PITCH_BEND;
            deltaSecsa = sound->nextPitch;
        }
        sithSoundMixer_SetFrequency(sound, deltaSecsa);
    }

    if ( (sound->flags & SITHSOUNDFLAG_PLAYING) && !(sound->flags & SITHSOUNDFLAG_LOOP) && !stdSound_IsPlaying(sound->pSoundBuf, 0) )
    {
        sithSoundMixer_StopSound(sound);
    }

    if ( !(sound->flags & SITHSOUNDFLAG_NO_3D)
      && (sound->flags & (SITHSOUNDFLAG_FOLLOWSTHING|SITHSOUNDFLAG_ABSOLUTE)) != 0 )
    {
        sithSoundMixer_UpdatePlayingSoundPosition(sound);
        if ( sound->anonymous_13 <= (double)sound->maxPosition )
        {
            if ( !sound->pSoundBuf )
            {
                sound->pSoundBuf = sithSound_LoadData(sound->sound);
                if ( !sound->pSoundBuf )
                {
                    sithSoundMixer_StopSound(sound);
                    return;
                }
                a2e = sound->vol_2 * 0.75;
                ++sound->sound->field_40;
                stdSound_BufferSetVolume(sound->pSoundBuf, a2e);
                if ( jkGuiSound_b3DSound )
                {
                    v29 = (stdSound_buffer_t *)stdSound_BufferQueryInterface(sound->pSoundBuf);
                    sound->p3DSoundObj = v29;
                    if ( v29 )
                        stdSound_BufferSetVolume(sound->pSoundBuf, sound->vol_2);
                }
                v30 = sound->p3DSoundObj;
                if ( v30 && (sound->flags & SITHSOUNDFLAG_NO_3D) == 0 )
                    stdSound_3DBufferIdk(v30, 0);
            }
            sithSoundMixer_UpdateSoundPos(sound);
            if ( (sound->flags & SITHSOUNDFLAG_DOPPLER) != 0 )
                sithSoundMixer_SetVelocity(sound);
            if ( (sound->flags & SITHSOUNDFLAG_PLAYING) == 0 )
                sithSoundMixer_sub_4DD3F0(sound);
        }
        else
        {
            if (sound->flags & SITHSOUNDFLAG_PLAYING)
            {
                sithSoundMixer_PlayingSoundReset(sound);
            }
            
            // Added: adjusted this so that sounds actually free?
            // TODO figure out why this needed to be changed
            if (!(sound->flags & SITHSOUNDFLAG_LOOP)) 
            {
                sithSoundMixer_StopSound(sound);
            }
        }
    }
}

void sithSoundMixer_UpdateSoundPos(sithPlayingSound *sound)
{
    double v2; // st7
    double v3; // st5
    double v4; // st4
    stdSound_buffer_t *v5; // eax
    double v6; // st6
    double v7; // st6
    double v8; // st6
    double v9; // st6
    double v10; // st7
    double v11; // st6
    stdSound_buffer_t *v12; // eax
    float a2; // [esp+0h] [ebp-18h]
    float a2a; // [esp+0h] [ebp-18h]
    float a2b; // [esp+0h] [ebp-18h]
    float v16; // [esp+8h] [ebp-10h]
    rdVector3 v17; // [esp+Ch] [ebp-Ch] BYREF
    float a1a; // [esp+1Ch] [ebp+4h]

    v2 = sound->vol_2;
    v16 = sound->vol_2;
    if ( sound->p3DSoundObj )
    {
        v8 = sound->anonymous_13 - sound->anonymous_5;
        if ( v8 > 0.0 )
        {
            v9 = v8 * sound->anonymous_7;
            if ( v9 < 0.0 )
            {
                v9 = 0.0;
            }
            else if ( v9 > 1.0 )
            {
                v9 = 1.0;
            }
            v2 = v2 * (1.0 - v9);
        }
        if ( v2 < 0.0 )
        {
            v2 = 0.0;
        }
        else if ( v2 > 1.5 )
        {
            v2 = 1.5;
        }
        a2b = v2 * 0.75;
        stdSound_BufferSetVolume(sound->pSoundBuf, a2b);
        v10 = sound->pos.y * 10.0;
        v11 = sound->pos.z * 10.0;
        v12 = sound->p3DSoundObj;
        v17.x = sound->pos.x * 10.0;
        v17.y = v10;
        v17.z = v11;
        stdSound_SetPosition(v12, &v17);
    }
    else
    {
        if ( (sound->flags & SITHSOUNDFLAG_AMBIENT_NOPAN) == 0 )
        {
            v3 = sound->posRelative.x;
            v4 = sound->posRelative.y;
            v5 = sound->pSoundBuf;
            a1a = sithCamera_currentCamera->viewMat.lvec.z * sound->posRelative.z
                + sithCamera_currentCamera->viewMat.lvec.y * v4
                + sithCamera_currentCamera->viewMat.lvec.x * v3;
            a2 = sithCamera_currentCamera->viewMat.rvec.x * v3
               + sithCamera_currentCamera->viewMat.rvec.y * v4
               + sithCamera_currentCamera->viewMat.rvec.z * sound->posRelative.z;
            stdSound_BufferSetPan(v5, a2);
            if ( a1a >= 0.0 )
                v2 = v16;
            else
                v2 = (1.0 - -a1a * 0.30000001) * v16;
        }
        v6 = sound->anonymous_13 - sound->anonymous_5;
        if ( v6 > 0.0 )
        {
            v7 = v6 * sound->anonymous_7;
            if ( v7 < 0.0 )
            {
                v7 = 0.0;
            }
            else if ( v7 > 1.0 )
            {
                v7 = 1.0;
            }
            v2 = v2 * (1.0 - v7);
        }
        if ( v2 < 0.0 )
        {
            v2 = 0.0;
        }
        else if ( v2 > 1.5 )
        {
            v2 = 1.5;
        }
        a2a = v2 * 0.75;
        stdSound_BufferSetVolume(sound->pSoundBuf, a2a);
    }
}

void sithSoundMixer_SetVelocity(sithPlayingSound *sound)
{
    sithThing *v1; // eax
    rdVector3 a2; // [esp+0h] [ebp-Ch] BYREF

    v1 = sound->thing;
    if ( v1 )
    {
        if ( sound->p3DSoundObj )
        {
            if ( v1->moveType == SITH_MT_PHYSICS )
            {
                a2.x = v1->physicsParams.vel.x * 10.0;
                a2.y = v1->physicsParams.vel.y * 10.0;
                a2.z = v1->physicsParams.vel.z * 10.0;
                stdSound_SetVelocity(sound->p3DSoundObj, &a2);
            }
        }
    }
    else
    {
        sound->flags &= ~SITHSOUNDFLAG_DOPPLER;
    }
}

void sithSoundMixer_SyncSounds()
{
    for (int i = 0; i < MIXER_NUMPLAYINGSOUNDS; i++)
    {
        sithPlayingSound* iter = &sithSoundMixer_aPlayingSounds[i];
        if (iter != sithSoundMixer_pPlayingSoundIdk)
        {
            sithSound* sound = iter->sound;
            if ( sound )
            {
                if ( (iter->flags & 1) != 0 )
                    sithDSSThing_SendPlaySoundPos(
                        iter->thing,
                        &iter->pos,
                        sound,
                        iter->anonymous_5,
                        iter->maxPosition,
                        iter->flags,
                        iter->refid,
                        -1,
                        255);
            }
        }
        ++iter;
    }
}

void sithSoundMixer_FreePlayingSound(sithPlayingSound *sound)
{
    int v1; // esi

    v1 = sound->idx;
    _memset(sound, 0, sizeof(sithPlayingSound));
    sound->idx = v1;
    sithSoundMixer_aIdk[sithSoundMixer_numSoundsAvailable2++] = v1;
}

void sithSoundMixer_FreeThing(sithThing *thing)
{
    int *v1; // esi
    int v2; // eax
    int v3; // eax
    int v5; // edx
    int v6; // eax
    stdSound_buffer_t *v7; // [esp-10h] [ebp-18h]
    unsigned int v8; // [esp+4h] [ebp-4h]

    if (!sithSoundMixer_bOpened)
        return;

    for (v8 = 0; v8 < sithSoundMixer_numSoundsAvailable; v8++)
    {
        sithPlayingSound* playingSound = &sithSoundMixer_aPlayingSounds[v8];
        
        if ( (playingSound->flags & SITHSOUNDFLAG_FOLLOWSTHING) != 0 && thing == playingSound->thing )
        {
            if ( (playingSound->flags & SITHSOUNDFLAG_LOOP) != 0 )
            {
                sithSoundMixer_StopSound(playingSound);
            }
            else
            {
                playingSound->flags &= ~SITHSOUNDFLAG_FOLLOWSTHING;
                playingSound->flags |= SITHSOUNDFLAG_ABSOLUTE;
                rdVector_Copy3(&playingSound->pos, &thing->position);
            }
        }
    }
}

sithPlayingSound* sithSoundMixer_GetSoundFromRef(int refid)
{
    unsigned int playingsound_idx; // ecx

    if ( !refid )
        return NULL;

    playingsound_idx = 0;
    for (int i = 0; i < 32; i++)
    {
        if ( sithSoundMixer_aPlayingSounds[i].sound && sithSoundMixer_aPlayingSounds[i].refid == refid )
            break;
        ++playingsound_idx;
    }

    if ( playingsound_idx < 0x20 )
        return &sithSoundMixer_aPlayingSounds[playingsound_idx];
    else
        return NULL;
}

void sithSoundMixer_UpdatePlayingSoundPosition(sithPlayingSound *sound)
{
    rdVector3 *pRelative; // edi
    double v4; // st7
    double v5; // st6
    sithThing *v6; // eax
    double v7; // st7
    sithCamera *v8; // edx
    double v9; // st7
    float v10; // ebx
    double v13; // st7
    char missing_1; // c0
    double v16; // st7
    char missing_2; // c0
    double v19; // st7
    char missing_3; // c0
    char missing_4; // c0
    float sounda; // [esp+14h] [ebp+4h]

    if ( (sound->flags & SITHSOUNDFLAG_ABSOLUTE) != 0 )
    {
        pRelative = &sound->posRelative;
        v4 = sound->pos.y - sithCamera_currentCamera->vec3_1.y;
        v5 = sound->pos.z - sithCamera_currentCamera->vec3_1.z;
        sound->posRelative.x = sound->pos.x - sithCamera_currentCamera->vec3_1.x;
        sound->posRelative.y = v4;
        sound->posRelative.z = v5;
    }
    else
    {
        v6 = sound->thing;
        v7 = v6->position.x;
        v8 = sithCamera_currentCamera;
        sound->pos.x = v6->position.x;
        v9 = v7 - v8->vec3_1.x;
        v10 = v6->position.z;
        sound->pos.y = v6->position.y;
        sound->pos.z = v10;
        pRelative = &sound->posRelative;
        sound->posRelative.x = v9;
        sound->posRelative.y = v6->position.y - v8->vec3_1.y;

        sound->posRelative.z = v6->position.z - v8->vec3_1.z;
        if ( v6->sector && (v6->sector->flags & SITH_SECTOR_UNDERWATER) == 0 ) // added v6->sector
            sound->flags &= ~SITHSOUNDFLAG_UNDERWATER;
        else
            sound->flags |= SITHSOUNDFLAG_UNDERWATER;
    }
    v13 = pRelative->x;
    if ( v13 < 0.0 )
        v13 = -v13;
    if ( v13 > sound->maxPosition )
        goto LABEL_23;
    v16 = sound->posRelative.y;
    if ( v16 < 0.0 )
        v16 = -v16;
    if ( v16 > sound->maxPosition )
        goto LABEL_23;
    v19 = sound->posRelative.z;
    if ( v19 < 0.0 )
        v19 = -v19;
    if ( v19 > sound->maxPosition )
    {
LABEL_23:
        sound->anonymous_13 = sound->maxPosition - -1.0;
    }
    else
    {
        sounda = rdVector_Normalize3QuickAcc(pRelative);
        sound->anonymous_13 = sounda;
        if ( sounda > sound->maxPosition ) // TODO verify
        {
            if ( (sound->flags & SITHSOUNDFLAG_UNDERWATER) == 0 || (sithCamera_currentCamera->sector->flags & SITH_SECTOR_UNDERWATER) != 0 )
            {
                if ( (sound->flags & SITHSOUNDFLAG_UNDERWATER) == 0 && (sithCamera_currentCamera->sector->flags & SITH_SECTOR_UNDERWATER) != 0 )
                    sound->anonymous_13 = sounda * 1.5;
            }
            else
            {
                sound->anonymous_13 = sounda * 1.5;
            }
        }
    }
}

void sithSoundMixer_SectorSound(sithSector *sector, sithSound *sound, float vol)
{
    sector->sectorSound = sound;
    sector->sectorSoundVol = vol;
    if ( sithSoundMixer_pLastSectorSoundSector == sector )
        sithSoundMixer_pLastSectorSoundSector = 0;
}

int sithSoundMixer_sub_4DD3F0(sithPlayingSound *sound)
{
    unsigned int v1; // ecx
    int v2; // esi
    sithPlayingSound* v3; // eax
    sithPlayingSound *v4; // esi
    int v5; // ecx
    sithPlayingSound *v7; // esi
    int v8; // eax
    int v9; // edx
    stdSound_buffer_t *v13; // [esp-4h] [ebp-Ch]
    stdSound_buffer_t *v14; // [esp-4h] [ebp-Ch]

#ifdef OPENAL_SOUND
    jkGuiSound_numChannels = 256;
#endif

    if ( sithSoundMixer_activeChannels >= (unsigned int)jkGuiSound_numChannels )
    {
        v2 = 0;
        while ( sithSoundMixer_dword_836C04 >= 0x20 )
        {
LABEL_8:
            if ( v2 )
                goto LABEL_19;
            sithSoundMixer_dword_836C04 = 0;
            v2 = 1;
        }
        v3 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
        while ( (v3->flags & SITHSOUNDFLAG_PLAYING) == 0 || (v3->flags & (SITHSOUNDFLAG_HIGHEST_PRIO|SITHSOUNDFLAG_HIGHPRIO|SITHSOUNDFLAG_LOOP)) != 0 )
        {
            v3++;
            sithSoundMixer_dword_836C04++;
            if ( v3 >= &sithSoundMixer_aPlayingSounds[32] )
                goto LABEL_8;
        }

        v4 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
        sithSoundMixer_PlayingSoundReset(v4);

        if ( (sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04].flags & SITHSOUNDFLAG_LOOP) == 0 )
        {
            v7 = &sithSoundMixer_aPlayingSounds[sithSoundMixer_dword_836C04];
            
            sithSoundMixer_StopSound(v7);
        }
        sithSoundMixer_dword_836C04++;
    }
LABEL_19:

    //printf("%u\n", sithSoundMixer_activeChannels);

    if ( sithSoundMixer_activeChannels >= (unsigned int)jkGuiSound_numChannels )
        return 0;
    if ( jkGuiSound_b3DSound )
        stdSound_CommitDeferredSettings();
    stdSound_BufferPlay(sound->pSoundBuf, sound->flags & SITHSOUNDFLAG_LOOP);
    sound->flags |= SITHSOUNDFLAG_PLAYING;
    sithSoundMixer_activeChannels++;
    return 1;
}

int sithSoundMixer_sub_4DD5D0(sithPlayingSound *sound)
{
    float a2; // [esp+0h] [ebp-8h]

    sound->pSoundBuf = sithSound_LoadData(sound->sound);
    if ( sound->pSoundBuf )
    {
        a2 = sound->vol_2 * 0.75;
        ++sound->sound->field_40;
        stdSound_BufferSetVolume(sound->pSoundBuf, a2);
        if ( jkGuiSound_b3DSound )
        {
            sound->p3DSoundObj = stdSound_BufferQueryInterface(sound->pSoundBuf);
            if ( sound->p3DSoundObj )
                stdSound_BufferSetVolume(sound->pSoundBuf, sound->vol_2);
        }
        return 1;
    }
    return 0;
}

uint32_t sithSoundMixer_GetThingSoundIdx(sithThing *thing, sithSound *sound)
{
    unsigned int result; // eax
    sithPlayingSound* i; // ecx

    result = 0;
    if ( !sithSoundMixer_numSoundsAvailable )
        return -1;
    for ( i = &sithSoundMixer_aPlayingSounds[0]; i->sound != sound || thing && i->thing != thing; i++)
    {
        if ( ++result >= sithSoundMixer_numSoundsAvailable )
            return -1;
    }
    return result;
}

void sithSoundMixer_StopSound(sithPlayingSound *sound)
{
    int v1; // eax
    sithSound *v2; // eax
    int v3; // edx
    int v4; // eax
    stdSound_buffer_t *v5; // [esp-4h] [ebp-Ch]

    if ( (sound->flags & SITHSOUNDFLAG_PLAYING) != 0 )
    {
        sithSoundMixer_PlayingSoundReset(sound);
    }
    if ( sound->pSoundBuf )
    {
        stdSound_BufferRelease(sound->pSoundBuf);
        sound->pSoundBuf = 0;
    }
    if ( sound->p3DSoundObj )
    {
        stdSound_3DBufferRelease(sound->p3DSoundObj);
        sound->p3DSoundObj = 0;
    }

    sithSoundMixer_FreePlayingSound(sound);
}

sithPlayingSound* sithSoundMixer_GetSoundFromIdx(signed int idx)
{
    sithPlayingSound *result; // eax

    if ( idx < 0 || idx >= 32 )
        result = 0;
    else
        result = &sithSoundMixer_aPlayingSounds[idx];
    return result;
}