#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>
#include <vector>
#include "dump.hpp"

namespace server {
    
	/**
	 * The perf client is used to retreive perf interface metrics
	 */
    class PerfClient {

		private:
		
        int _numCPU;
		int _minFreqCPU;
		int _maxFreqCPU;
		int *_fdsInstructions; //file descriptors for instructions
		int *_fdsCycles; //file descriptors for instructions

	    std::vector <std::string> _cpuPath;

		int fd_start (int cpu, perf_hw_id event_type);

		const long long fd_read (int fd);

		void fd_close (int fd);

		long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);

		public: 
		
		PerfClient ();

		/**
		 * Start the different part of the daemon
		 */
		void perf_init();

		void perf_enable();

		void perf_reset();

		void perf_read_cpi (long long* instructions, long long* cycles);

		void perf_close ();

		const long long readCPUFrequency();

		const void addHostMemoryUsage(Dump* dump);

		const int getVCPUs();

		const int getMinFreq();

		const int getMaxFreq();
    };

}
