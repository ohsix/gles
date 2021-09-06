#include <stdlib.h>
#include <limits.h>

#include "SDL.h"
#include "SDL2/SDL_opengles2.h"
#include "SDL2/SDL_image.h"

#ifdef HAVE_GSL
#include "gsl/gsl_histogram.h"
#endif

#ifdef __EMSCRIPTEN__
#include "emscripten.h"
#include "emscripten/html5.h"
#endif

/* map the cursor position to a display index */
static int mouse_on_display()
{
	int mx, my;
	int displays = SDL_GetNumVideoDisplays();

	if(displays == 1)
		return 0;

	SDL_GetGlobalMouseState(&mx, &my);

	while(displays-- > 0)
	{
		SDL_Rect db;

		SDL_GetDisplayBounds(displays, &db);

		if(mx >= db.x && mx <= db.x + db.w && my >= db.y && my <= db.y + db.h)
			return displays;
	}

	return 0;
}

/* used for timestamps */
static double pc_freq = 0;
static uint64_t pc_init = 0;

double now()
{
	if(pc_init == 0)
	{
		pc_freq = SDL_GetPerformanceFrequency();
		pc_init = SDL_GetPerformanceCounter();
	}

	return (SDL_GetPerformanceCounter() - pc_init) / pc_freq;
}
	
Uint32 EV_TIME;

enum { TIME_BEGIN_FRAME, TIME_END_FRAME, TIME_FRAME_DONE };

Uint32 frame_start;
Uint32 last_frame_time;

int collect_events(void* user, SDL_Event* e)
{
	if(e->type == EV_TIME)
	{
		SDL_UserEvent* ue = (SDL_UserEvent*)e;
		switch(ue->code)
		{
			case TIME_BEGIN_FRAME:
				frame_start = ue->timestamp;
				break;

			case TIME_END_FRAME:
				last_frame_time = ue->timestamp - frame_start;
				frame_start = 0;
				break;

			default:
				break;
		}

		return 0;
	}
				
	return 1;
}

void init_events()
{
	EV_TIME = SDL_RegisterEvents(1);
	SDL_SetEventFilter(collect_events, NULL);
}

void post_event(Sint32 code)
{
	SDL_UserEvent e = { e.type = EV_TIME, e.code = code };

	e.type = EV_TIME;
	e.code = code;

	SDL_PushEvent((SDL_Event*)&e);
}

float merb()
{
	return (float)rand() / (float)(RAND_MAX);
}

#ifdef HAVE_GSL
gsl_histogram* h = NULL;
#endif

void setup(SDL_Window* w)
{
#ifdef HAVE_GSL
	h = gsl_histogram_alloc(50);

	gsl_histogram_set_ranges_uniform(h, 58, 62);
#endif
}

void draw(SDL_Window* w)
{
	post_event(TIME_BEGIN_FRAME);

//	SDL_Delay((1000.0f/60) * (1.0f + (merb() * 2.0f)));
	SDL_Delay(merb() * 16.0f);
	glClearColor(0.0f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapWindow(w);
	post_event(TIME_END_FRAME);
}

/* run while returning not zero */
int iter(double time, void* u)
{
	SDL_Window* w = (SDL_Window*)u;
	SDL_Event e;
	
	while(SDL_PollEvent(&e))
	{
		switch(e.type)
		{
			case SDL_QUIT:
				return 0;
				break;

			case SDL_WINDOWEVENT:
				break;

			case SDL_KEYDOWN:
				if(((SDL_KeyboardEvent*)&e)->keysym.sym == SDLK_ESCAPE)
					return 0;
			case SDL_KEYUP:
				break;

			case SDL_MOUSEMOTION:
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				break;

			case SDL_TEXTEDITING:
			case SDL_KEYMAPCHANGED:
				break;

			case SDL_AUDIODEVICEADDED:
			case SDL_AUDIODEVICEREMOVED:
				break;

			default:
				SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, "unhandled SDL_Event: 0x%08x", e.type);
		}
	}

	draw(w);

#ifdef HAVE_GSL
	gsl_histogram_increment(h, 1000.0f / last_frame_time);
#endif

	return 1;
}

int main(int argc, char* argv[])
{
	SDL_Window* w;
	SDL_GLContext gl;

	SDL_SetHint(SDL_HINT_EVENT_LOGGING, "1"); // log all but input
	SDL_InitSubSystem(SDL_INIT_VIDEO);

#ifndef __EMSCRIPTEN__
	// causes context creation errors
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

	/* OpenGL ES 2.0 */
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

	/* SDL doesn't give me the visual i want for a window with an alpha channel
	 *	get:
		Visual ID: 132  depth=24  class=DirectColor, type=window,pixmap,pbuffer
		    bufferSize=24 level=0 renderType=rgba doubleBuffer=1 stereo=0
		    rgba: redSize=8 greenSize=8 blueSize=8 alphaSize=0 float=N sRGB=N
		    auxBuffers=0 depthSize=24 stencilSize=8
		    accum: redSize=0 greenSize=0 blueSize=0 alphaSize=0
		    multiSample=0  multiSampleBuffers=0
		    visualCaveat=None
		    Opaque.

		  pict format:
        		format id:    0x2a
		        type:         Direct
		        depth:        24
		        alpha:         0 mask 0x0
		        red:          16 mask 0xff
		        green:         8 mask 0xff
		        blue:          0 mask 0xff

		want:
		Visual ID: b3  depth=32  class=TrueColor, type=window,pixmap,pbuffer
		    bufferSize=32 level=0 renderType=rgba doubleBuffer=1 stereo=0
		    rgba: redSize=8 greenSize=8 blueSize=8 alphaSize=8 float=N sRGB=N
		    auxBuffers=0 depthSize=24 stencilSize=8
		    accum: redSize=0 greenSize=0 blueSize=0 alphaSize=0
		    multiSample=0  multiSampleBuffers=0
		    visualCaveat=None
		    Opaque.

		  pict format:
		        format id:    0x26
		        type:         Direct
		        depth:        32
		        alpha:        24 mask 0xff
		        red:          16 mask 0xff
		        green:         8 mask 0xff
		        blue:          0 mask 0xff

		XRender has to be used to to distinguish between pict format 0x2a and 0x26
		xdpyinfo -ext RENDER displays pict format info */
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
//	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
//	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	init_events();

	int di = mouse_on_display();
	SDL_Log("di = %d", di);

	w = SDL_CreateWindow("", SDL_WINDOWPOS_UNDEFINED_DISPLAY(di), SDL_WINDOWPOS_UNDEFINED_DISPLAY(di), 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);

//	SDL_SetWindowOpacity(w, 0.5f);

	gl = SDL_GL_CreateContext(w);
	SDL_Log("gl: %p", gl);

	SDL_Log("pf: 0x%08x", SDL_GetWindowPixelFormat(w));

	int alpha;

	SDL_GL_GetAttribute(SDL_GL_ALPHA_SIZE, &alpha);
	SDL_Log("alpha: %d", alpha);

	SDL_ShowWindow(w);

	setup(w);

#ifdef __EMSCRIPTEN__
	emscripten_request_animation_frame_loop(iter, 0);
#else
	while(iter(now(), (void*)w))
		;

	SDL_Log("exit time");

	SDL_GL_DeleteContext(gl);
	SDL_DestroyWindow(w);

#ifdef HAVE_GSL
	FILE* ho = fopen("frame_histogram.dat", "w");
	gsl_histogram_fprintf(ho, h, "%g", "%g");
	fclose(ho);
#endif

#endif	
	return EXIT_SUCCESS;
}

