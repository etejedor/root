/*
 * TExtraeInstrumenter.cxx
 *
 *  Created on: May 13, 2015
 *      Author: etejedor
 */


#include "TExtraeInstrumenter.h"

#include "extrae_user_events.h"

#include <string.h>

std::atomic<unsigned int> TExtraeInstrumenter::curr_thid = {0};


/*void TExtraeInstrumenter::Init() {
	Extrae_set_threadid_function(TExtraeInstrumenter::GetThreadID);
	Extrae_set_numthreads_function(TExtraeInstrumenter::GetNumThreads);

	Extrae_init();

	// Create string tags for event types and values
	unsigned nvalues;
	extrae_value_t values[MAX_EVENT_NVAL]    = { END_GENERIC, START_GENERIC };
	const char *descr_values[MAX_EVENT_NVAL] = { "End" };
	extrae_type_t types[EVENT_NTYPES]        = { PBP_TASK, CACHE_READ, UNZIP, DESERIALIZE, LOCK };
	const char *descriptions[EVENT_NTYPES]   = { "PBP Task", "Cache Read", "Unzip", "Deserialize", "Lock" };
	for (int i = 0; i < EVENT_NTYPES; i++) {
		if (strcmp(descriptions[i], "Lock") == 0) {
			nvalues = MAX_EVENT_NVAL;
			values[1] = ACQUIRE;
			values[2] = RELEASE;
			descr_values[1] = "Acquire";
			descr_values[2] = "Release";
		}
		else {
			nvalues = MIN_EVENT_NVAL;
			descr_values[1] = descriptions[i];
		}
		Extrae_define_event_type(&types[i], (char*)descriptions[i], &nvalues, values, (char**)descr_values);
	}
}*/

void TExtraeInstrumenter::Init() {
		Extrae_set_threadid_function(TExtraeInstrumenter::GetThreadID);
		Extrae_set_numthreads_function(TExtraeInstrumenter::GetNumThreads);  

		Extrae_init();

		unsigned int ROOT_EVENT = PBP_TASK;

		// Create string tags for event types and values
		unsigned nvalues = 7;
		extrae_value_t UNLOCK = LOCK+1;
		extrae_value_t values[7]    = { 0, PBP_TASK, CACHE_READ, UNZIP, DESERIALIZE, LOCK, UNLOCK };
		const char *descr_values[7] = { "End", "PBP Task", "Cache Read", "Unzip", "Deserialize", "Lock", "Unlock" };
		extrae_type_t type = ROOT_EVENT;
		const char *description = "ROOT";
		for (int i = 0; i < EVENT_NTYPES; i++) {
			Extrae_define_event_type(&type, (char*)description, &nvalues, values, (char**)descr_values);
		}
}


void TExtraeInstrumenter::End() {
	Extrae_fini();
}

void TExtraeInstrumenter::Event(int type, int value) {
	Extrae_event(type, value);
}
