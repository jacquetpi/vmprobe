#pragma once

#include <pthread.h>

namespace utils {

	class mutex {

	    pthread_mutex_t _m;

	    
	public:
	    
	    mutex ();

	    void lock ();

	    void unlock ();

	    ~mutex ();
	};
}
