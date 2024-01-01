#ifndef CANDYBOX_TWEEN_HPP__
#define CANDYBOX_TWEEN_HPP__

#include <vector>
#include <cstdint>
#include "candybox/Easing.hpp"

// NOLINTBEGIN(modernize-use-nodiscard)
namespace candybox {

class Timeline;

class ITimelineEvent
{
public:
	ITimelineEvent(double start, double duration, uint32_t repeat)
	    : mRepeatCount(repeat), mTriggerTime(start), mDuration(duration)
	{
	}
	virtual ~ITimelineEvent() = default;

	double GetTriggerTime() const { return mTriggerTime; }
	double GetDuration() const { return mDuration; }

	bool IsInfiniteLoop() const { return mRepeatCount == kInfiniteLoop; }
	// Return true if the event is within its time bound [mTriggerTime, mnTriggerTime + mDuration],
	// and the event is either in infinite loop or its repeated count is lesser than mRepeatCount.
	// Can be override.
	virtual bool IsActive(double currentTime) const;
	virtual void Execute(double currentTime) = 0; // pure virtual.

protected:
	friend class Timeline;

	static const uint32_t kInfiniteLoop = -1;

	uint32_t mRepeated{0};
	uint32_t mRepeatCount; // The event will at least repeat this count.
	double mTriggerTime;
	// The repetition of event with duration set to zero depend on repeat count.
	double mDuration;
	bool mActive{true};
};

class TimelineEvent_f : public ITimelineEvent
{
public:
	// time - in seconds.
	typedef void (*EventCallback)(void *uptr, const TimelineEvent_f &event, double time);

	TimelineEvent_f(
	    double start,
	    double duration,
	    EventCallback callback,
	    uint32_t repeat = kInfiniteLoop)
	    : ITimelineEvent(start, duration, repeat), mCallback(callback)
	{
	}

	void SetUserPointer(void *ptr) { mUserPointer = ptr; }
	void Execute(double currentTime) override;

private:
	EventCallback mCallback;
	void *mUserPointer{nullptr};
};

class TimelineEvent_v : public ITimelineEvent
{
public:
	TimelineEvent_v(
	    double start,
	    double duration,
	    double startValue,
	    double endValue,
	    easing::Type type = easing::Type::kLinear,
	    uint32_t repeat = kInfiniteLoop)
	    : ITimelineEvent(start, duration, repeat),
	      mStartValue(startValue),
	      mEndValue(endValue),
	      mValue(startValue),
	      mEasingType(type)
	{
	}

	double GetValue() const { return mValue; }
	void SetValue(double v) { mValue = v; }
	double GetStartValue() const { return mStartValue; }
	void SetStartValue(double v) { mStartValue = v; }
	double GetEndValue() const { return mEndValue; }
	void SetEndValue(double v) { mEndValue = v; }

	void Execute(double currentTime) override;

private:
	double mStartValue, mEndValue;
	double mValue;
	easing::Type mEasingType{};
};

// Events should be "new"ed and add to the timeline.
// Events will be automatically destroyed  when its lifespan has passed.
class Timeline
{
public:
	explicit Timeline(double startUpTime) : mStartUpTime(startUpTime) { }

	void Update(); // Pass in time get with glfwGetTime.
	void AddEvent(ITimelineEvent *event);
	void AddEvent(
	    double start,
	    double duration,
	    TimelineEvent_f::EventCallback callback,
	    uint32_t repeat = ITimelineEvent::kInfiniteLoop);
	double GetTime() const { return mGetTimeCb(); };
	void SetGetTimeCallback(double (*cb)()) { mGetTimeCb = cb; };

private:
	// Event with smaller "mTriggerTime" will deemed with high priority.
	std::vector<ITimelineEvent *> mEvents;
	double mStartUpTime;
	double mPrevUpdateTime{-1.f};
	double (*mGetTimeCb)(){nullptr};
};

} // namespace candybox
// NOLINTEND(modernize-use-nodiscard)

#endif // CANDYBOX_TWEEN_HPP__
