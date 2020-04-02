#ifndef _SDLFUNCS_H_
#define _SDLFUNCS_H_

#include "SDL.h"
#include "SDL_thread.h"
#include "SDL_syswm.h"

#ifdef _WIN32
        #define HACK_HACK_HACK __cdecl
#else
        #define HACK_HACK_HACK
#endif

struct sdlfuncs {
	int	(HACK_HACK_HACK *Init)	(Uint32 flags);
	void	(HACK_HACK_HACK *Quit)	(void);
	int	(HACK_HACK_HACK *mutexP)	(SDL_mutex *mutex);
	int	(HACK_HACK_HACK *mutexV)	(SDL_mutex *mutex);
	int	(HACK_HACK_HACK *PeepEvents)	(SDL_Event *events, int numevents,
					SDL_eventaction action, Uint32 minType, Uint32 maxType);
	char	*(HACK_HACK_HACK *GetCurrentVideoDriver)	(void);
	int	(HACK_HACK_HACK *SemWait)	(SDL_sem *sem);
	int	(HACK_HACK_HACK *SemWaitTimeout)(SDL_sem *sem, Uint32 timeout);
	int	(HACK_HACK_HACK *SemPost)	(SDL_sem *sem);
	Uint8	(HACK_HACK_HACK *EventState)	(Uint32 type, int state);
	SDL_Surface *(HACK_HACK_HACK *CreateRGBSurfaceFrom)(void *pixels, int width, int height, int depth, int pitch,
							Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
	void	(HACK_HACK_HACK *RenderPresent)	(SDL_Renderer *renderer);
	SDL_sem *(HACK_HACK_HACK *SDL_CreateSemaphore)	(Uint32 initial_value);
	void (HACK_HACK_HACK *SDL_DestroySemaphore)	(SDL_sem *semaphore);
	SDL_mutex	*(HACK_HACK_HACK *SDL_CreateMutex)	(void);
	int	(HACK_HACK_HACK *WaitEventTimeout)	(SDL_Event *event, int timeout);
	int (HACK_HACK_HACK *CreateWindowAndRenderer)	(int w, int h, Uint32 flags, SDL_Window **win, SDL_Renderer **ren);
	void	(HACK_HACK_HACK *FreeSurface)	(SDL_Surface *surface);
	void	(HACK_HACK_HACK *SetWindowTitle)	(SDL_Window *window, const char *title);
	void	(HACK_HACK_HACK *GetWindowSize)	(SDL_Window *window, int *w, int *h);
	void	(HACK_HACK_HACK *SetWindowIcon)	(SDL_Window *win, SDL_Surface *icon);
	int	(HACK_HACK_HACK *ShowCursor)	(int toggle);
	Uint32	(HACK_HACK_HACK *WasInit)	(Uint32 flags);
	SDL_bool (HACK_HACK_HACK *GetWindowWMInfo) (SDL_Window *window, SDL_SysWMinfo *info);
	const char	*(HACK_HACK_HACK *GetError)	(void);
	int (HACK_HACK_HACK *InitSubSystem)(Uint32 flags);
	void (HACK_HACK_HACK *QuitSubSystem)(Uint32 flags);
	int (HACK_HACK_HACK *OpenAudio)(SDL_AudioSpec *desired, SDL_AudioSpec *obtained);
	void (HACK_HACK_HACK *CloseAudio)(void);
	void (HACK_HACK_HACK *LockAudio)(void);
	void (HACK_HACK_HACK *UnlockAudio)(void);
	void (HACK_HACK_HACK *PauseAudio)(int pause_on);
	SDL_AudioStatus (HACK_HACK_HACK *GetAudioStatus)(void);
	SDL_Texture* (HACK_HACK_HACK *CreateTexture)	(SDL_Renderer *renderer, Uint32 format, int access, int w, int h);
	int (HACK_HACK_HACK *UpdateTexture)	(SDL_Texture *texture, const SDL_Rect * rect, const void *pixels, int pitch);
	int (HACK_HACK_HACK *RenderClear)	(SDL_Renderer *renderer);
	int (HACK_HACK_HACK *RenderCopy)	(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *srcrect, const SDL_Rect *dstrect);
	SDL_bool (HACK_HACK_HACK *SetHint)	(const char *name, const char *value);
	const char * (HACK_HACK_HACK *GetHint)	(const char *name);
	SDL_Keymod (HACK_HACK_HACK *GetModState)	(void);
	void (HACK_HACK_HACK *SetWindowSize)	(SDL_Window *window, int w, int h);
	void (HACK_HACK_HACK *DestroyTexture)	(SDL_Texture *texture);
	int	gotfuncs;
};

/* Defined in SDL_win32_main.c for Win32 */
extern struct sdlfuncs	sdl;
extern SDL_sem *sdl_exit_sem;

#ifdef __cplusplus
extern "C" {
#endif
int load_sdl_funcs(struct sdlfuncs *sdlf);
int init_sdl_audio(void);
int init_sdl_video(void);
#ifdef __cplusplus
}
#endif

#endif
