#pragma once
#include "IObserver.h"
#include <vector>
#include <memory>

struct Subject
{
	Subject() {}
	~Subject() {}
	void attach(IObserver* observer)
	{
		vecObservers.emplace_back(observer);
	}
	void dettach(IObserver* observer)
	{
		vecObservers.erase(std::remove(vecObservers.begin(), vecObservers.end(), observer), vecObservers.end());
	}

	std::vector<IObserver*> vecObservers;
};

//play the sound effect when cued for
class AudioCueSubject
{
public:
	AudioCueSubject()
	{
		subject = std::make_unique<Subject>();
	}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify()
	{
		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

private:
	std::unique_ptr<Subject> subject;
};

//----------------------------------------------
//MOUSE SUBJECTS
//left mouse button subjects and observers
class LBtnPressedSubject
{
public:
	LBtnPressedSubject() :
		iMouseX(0),
		iMouseY(0)
	{
		subject = std::make_unique<Subject>();
	}

	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify(const int iMouseX, const int iMouseY)
	{
		this->iMouseX = iMouseX;
		this->iMouseY = iMouseY;
		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

	int getMouseX() { return iMouseX; }
	int getMouseY() { return iMouseY; }

private:
	std::unique_ptr<Subject> subject;
	int iMouseX, iMouseY;
};

class LBtnMotionSubject
{
public:
	LBtnMotionSubject() :
		iRelX(0),
		iRelY(0),
		iMouseX(0),
		iMouseY(0)
	{
		subject = std::make_unique<Subject>();
	}
	~LBtnMotionSubject()
	{}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify(const int iMouseX, const int iMouseY, const int iRelX, const int iRelY)
	{
		this->iMouseX = iMouseX;
		this->iMouseY = iMouseY;
		this->iRelX = iRelX;
		this->iRelY = iRelY;

		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

	int getMouseX() { return iMouseX; }
	int getMouseY() { return iMouseY; }
	int getRelX() { return iRelX; }
	int getRelY() { return iRelY; }

private:
	std::unique_ptr<Subject> subject;
	int iRelX, iRelY;
	int iMouseX, iMouseY;
};

class LBtnReleasedSubject
{
public:
	LBtnReleasedSubject() 
	{
		subject = std::make_unique<Subject>();
	}
	~LBtnReleasedSubject()
	{}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify()
	{
		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

private:
	std::unique_ptr<Subject> subject;
};


//right button down and then mouse moved for SDL_MOUSEMOTION event 
class RBtnMotionSubject
{
public:
	RBtnMotionSubject() :
		iRelX(0),
		iRelY(0),
		iMouseX(0),
		iMouseY(0)
	{
		subject = std::make_unique<Subject>();
	}
	~RBtnMotionSubject()
	{}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify(const int iMouseX, const int iMouseY, const int iRelX, const int iRelY)
	{
		this->iMouseX = iMouseX;
		this->iMouseY = iMouseY;
		this->iRelX = iRelX;
		this->iRelY = iRelY;

		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

	int getMouseX() { return iMouseX; }
	int getMouseY() { return iMouseY; }
	int getRelX() { return iRelX; }
	int getRelY() { return iRelY; }

private:
	std::unique_ptr<Subject> subject;
	int iRelX, iRelY;
	int iMouseX, iMouseY;
};


// middle mouse button subjects and observers
class MBtnMotionSubject
{
public:
	MBtnMotionSubject() :
		iRelX(0),
		iRelY(0),
		iMouseX(0),
		iMouseY(0)
	{
		subject = std::make_unique<Subject>();
	}
	~MBtnMotionSubject()
	{}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify(const int iMouseX, const int iMouseY, const int iRelX, const int iRelY)
	{
		this->iMouseX = iMouseX;
		this->iMouseY = iMouseY;
		this->iRelX = iRelX;
		this->iRelY = iRelY;

		for (auto observer : subject->vecObservers)
			observer->onNotify();
	}

	int getMouseX() { return iMouseX; }
	int getMouseY() { return iMouseY; }
	int getRelX() { return iRelX; }
	int getRelY() { return iRelY; }

private:
	std::unique_ptr<Subject> subject;
	int iRelX, iRelY;
	int iMouseX, iMouseY;
};


class MouseScrollSubject
{
public:
	MouseScrollSubject()
	{
		subject = std::make_unique<Subject>();
	}
	~MouseScrollSubject() {}
	void attach(IObserver* observer)
	{
		subject->attach(observer);
	}
	void dettach(IObserver* observer)
	{
		subject->dettach(observer);
	}
	void notify(const int iScrollX, const int iScrollY)
	{
		this->iScrollX = iScrollX;
		this->iScrollY = iScrollY;
		for (auto o : subject->vecObservers)
			o->onNotify();
	}

	int getScrollX() const { return iScrollX; }
	int getScrollY() const { return iScrollY; }

private:
	std::unique_ptr<Subject> subject;
	int iScrollX, iScrollY;
};