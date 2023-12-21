#include <vector>
#include <cmath>
#include "candybox/Tween.hpp"
#include <cassert>

using namespace ie;

static void
Insert(std::vector<ITimelineEvent *> &events, ITimelineEvent *event)
{
	double triggerTime = event->GetTriggerTime();
	for (auto it = events.begin(); it != events.end(); it++)
	{
		if (triggerTime > (*it)->GetTriggerTime())
		{
			events.insert(it, event);
			return;
		}
	}
	events.emplace_back(event);
}

template <typename T>
static void
Remove(std::vector<ITimelineEvent *> &events, T &iterator)
{
	delete *iterator;
	events.erase(iterator);
}

void
Timeline::AddEvent(ITimelineEvent *event)
{
	Insert(mEvents, event);
}

void
Timeline::AddEvent(
    double start,
    double duration,
    TimelineEvent_f::EventCallback callback,
    uint32_t repeat)
{
	auto *event = new TimelineEvent_f(start, duration, callback, repeat);
	Insert(mEvents, event);
}

void
Timeline::Update()
{
	double trueTime = GetTime() - mStartUpTime;
	for (auto it = mEvents.begin(); it != mEvents.end(); it++)
	{
		auto &event = *it;
		if (trueTime < event->GetTriggerTime()) break;

		// Lazy removal.
		if (!event->IsInfiniteLoop() && event->mRepeatCount <= event->mRepeated)
		{
			// Remove the event.
			Remove(mEvents, it);
			it--; // Recover.
			event = *it; // Correct.
		}

		if (event->mDuration == 0.f)
		{
			event->Execute(trueTime - event->GetTriggerTime());
			event->mRepeated++;
		}
		else if (event->IsActive(trueTime) || event->IsInfiniteLoop())
		{
			// Execute callback.
			uint32_t timesShouldExec = 1;
			if (!event->IsInfiniteLoop())
			{
				timesShouldExec =
				    (uint32_t)std::floor(
				        (trueTime - event->GetTriggerTime()) /
				        (event->GetDuration() / event->mRepeatCount)) -
				    event->mRepeated;
			}
			for (uint32_t i = 0; i < timesShouldExec; ++i)
			{
				event->Execute(trueTime - event->GetTriggerTime());
				trueTime = GetTime() - mStartUpTime;
			}
			event->mRepeated += timesShouldExec;
		}
		else
		{
			// Check if we miss up some execution.
			if (event->mRepeated < event->mRepeatCount)
			{
				for (uint32_t i = 0; i < event->mRepeatCount - event->mRepeated; ++i)
				{
					event->Execute(trueTime - event->GetTriggerTime());
					trueTime = GetTime() - mStartUpTime;
				}
				event->mRepeated = event->mRepeatCount;
			}

			// The event will never be active again.
			Remove(mEvents, it);
			it--; // Recover.
		}
	}

	mPrevUpdateTime = trueTime;
}

void
TimelineEvent_f::Execute(double currentTime)
{
	if (mCallback) mCallback(mUserPointer, *this, currentTime);
}

bool
ITimelineEvent::IsActive(double currentTime) const
{
	return mActive && currentTime <= mTriggerTime + mDuration && currentTime >= mTriggerTime &&
	       (mRepeatCount >= mRepeated || IsInfiniteLoop());
}

void
TimelineEvent_v::Execute(double currentTime)
{
	assert(mEasingType != easing::Type::kNums);
	easing::Func func = easing::GetMappingFunc(mEasingType);
	if (func)
	{
		mValue = func(currentTime, mStartValue, mEndValue - mStartValue, mDuration);
	}
}
