#pragma once

class IObserver
{
public:
	IObserver() {}
	virtual ~IObserver() {}
	virtual void onNotify() = 0;
};
