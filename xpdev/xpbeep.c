/* $Id: xpbeep.c,v 1.25 2006/04/13 07:41:57 deuce Exp $ */

/* standard headers */
#include <math.h>

#if defined(_WIN32)
	#include <windows.h>
	#include <mmsystem.h>
#elif defined(__unix__)
	#include <fcntl.h>
	#if SOUNDCARD_H_IN==1
		#include <sys/soundcard.h>
	#elif SOUNDCARD_H_IN==2
		#include <soundcard.h>
	#elif SOUNDCARD_H_IN==3
		#include <linux/soundcard.h>
	#else
		#ifndef USE_ALSA
			#warning Cannot find soundcard.h
		#endif
	#endif
	#ifdef USE_ALSA_SOUND
		#include <alsa/asoundlib.h>
	#endif
	/* KIOCSOUND */
	#if defined(__FreeBSD__)
		#include <sys/kbio.h>
	#elif defined(__linux__)
		#include <sys/kd.h>	
	#elif defined(__solaris__)
		#include <sys/kbio.h>
		#include <sys/kbd.h>
	#endif
	#if defined(__OpenBSD__) || defined(__NetBSD__)
		#include <machine/spkr.h>
	#elif defined(__FreeBSD__)
		#include <machine/speaker.h>
	#endif
#endif

/* xpdev headers */
#include "genwrap.h"
#include "xpbeep.h"

#define S_RATE	22050

static BOOL sound_device_open_failed=FALSE;
static BOOL alsa_device_open_failed=FALSE;

#define WAVE_PI	3.14159265358979323846
#define WAVE_TPI 6.28318530717958647692

#ifdef USE_ALSA_SOUND
struct alsa_api_struct {
	int		(*snd_pcm_open)
				(snd_pcm_t **pcm, const char *name, snd_pcm_stream_t stream, int mode);
	int		(*snd_pcm_hw_params_malloc)
				(snd_pcm_hw_params_t **ptr);
	int		(*snd_pcm_hw_params_any)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
	int		(*snd_pcm_hw_params_set_access)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
	int		(*snd_pcm_hw_params_set_format)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
	int		(*snd_pcm_hw_params_set_rate_near)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
	int		(*snd_pcm_hw_params_set_channels)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
	int		(*snd_pcm_hw_params)
				(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
	int		(*snd_pcm_prepare)
				(snd_pcm_t *pcm);
	void	(*snd_pcm_hw_params_free)
				(snd_pcm_hw_params_t *obj);
	int 	(*snd_pcm_close)
				(snd_pcm_t *pcm);
	snd_pcm_sframes_t (*snd_pcm_writei)
				(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
};

struct alsa_api_struct *alsa_api=NULL;
#endif

/********************************************************************************/
/* Calculate and generate a sound wave pattern (thanks to Deuce!)				*/
/********************************************************************************/
void makewave(double freq, unsigned char *wave, int samples, enum WAVE_SHAPE shape)
{
	int	i;
	int j;
	int midpoint;
	double inc;
	double pos;
	BOOL endhigh;
	BOOL starthigh;

	midpoint=samples/2;
	inc=8.0*atan(1.0);
	inc *= ((double)freq / (double)S_RATE);

	for(i=0;i<samples;i++) {
		pos=(inc*(double)i);
		pos -= (int)(pos/WAVE_TPI)*WAVE_TPI;
		switch(shape) {
			case WAVE_SHAPE_SINE:
				wave[i]=(sin (pos))*127+128;
				break;
			case WAVE_SHAPE_SINE_HARM:
				wave[i]=(sin (pos))*64+128;
				wave[i]=(sin ((inc*2)*(double)i))*24;
				wave[i]=(sin ((inc*3)*(double)i))*16;
				break;
			case WAVE_SHAPE_SAWTOOTH:
				wave[i]=(WAVE_TPI-pos)*40.5;
				break;
			case WAVE_SHAPE_SQUARE:
				wave[i]=(pos<WAVE_PI)?255:0;
				break;
			case WAVE_SHAPE_SINE_SAW:
				wave[i]=(((sin (pos))*127+128)+((WAVE_TPI-pos)*40.5))/2;
				break;
			case WAVE_SHAPE_SINE_SAW_CHORD:
				wave[i]=(((sin (pos))*64+128)+((WAVE_TPI-pos)*6.2))/2;
				wave[i]+=(sin ((inc/2)*(double)i))*24;
				wave[i]+=(sin ((inc/3)*(double)i))*16;
				break;
			case WAVE_SHAPE_SINE_SAW_HARM:
				wave[i]=(((sin (pos))*64+128)+((WAVE_TPI-pos)*6.2))/2;
				wave[i]+=(sin ((inc*2)*(double)i))*24;
				wave[i]+=(sin ((inc*3)*(double)i))*16;
				break;
		}
	}
	
	/* Now we have a "perfect" wave... 
	 * we must clean it up now to avoid click/pop
	 */
	if(wave[samples-1]>128)
		endhigh=TRUE;
	else
		endhigh=FALSE;
	/* Completely remove the last wave fragment */
	i=samples-1;
	if(wave[i]!=128) {
		for(;i>midpoint;i--) {
			if(endhigh && wave[i]<128)
				break;
			if(!endhigh && wave[i]>128)
				break;
			wave[i]=128;
		}
	}

#if 0
	if(wave[0]>128)
		starthigh=TRUE;
	else
		starthigh=FALSE;
	/* Completely remove the first wave fragment */
	i=0;
	if(wave[i]!=128) {
		for(;i<midpoint;i++) {
			if(starthigh && wave[i]<128)
				break;
			if(!starthigh && wave[i]>128)
				break;
			wave[i]=128;
		}
	}
#endif
}

/********************************************************************************/
/* Play a tone through the wave/DSP output device (sound card) - Deuce			*/
/********************************************************************************/

#if defined(_WIN32)

BOOL xptone(double freq, DWORD duration, enum WAVE_SHAPE shape)
{
	WAVEFORMATEX	w;
	WAVEHDR			wh;
	HWAVEOUT		waveOut;
	unsigned char	wave[S_RATE*15/2+1];
	WORD* 			p;
	DWORD			endTime;
	BOOL			success=FALSE;

	w.wFormatTag = WAVE_FORMAT_PCM;
	w.nChannels = 1;
	w.nSamplesPerSec = S_RATE;
	w.wBitsPerSample = 8;
	w.nBlockAlign = (w.wBitsPerSample * w.nChannels) / 8;
	w.nAvgBytesPerSec = w.nSamplesPerSec * w.nBlockAlign;

	if(!sound_device_open_failed && waveOutOpen(&waveOut, WAVE_MAPPER, &w, 0, 0, 0)!=MMSYSERR_NOERROR)
		sound_device_open_failed=TRUE;
	if(sound_device_open_failed)
		return(FALSE);

	memset(&wh, 0, sizeof(wh));
	wh.lpData=wave;
	wh.dwBufferLength=S_RATE*duration/1000;
	if(wh.dwBufferLength<=S_RATE/freq*2)
		wh.dwBufferLength=S_RATE/freq*2;

	makewave(freq,wave,wh.dwBufferLength,shape);
	if(waveOutPrepareHeader(waveOut, &wh, sizeof(wh))!=MMSYSERR_NOERROR)
		goto abrt;
	if(waveOutWrite(waveOut, &wh, sizeof(wh))==MMSYSERR_NOERROR)
		success=TRUE;
abrt:
	while(waveOutUnprepareHeader(waveOut, &wh, sizeof(wh))==WAVERR_STILLPLAYING)
		SLEEP(1);
	waveOutClose(waveOut);

	return(success);
}

#elif defined(__unix__)

BOOL DLLCALL xptone(double freq, DWORD duration, enum WAVE_SHAPE shape)
{
#if defined(USE_ALSA_SOUND) || defined(AFMT_U8)
	unsigned char	wave[S_RATE*15/2+1];
	int samples;
#endif

#ifdef USE_ALSA_SOUND
	snd_pcm_t *playback_handle;
	snd_pcm_hw_params_t *hw_params=NULL;
	void *dl;
#endif

#ifdef AFMT_U8
	int dsp;
	int format=AFMT_U8;
	int channels=1;
	int	rate=S_RATE;
	int	fragsize=0x7fff0004;
	int wr;
	int	i;
#endif

#if defined(USE_ALSA_SOUND) || defined(AFMT_U8)
	if(freq<17)
		freq=17;
	samples=S_RATE*duration/1000;
	if(samples<=S_RATE/freq*2)
		samples=S_RATE/freq*2;
	makewave(freq,wave,samples,shape);
#endif

#ifdef USE_ALSA_SOUND
	if(!alsa_device_open_failed) {
		if(alsa_api==NULL) {
			
			if(((alsa_api=(struct alsa_api_struct *)malloc(sizeof(struct alsa_api_struct)))==NULL)
					|| ((dl=dlopen("libasound.so",RTLD_LAZY))==NULL)
					|| ((alsa_api->snd_pcm_open=dlsym(dl,"snd_pcm_open"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_malloc=dlsym(dl,"snd_pcm_hw_params_malloc"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_any=dlsym(dl,"snd_pcm_hw_params_any"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_set_access=dlsym(dl,"snd_pcm_hw_params_set_access"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_set_format=dlsym(dl,"snd_pcm_hw_params_set_format"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_set_rate_near=dlsym(dl,"snd_pcm_hw_params_set_rate_near"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_set_channels=dlsym(dl,"snd_pcm_hw_params_set_channels"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params=dlsym(dl,"snd_pcm_hw_params"))==NULL)
					|| ((alsa_api->snd_pcm_prepare=dlsym(dl,"snd_pcm_prepare"))==NULL)
					|| ((alsa_api->snd_pcm_hw_params_free=dlsym(dl,"snd_pcm_hw_params_free"))==NULL)
					|| ((alsa_api->snd_pcm_close=dlsym(dl,"snd_pcm_close"))==NULL)
					|| ((alsa_api->snd_pcm_writei=dlsym(dl,"snd_pcm_writei"))==NULL)
					|| ((alsa_api->=dlsym(dl,""))==NULL)) {
				FREE_AND_NULL(alsa_api);
			}
			if(alsa_api==NULL)
				alsa_device_open_failed=TRUE;
			else {
			}
		}
		if(alsa_api!=NULL) {
			if((alsa_api->snd_pcm_open(&playback_handle, argv[1], SND_PCM_STREAM_PLAYBACK, 0)<0)
					|| (alsa_api->snd_pcm_hw_params_malloc(&hw_params)<0)
					|| (alsa_api->snd_pcm_hw_params_any(playback_handle, hw_params)<0)
					|| (alsa_api->snd_pcm_hw_params_set_access(playback_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
					|| (alsa_api->snd_pcm_hw_params_set_format(playback_handle, hw_params, SND_PCM_FORMAT_U8) < 0)
					|| (alsa_api->snd_pcm_hw_params_set_rate_near(playback_handle, hw_params, S_RATE, 0) < 0) 
					|| (alsa_api->snd_pcm_hw_params_set_channels(playback_handle, hw_params, 1) < 0)
					|| (alsa_api->snd_pcm_hw_params(playback_handle, hw_params) < 0)
					|| (alsa_api->snd_pcm_prepare(playback_handle) < 0)) {
				alsa_device_open_failed=TRUE;
				if(hw_params!=NULL)
					alsa_api->snd_pcm_hw_params_free(hw_params);
				if(playback_handle!=NULL)
					alsa_api->snd_pcm_close(playback_handle);
			}
			else {
				alsa_api->snd_pcm_hw_params_free(hw_params);
				if(alsa_api->snd_pcm_writei(playback_handle, wave, samples)!=samples)
					alsa_device_open_failed=TRUE;
				else {
					alsa_api->snd_pcm_close (playback_handle);
					return(TRUE);
				}
				alsa_api->snd_pcm_close (playback_handle);
			}
		}
		if(alsa_device_open_failed)
			FREE_AND_NULL(alsa_api);
	}
#endif

#ifdef AFMT_U8
	if(!sound_device_open_failed) {
		if((dsp=open("/dev/dsp",O_WRONLY,0))<0) {
			sound_device_open_failed=TRUE;
		}
		else  {
			ioctl(dsp, SNDCTL_DSP_SETFRAGMENT, &fragsize);
			if((ioctl(dsp, SNDCTL_DSP_SETFMT, &format)==-1) || format!=AFMT_U8) {
				sound_device_open_failed=TRUE;
				close(dsp);
			}
			else if((ioctl(dsp, SNDCTL_DSP_CHANNELS, &channels)==-1) || channels!=1) {
				sound_device_open_failed=TRUE;
				close(dsp);
			}
			else if((ioctl(dsp, SNDCTL_DSP_SPEED, &rate)==-1) || rate!=S_RATE) {
				sound_device_open_failed=TRUE;
				close(dsp);
			}
		}
	}
	if(sound_device_open_failed)
		return(FALSE);
	wr=0;
	while(wr<samples) {
		i=write(dsp, wave+wr, samples-wr);
		if(i>=0)
			wr+=i;
	}
	close(dsp);

	return(TRUE);
#else
	return(FALSE);
#endif
}

/****************************************************************************/
/* Generate a tone at specified frequency for specified milliseconds		*/
/* Thanks to Casey Martin (and Deuce) for this code							*/
/****************************************************************************/
void DLLCALL unix_beep(int freq, int dur)
{
	static int console_fd=-1;

#if defined(__OpenBSD__) || defined(__FreeBSD__) || defined(__NetBSD__)
	int speaker_fd=-1;
	tone_t tone;

	speaker_fd = open("/dev/speaker", O_WRONLY|O_APPEND);
	if(speaker_fd != -1)  {
		tone.frequency=freq;
		tone.duration=dur/10;
		ioctl(speaker_fd,SPKRTONE,&tone);
		close(speaker_fd);
		return;
	}
#endif

#if !defined(__GNU__) && !defined(__QNX__) && !defined(__OpenBSD__) && !defined(__NetBSD__) && !defined(__APPLE__) && !defined(__CYGWIN__)
	if(console_fd == -1) 
  		console_fd = open("/dev/console", O_NOCTTY);
	
	if(console_fd != -1) {
#if defined(__solaris__)
		ioctl(console_fd, KIOCCMD, KBD_CMD_BELL);
#else
		if(freq != 0)	/* Don't divide by zero */
			ioctl(console_fd, KIOCSOUND, (int) (1193180 / freq));
#endif /* solaris */
		SLEEP(dur);
#if defined(__solaris__)
		ioctl(console_fd, KIOCCMD, KBD_CMD_NOBELL);	/* turn off tone */
#else
		ioctl(console_fd, KIOCSOUND, 0);	/* turn off tone */
#endif /* solaris */
	}
#endif
}

#endif

/********************************************************************************/
/* Play sound through DSP/wave device, if unsuccessful, play through PC speaker	*/
/********************************************************************************/
void xpbeep(double freq, DWORD duration)
{
	if(xptone(freq,duration,WAVE_SHAPE_SINE_SAW_HARM))
		return;

#if defined(_WIN32)
	Beep((DWORD)(freq+.5), duration);
#elif defined(__unix__)
	unix_beep((int)(freq+.5),duration);
#endif
}
