#include "libvirtcli.hpp"
#include "perfcli.hpp"

namespace server {
    
    /**
     * The daemon class is the main class of the monitor
     * It manage the libvirt connections
     */
    class Daemon {

		private:

			// The libvirt connection
			LibvirtClient* _libvirt;

			// The perf interface
			PerfClient* _perfcli;

			Dump* _dump;

			// Fetching delay
			int _delay;

			void retrievePerfMetrics();

			void retrieveLibvirtMetrics();
		
		public: 
		
			Daemon();

			/**
			 * Start the different part of the daemon
			 */
			void start ();

			/**
			 * Force the killing of the daemon
			 */
			void kill ();
	
    };
}
