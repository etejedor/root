/*
 * TExtraeInstrumenter.h
 *
 *  Created on: May 13, 2015
 *      Author: etejedor
 */

#ifndef ROOT_TEXTRAEINSTRUMENTER
#define ROOT_TEXTRAEINSTRUMENTER

// Event IDs
#define PBP_TASK     8000000
#define CACHE_READ   8000001
#define UNZIP        8000002
#define DESERIALIZE  8000003
#define LOCK         8000004

// Event values
#define END_GENERIC   0
#define START_GENERIC 1
#define ACQUIRE       1 // LOCK
#define RELEASE       2 // LOCK

#define EVENT_NTYPES   5
#define MAX_EVENT_NVAL 3
#define MIN_EVENT_NVAL 2

#define NUM_THREADS 4 

#include <atomic>



class TExtraeInstrumenter {

private:
	static std::atomic<unsigned int> curr_thid;

public:
	static void Init();
	static void End();
	static void Event(int,int);

	static unsigned int GetThreadID()
	{
		thread_local unsigned int thid = curr_thid.fetch_add(1);
		return thid;
	}

	static unsigned int GetNumThreads()
	{
		return NUM_THREADS;
	}
};


//#define EXTRAE

#ifdef EXTRAE
#define R__EXTRAE_INIT() TExtraeInstrumenter::Init()
#define R__EXTRAE_END()  TExtraeInstrumenter::End()
#define R__EXTRAE_EVENT(event_id, event_val) TExtraeInstrumenter::Event(event_id, event_val)
/*#define R__EXTRAE_EVENT(event_id, event_val) \
if (event_val == 0)         TExtraeInstrumenter::Event(PBP_TASK, 0); \
else if (event_id != LOCK)  TExtraeInstrumenter::Event(PBP_TASK, event_id); \
else                        TExtraeInstrumenter::Event(PBP_TASK, event_id+event_val-1);*/
#else
#define R__EXTRAE_INIT() { }
#define R__EXTRAE_END()  { }
#define R__EXTRAE_EVENT(event_id, event_val) { }
#endif


#endif
