#include "perfcli.hpp"
#include <sstream>
#include <fstream>
#include "utils/log.hpp"
#include "error.hpp"
#include <limits>

namespace server {

    PerfClient::PerfClient(){
        _numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        _fdsInstructions = new int[_numCPU];
        _fdsCycles = new int[_numCPU];
        utils::logging::info(_numCPU, "cpu(s) found");
    }

    void PerfClient::perf_init () {
        for(int i=0;i<_numCPU;i++){
            _fdsInstructions[i] = fd_start(i, PERF_COUNT_HW_INSTRUCTIONS);
            _fdsCycles[i] = fd_start(i, PERF_COUNT_HW_CPU_CYCLES);
        }
        utils::logging::success("Perf counters initalized");
    }

    void PerfClient::perf_enable () {
        for(int i=0;i<_numCPU;i++){
            ioctl(_fdsInstructions[i], PERF_EVENT_IOC_ENABLE, 0);
            ioctl(_fdsCycles[i] , PERF_EVENT_IOC_ENABLE, 0);
        }
    }

    void PerfClient::perf_reset () {
        for(int i=0;i<_numCPU;i++){
            ioctl(_fdsInstructions[i], PERF_EVENT_IOC_RESET, 0);
            ioctl(_fdsCycles[i] , PERF_EVENT_IOC_RESET, 0);
        }
    } 

    void PerfClient::perf_read_cpi (long long* instructions, long long* cycles){
        *instructions = 0;
        *cycles = 0;
        for(int i=0;i<_numCPU;i++){
            *instructions += fd_read(_fdsInstructions[i]);
            *cycles += fd_read(_fdsCycles[i]);
        }
    }

    void PerfClient::perf_close () {
        utils::logging::info("Closing perf counters");
        for(int i=0;i<_numCPU;i++){
            fd_close(_fdsInstructions[i]);
            fd_close(_fdsCycles[i]);
        }
    }

    int PerfClient::fd_start (int cpu, perf_hw_id event_type) {
        struct perf_event_attr pe;
        int fd;

        memset(&pe, 0, sizeof(pe));
        pe.type = PERF_TYPE_HARDWARE;
        pe.size = sizeof(pe);
        pe.config = event_type;
        pe.disabled = 1;
		pe.exclude_user = 0,
		pe.exclude_kernel = 0,
		pe.exclude_hv = 0,
		pe.exclude_idle = 0;

        fd = perf_event_open(&pe, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC);
        if (fd == -1) {
            utils::logging::error ("PerfClient::fd_start Failed to initialize a counter :", event_type);
            throw ProbeError("PerfClient::fd_start failed\n");
        }

        return fd;
    }

    const long long PerfClient::fd_read(int fd){
        long long count;
        read(fd, &count, sizeof(count));
        return count;
    }

    void PerfClient::fd_close (int fd) {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        close(fd);
    }

    long PerfClient::perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags){
        int ret;
        ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
        return ret;
    }

    const long long PerfClient::readCPUFrequency () {
	    if (this-> _cpuPath.size () != (size_t)this-> _numCPU ) {
            for (int i = 0 ; i < this-> _numCPU; i++) {
                std::stringstream path;
                path << "/sys/devices/system/cpu/cpu" << i << "/cpufreq/scaling_cur_freq";
                this-> _cpuPath.push_back (path.str ());
            }
	    }

        long long sum = 0;
        unsigned int freq;
	    for (int i = 0 ; i < this-> _numCPU; i++) {
            std::ifstream f (this-> _cpuPath[i]);
            std::stringstream buffer;
            std::string res;
            buffer << f.rdbuf();
            buffer >> freq;
            f.close();
            sum+=freq;
	    }

	    return sum/(this-> _numCPU);
	}

    const void PerfClient::addHostMemoryUsage(Dump* dump){
        // We don't use sysinfo as memAvailable is not directly exposed
        unsigned long memTotal, memAvailable, memFree, buffers, cached;
        std::ifstream infile("/proc/meminfo");

        infile.ignore(18, ' '); 
        infile >> memTotal;
        infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        infile.ignore(18, ' '); 
        infile >> memFree;
        infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        infile.ignore(18, ' '); 
        infile >> memAvailable;
        infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        infile.ignore(18, ' ');
        infile >> buffers;
        infile.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        infile.ignore(18, ' '); 
        infile >> cached;

        infile.close();

        dump->addGlobalMetric("memory_total", memTotal);
        dump->addGlobalMetric("memory_free", memFree);
        dump->addGlobalMetric("memory_buffers", buffers);
        dump->addGlobalMetric("memory_cached", cached);
        dump->addGlobalMetric("memory_available", memAvailable);
    }

    const int PerfClient::getVCPUs () {
        return _numCPU;
    }

}
