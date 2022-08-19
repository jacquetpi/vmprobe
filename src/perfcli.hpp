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
#include <unordered_map>
#include <bits/stdc++.h>

namespace server {
	/**
	 * The perf client is used to retreive perf interface metrics
	 */

	static std::unordered_map<std::string, perf_hw_id> const perfHwId = {
		{"PERF_COUNT_HW_CPU_CYCLES", perf_hw_id::PERF_COUNT_HW_CPU_CYCLES},
		{"PERF_COUNT_HW_INSTRUCTIONS", perf_hw_id::PERF_COUNT_HW_INSTRUCTIONS},
		{"PERF_COUNT_HW_CACHE_REFERENCES", perf_hw_id::PERF_COUNT_HW_CACHE_REFERENCES},
		{"PERF_COUNT_HW_CACHE_MISSES", perf_hw_id::PERF_COUNT_HW_CACHE_MISSES},
		{"PERF_COUNT_HW_BRANCH_INSTRUCTIONS", perf_hw_id::PERF_COUNT_HW_BRANCH_INSTRUCTIONS},
		{"PERF_COUNT_HW_BRANCH_MISSES", perf_hw_id::PERF_COUNT_HW_BRANCH_MISSES},
		{"PERF_COUNT_HW_BUS_CYCLES", perf_hw_id::PERF_COUNT_HW_BUS_CYCLES},
		{"PERF_COUNT_HW_STALLED_CYCLES_FRONTEND", perf_hw_id::PERF_COUNT_HW_STALLED_CYCLES_FRONTEND},
		{"PERF_COUNT_HW_STALLED_CYCLES_BACKEND", perf_hw_id::PERF_COUNT_HW_STALLED_CYCLES_BACKEND},
		{"PERF_COUNT_HW_REF_CPU_CYCLES", perf_hw_id::PERF_COUNT_HW_REF_CPU_CYCLES}
	};

	static std::unordered_map<std::string, perf_hw_cache_id> const perfHwCacheId = {
		{"PERF_COUNT_HW_CACHE_L1D", perf_hw_cache_id::PERF_COUNT_HW_CACHE_L1D},
		{"PERF_COUNT_HW_CACHE_L1I", perf_hw_cache_id::PERF_COUNT_HW_CACHE_L1I},
		{"PERF_COUNT_HW_CACHE_LL", perf_hw_cache_id::PERF_COUNT_HW_CACHE_LL},
		{"PERF_COUNT_HW_CACHE_DTLB", perf_hw_cache_id::PERF_COUNT_HW_CACHE_DTLB},
		{"PERF_COUNT_HW_CACHE_ITLB", perf_hw_cache_id::PERF_COUNT_HW_CACHE_ITLB},
		{"PERF_COUNT_HW_CACHE_BPU", perf_hw_cache_id::PERF_COUNT_HW_CACHE_BPU},
		{"PERF_COUNT_HW_CACHE_NODE", perf_hw_cache_id::PERF_COUNT_HW_CACHE_NODE}
	};

	static std::unordered_map<std::string, perf_sw_ids> const perfSwId = {
		{"PERF_COUNT_SW_CPU_CLOCK", perf_sw_ids::PERF_COUNT_SW_CPU_CLOCK},
		{"PERF_COUNT_SW_TASK_CLOCK", perf_sw_ids::PERF_COUNT_SW_TASK_CLOCK},
		{"PERF_COUNT_SW_PAGE_FAULTS", perf_sw_ids::PERF_COUNT_SW_PAGE_FAULTS},
		{"PERF_COUNT_SW_CONTEXT_SWITCHES", perf_sw_ids::PERF_COUNT_SW_CONTEXT_SWITCHES},
		{"PERF_COUNT_SW_CPU_MIGRATIONS", perf_sw_ids::PERF_COUNT_SW_CPU_MIGRATIONS},
		{"PERF_COUNT_SW_PAGE_FAULTS_MIN", perf_sw_ids::PERF_COUNT_SW_PAGE_FAULTS_MIN},
		{"PERF_COUNT_SW_PAGE_FAULTS_MAJ", perf_sw_ids::PERF_COUNT_SW_PAGE_FAULTS_MAJ},
		{"PERF_COUNT_SW_ALIGNMENT_FAULTS", perf_sw_ids::PERF_COUNT_SW_ALIGNMENT_FAULTS},
		{"PERF_COUNT_SW_EMULATION_FAULTS", perf_sw_ids::PERF_COUNT_SW_EMULATION_FAULTS},
		{"PERF_COUNT_SW_DUMMY", perf_sw_ids::PERF_COUNT_SW_DUMMY},
		{"PERF_COUNT_SW_BPF_OUTPUT", perf_sw_ids::PERF_COUNT_SW_BPF_OUTPUT}
	};

    class PerfClient {

		private:
		
        int _numCPU;
		int _minFreqCPU;
		int _maxFreqCPU;
		
		std::unordered_map<std::string ,std::vector<int> > _fdCounters;

	    std::vector <std::string> _cpuPath;

		int fdStart(int cpu, perf_type_id type, int event);

		const long long fdRead(int fd);

		void fdClose(int fd);

		long perfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);

		public: 
		
		PerfClient ();

		/**
		 * Start the different part of the daemon
		 */
		void perfInit();

		void perfEnable();

		void perfReset();

		void perfReadCounters(Dump* dump);

		void perfClose();

		const long long readCPUFrequency();

		const void addHostMemoryUsage(Dump* dump);

		const int getVCPUs();

		const int getMinFreq();

		const int getMaxFreq();
    };

	static std::string to_lower(std::string s) {        
		for(char &c : s)
			c = tolower(c);
		return s;
	}

}
