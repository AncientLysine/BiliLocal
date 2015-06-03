#pragma once

#include <QElapsedTimer>

class ElapsedTimer
{
public:
	ElapsedTimer()
	{
		estimate = qSNaN();
		interval = overflow = 0;
	}

	void setInterval(double second)
	{
		interval = second;
	}

	void invalidate()
	{
		inner.invalidate();
	}

	bool swap()
	{
		overflow = inner.isValid() ? qMax(0.0, elapsed() - estimate) : 0.0;
		inner.start();
		estimate = qSNaN();
		return overflow > 0;
	}

	double step()
	{
		if (inner.isValid() && qIsNaN(estimate)){
			estimate = qMax(interval, elapsed());
			return estimate + overflow;
		}
		else{
			return 0;
		}
	}

private:
	double interval;
	double estimate;
	double overflow;
	QElapsedTimer inner;

	double elapsed()
	{
		return inner.nsecsElapsed() / 1000000000.0;
	}
};
