#include "MainMenuState.h"

#include <GL/glew.h>
#include <SDL.h>
#include <glm/gtc/type_ptr.hpp>
#include <stb_image.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#include <nuklear.h>
#include "../nuklear_sdl_gl3.h"
#include "../style.h"

struct nk_context* ctx;
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

MainMenuState::MainMenuState(AppSettings* appSettings, SMQueue* smQueue, SDL_Window* mWindow) :
	State(appSettings, smQueue),
	fCurTime(0.f)
{
	audioCueSubject = new AudioCueSubject();
	mAudioSys = new AudioSys(audioCueSubject, appSettings->strAssetSrc);

	//nuklear
	fNukWidth = appSettings->mWidth / 2.5f;
	fNukHeight = appSettings->mHeight / 2.5f;
	fNukX = appSettings->mWidth / 2.f - fNukWidth / 2.f;
	fNukY = appSettings->mHeight / 2.f - fNukHeight / 2.f;
	ctx = nk_sdl_init(mWindow);
	struct nk_font_atlas* atlas;
	nk_sdl_font_stash_begin(&atlas);
	nk_sdl_font_stash_end();
	set_style(ctx, THEME_RED);

	vBkColor[0].r = .28f, vBkColor[0].g = 1.f, vBkColor[0].b = 1.f;
	vBkColor[1].r = 1.f, vBkColor[1].g = .28f, vBkColor[1].b = 1.f;
	vBkColor[2].r = 1.f, vBkColor[2].g = 1.f, vBkColor[2].b = .28f;
	iBkColIndex = 0;
}

MainMenuState::~MainMenuState()
{
	delete audioCueSubject;
	delete mAudioSys;
}

void MainMenuState::run(SDL_Window* mWindow)
{
	Uint32 uTicksLastFrame = SDL_GetTicks();
	while (true)
	{
		//update
		SDL_Event e;
		nk_input_begin(ctx);
		while (SDL_PollEvent(&e))
		{
			nk_sdl_handle_event(&e);

			switch (e.type)
			{
			case SDL_KEYDOWN:
				//universal state input processing
				switch (e.key.keysym.sym)
				{
				case SDLK_RETURN:
					smQueue->push(SMMessage::PUSH_PLAY_STATE);
					return;

				case SDLK_ESCAPE:
					smQueue->push(SMMessage::POP);
					return;
				}

				break;

			case SDL_QUIT:
				smQueue->push(SMMessage::QUIT);
				return;
			}
		}
		nk_input_end(ctx);

		float fDeltaTime = static_cast<float>(SDL_GetTicks() - uTicksLastFrame) / 1000.f;
		uTicksLastFrame = SDL_GetTicks();

		fCurTime += fDeltaTime;
		if (fCurTime > 3.f)
		{
			fCurTime -= 3.f;
			iBkColIndex = iBkColIndex + 1 > 2 ? 0 : iBkColIndex + 1;
			audioCueSubject->notify();
		}

		if (nk_begin(ctx, "glRoom", nk_rect(fNukX, fNukY, fNukWidth, fNukHeight), NK_WINDOW_TITLE | NK_WINDOW_BORDER))
		{

			nk_layout_row_dynamic(ctx, 22, 1);
			nk_label_colored(ctx, "Controls :", NK_TEXT_ALIGN_LEFT, nk_color(255, 0, 0, 255));

			nk_label(ctx, "Enter - Play", 1);
			nk_label(ctx, "Left Click - Pick/Move objects", 1);
			nk_label(ctx, "Right Click - Camera Motion", 1);
			nk_label(ctx, "Scroll - Camera Zoom", 1);
			nk_label(ctx, "ESC - Quit", 1);
		
			nk_property_float(ctx, "Window Size:", 0.5f, &appSettings->fWindowSize, 1.5f, 0.02, 1);
		
			nk_label_colored(ctx, "Restart application to update window size.", NK_TEXT_ALIGN_LEFT, nk_color(255, 0, 0, 255));
			nk_label_colored(ctx, "//chirag", NK_TEXT_ALIGN_LEFT, nk_color(0, 255, 0, 255));

		}
		nk_end(ctx);

		glClearColor(vBkColor[iBkColIndex].r, vBkColor[iBkColIndex].g, vBkColor[iBkColIndex].b, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		nk_sdl_render(NK_ANTI_ALIASING_ON, MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
		SDL_GL_SwapWindow(mWindow);
	}
}
