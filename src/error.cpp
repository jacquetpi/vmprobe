#include "error.hpp"

namespace server {

	ProbeError::ProbeError (const std::string & msg) :
	    utils::exception (msg)
	{}

}
