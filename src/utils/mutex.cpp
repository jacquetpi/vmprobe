#include "mutex.hpp"

namespace utils {

	mutex::mutex () : _m (PTHREAD_MUTEX_INITIALIZER)
	{}

	void mutex::lock () {
	    pthread_mutex_lock (&this-> _m);
	}

	void mutex::unlock () {
	    pthread_mutex_unlock (&this-> _m);
	}

	mutex::~mutex () {
	    this-> unlock ();
	}

}
