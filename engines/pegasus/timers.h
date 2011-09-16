/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * Additional copyright for this file:
 * Copyright (C) 1995-1997 Presto Studios, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
 
#ifndef PEGASUS_TIMERS_H
#define PEGASUS_TIMERS_H

#include "common/list.h"
#include "common/rational.h"

#include "pegasus/constants.h"

namespace Pegasus {

class Idler {
friend class PegasusEngine;

public:
	Idler();
	virtual ~Idler();
	
	virtual void startIdling();
	virtual void stopIdling();
	bool isIdling() const { return _isIdling; }
	
protected:
	virtual void useIdleTime() {}

	bool _isIdling;
};

enum {
	kLoopTimeBase           = 1,
	kPalindromeLoopTimeBase = 2,
	kMaintainTimeBaseZero   = 4
};

class TimeBaseCallBack;

class TimeBase {
friend class TimeBaseCallBack;
public:
	TimeBase(const TimeScale = kDefaultTimeScale);
	virtual ~TimeBase();
	
	virtual void setTime(const TimeValue, const TimeScale = 0);
	virtual TimeValue getTime(const TimeScale = 0);
	
	virtual void setScale(const TimeScale scale) { _preferredScale = scale; }
	virtual TimeScale getScale() const { return _preferredScale; }
	
	virtual void setRate(const Common::Rational);
	virtual Common::Rational getRate() const { return _rate; }
	
	virtual void start();
	virtual void stop();
	virtual bool isRunning();
	
	virtual void pause();
	virtual void resume();
	bool isPaused() const { return _paused; }
	
	virtual void setFlags(const uint32 flags) { _flags = flags; }
	virtual uint32 getFlags() const { return _flags; }
	
	virtual void setStart(const TimeValue, const TimeScale = 0);
	virtual TimeValue getStart(const TimeScale = 0) const;
	
	virtual void setStop(const TimeValue, const TimeScale = 0);
	virtual TimeValue getStop(const TimeScale = 0) const;
	
	virtual void setSegment(const TimeValue, const TimeValue, const TimeScale = 0);
	virtual void getSegment(TimeValue&, TimeValue&, const TimeScale = 0) const;
	
	virtual TimeValue getDuration(const TimeScale = 0) const;

	virtual void setMasterTimeBase(TimeBase *timeBase);
		
	void disposeAllCallBacks();

	// ScummVM's API additions (to replace the need for actual timers)
	void checkCallBacks();

protected:
	void addCallBack(TimeBaseCallBack *);
	void removeCallBack(TimeBaseCallBack *);

	TimeBase *_master;
	TimeScale _preferredScale;
	TimeBaseCallBack *_callBackList;
	Common::Rational _rate, _pausedRate;
	bool _paused;
	uint32 _startTime, _startScale;
	uint32 _stopTime, _stopScale;
	int32 _timeOffset;
	uint32 _flags;
	TimeValue _pausedTime;

private:
	Common::Rational getEffectiveRate() const;

	Common::List<TimeBase *> _slaves;
};

// Type passed to initCallBack()
enum CallBackType {
	kCallBackNone = 0,
	kCallBackAtTime = 1,
	kCallBackAtExtremes = 4
};

// Trigger passed to scheduleCallBack()
enum CallBackTrigger {
	kTriggerNone = 0,

	// AtTime flags
	kTriggerTimeFwd = 1,

	// AtExtremes flags
	kTriggerAtStart = 1,
	kTriggerAtStop = 2
};

class TimeBaseCallBack {
friend class TimeBase;

public:
	TimeBaseCallBack();
	virtual ~TimeBaseCallBack();

	void initCallBack(TimeBase *, CallBackType type);

	void releaseCallBack();

	void scheduleCallBack(CallBackTrigger trigger, uint32 param2, uint32 param3);
	void cancelCallBack();

protected:
	virtual void callBack() = 0;

	TimeBase *_timeBase;
	
	// Owned and operated by TimeBase;
	TimeBaseCallBack *_nextCallBack;

	// Our storage of the QuickTime timer crap
	CallBackType _type;
	CallBackTrigger _trigger;
	uint32 _param2, _param3;

private:
	void disposeCallBack();
};

class IdlerTimeBase : public Idler, public TimeBase {
public:
	IdlerTimeBase();
	virtual ~IdlerTimeBase() { stopIdling(); }

	TimeValue getLastTime() const { return _lastTime; }

protected:
	virtual void useIdleTime();
	virtual void timeChanged(const TimeValue) {}
	
	TimeValue _lastTime;

};

class Notification;

class NotificationCallBack : public TimeBaseCallBack {
public:
	NotificationCallBack();
	virtual ~NotificationCallBack() {}
	
	void setNotification(Notification *notifier) { _notifier = notifier; }

	void setCallBackFlag(const tNotificationFlags flag) { _callBackFlag = flag; }
	tNotificationFlags getCallBackFlag() const { return _callBackFlag; }
	
protected:
	void callBack();
	
	Notification *_notifier;
	tNotificationFlags _callBackFlag;
};

class DynamicElement : public TimeBase {
public:
	TimeValue percentSeconds(const uint32 percent) { return getScale() * percent / 100; }
};

} // End of namespace Pegasus

#endif
