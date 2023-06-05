#include "InputSys.h"
#include "SystemComponents.h"

#include <LinearMath/btIDebugDraw.h>
#include <SDL_events.h>

InputSys::InputSys(
	SMQueue* smQueue,
	entt::registry* mRegistry,
	btDiscreteDynamicsWorld* dynamicsWorld,
	LBtnPressedSubject* lBtnPressedSubject,
	LBtnMotionSubject* lBtnMotionSubject,
	LBtnReleasedSubject* lBtnReleasedSubject,
	RBtnMotionSubject* rBtnMotionSubject,
	MBtnMotionSubject* mBtnMotionSubject,
	MouseScrollSubject* mouseScrollSubject) :
	smQueue(smQueue),
	mRegistry(mRegistry),
	dynamicsWorld(dynamicsWorld),
	bLBtnDown(false),
	bRBtnDown(false),
	bMBtnDown(false),
	lBtnMotionSubject(lBtnMotionSubject),
	rBtnMotionSubject(rBtnMotionSubject),
	mBtnMotionSubject(mBtnMotionSubject),
	mouseScrollSubject(mouseScrollSubject),
	lBtnPressedSubject(lBtnPressedSubject),
	lBtnReleasedSubject(lBtnReleasedSubject)
{
}

InputSys::~InputSys()
{
}

bool InputSys::update()
{
	SDL_Event e;
	while (SDL_PollEvent(&e))
	{
		switch (e.type)
		{
		case SDL_MOUSEBUTTONDOWN:
			if (!bLBtnDown && !bRBtnDown && !bMBtnDown)
			{
				if (e.button.button == SDL_BUTTON_LEFT)
				{
					int x, y;
					SDL_GetMouseState(&x, &y);
					bLBtnDown = true;
					lBtnPressedSubject->notify(x, y);
				}
				else if (e.button.button == SDL_BUTTON_RIGHT)
				{
					int x, y;
					SDL_GetRelativeMouseState(&x, &y);					//get relative mouse state here so the value is delta in SDL_MOUSEMOTION
					bRBtnDown = true;
				}
				else
				{
					int x, y;
					SDL_GetRelativeMouseState(&x, &y);					//get relative mouse state here so the value is delta in SDL_MOUSEMOTION
					bMBtnDown = true;
				}
			}
			break;

		case SDL_MOUSEMOTION:
		{
			if (bLBtnDown)
			{
				int relX, relY, x, y;
				SDL_GetMouseState(&x, &y);
				SDL_GetRelativeMouseState(&relX, &relY);
				lBtnMotionSubject->notify(x, y, relX, relY);
			}
			else if (bRBtnDown)
			{
				int relX, relY, x, y;
				SDL_GetMouseState(&x, &y);
				SDL_GetRelativeMouseState(&relX, &relY);
				rBtnMotionSubject->notify(x, y, relX, relY);
			}
			else if (bMBtnDown)
			{
			}
		}
			break;

		case SDL_MOUSEBUTTONUP:
			if (bLBtnDown)
			{
				lBtnReleasedSubject->notify();
				bLBtnDown = false;
			}
			else if (bRBtnDown)
				bRBtnDown = false;
			else if (bMBtnDown)
				bMBtnDown = false;
			break;

		case SDL_MOUSEWHEEL:
			mouseScrollSubject->notify(e.wheel.x, e.wheel.y);
			break;

		case SDL_KEYDOWN:
			switch (e.key.keysym.sym)
			{
				//set debug draw mode for rendering system
				//view[0] is used since only 1 entity is allowed a System Component
			case SDLK_1:
				mRegistry->patch<SCDrawMode>(
					mRegistry->view<SCDrawMode>()[0],
					[](SCDrawMode& scDrawMode)
					{
						scDrawMode.drawMode = DrawMode::NORMAL;
					}
					);
				dynamicsWorld->getDebugDrawer()->setDebugMode(0);
				break;
			case SDLK_2:
				mRegistry->patch<SCDrawMode>(
					mRegistry->view<SCDrawMode>()[0],
					[](SCDrawMode& scDrawMode)
					{
						scDrawMode.drawMode = DrawMode::NORMAL_DEBUG;
					}
					);
				dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawAabb + btIDebugDraw::DBG_DrawConstraints + btIDebugDraw::DBG_DrawWireframe);
				break;
			case SDLK_3:
				mRegistry->patch<SCDrawMode>(
					mRegistry->view<SCDrawMode>()[0], 
					[](SCDrawMode& scDrawMode)
					{
						scDrawMode.drawMode = DrawMode::DEBUG;
					}
					);
				dynamicsWorld->getDebugDrawer()->setDebugMode(btIDebugDraw::DBG_DrawAabb + btIDebugDraw::DBG_DrawConstraints + btIDebugDraw::DBG_DrawWireframe);
				break;


			case SDLK_ESCAPE:
				smQueue->push(SMMessage::POP);
				return true;
			}
			break;

		case SDL_QUIT:
			smQueue->push(SMMessage::QUIT);
			return true;
		}
	}

	return false;
}