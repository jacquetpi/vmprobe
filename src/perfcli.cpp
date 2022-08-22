#include "perfcli.hpp"
#include <sstream>
#include <fstream>
#include "utils/log.hpp"
#include "utils/config.hpp"
#include "error.hpp"
#include <limits>

namespace server {

    PerfClient::PerfClient(){
        _numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        utils::logging::info(_numCPU, "cpu(s) found");
    }

    void PerfClient::perfInit () {
        if (utils::Config::Get().countersPerCore == 0)
            for(int i=0;i<_numCPU;i++){
                for(const auto& event : utils::Config::Get().perfEventHardware){
                    _fdCounters[event].push_back(fdStart(i, PERF_TYPE_HARDWARE, perfHwId.find(event)->second));
                }
                for(const auto& event : utils::Config::Get().perfEventHardwareCache){
                    _fdCounters[event].push_back(fdStart(i, PERF_TYPE_HW_CACHE, perfHwCacheId.find(event)->second));
                }
                for(const auto& event : utils::Config::Get().perfEventSoftware){
                    _fdCounters[event].push_back(fdStart(i, PERF_TYPE_SOFTWARE, perfSwId.find(event)->second));
                }
            }
        else{
            int nbEvents = utils::Config::Get().perfEventHardware.size() + utils::Config::Get().perfEventHardwareCache.size() + utils::Config::Get().perfEventSoftware.size();
            utils::logging::info(nbEvents, " events à repartir");
        }
        utils::logging::success("Perf counters initalized");
    }

    void PerfClient::perfEnable () {
        for (auto& it: _fdCounters) {
            for(int i=0;i<_numCPU;i++){
                ioctl(it.second.at(i), PERF_EVENT_IOC_ENABLE, 0);
            }
        }
    }

    void PerfClient::perfReset () {
        for (auto& it: _fdCounters) {
            for(int i=0;i<_numCPU;i++){
                ioctl(it.second.at(i), PERF_EVENT_IOC_RESET, 0);
            }
        }
    } 

    void PerfClient::perfReadCounters(Dump* dump){
        for (auto& it: _fdCounters) {
            std::string key = it.first;
            long long value;
            for(int i=0;i<_numCPU;i++){
                value+= fdRead(it.second.at(i));
            }
            key.erase(0,11); // remove PERF_COUNT_
            key.erase(remove(key.begin(), key.end(), '_'), key.end());
            dump->addGlobalMetric("perf_" + to_lower(key), value);
        }
    }

    void PerfClient::perfClose () {
        for (auto& it: _fdCounters) {
            for(int i=0;i<_numCPU;i++){
                fdClose(it.second.at(i));
            }
        }
        utils::logging::info("Perf counters closed");
    }

    int PerfClient::fdStart (int cpu, perf_type_id type, int event) {
        struct perf_event_attr pe;
        int fd;

        memset(&pe, 0, sizeof(pe));
        pe.type = type;
        pe.size = sizeof(pe);
        pe.config = event;
        pe.disabled = 1;
		pe.exclude_user = 0;
		pe.exclude_kernel = 0;
		pe.exclude_hv = 0;
		pe.exclude_idle = 0;

        fd = perfEventOpen(&pe, -1, cpu, -1, PERF_FLAG_FD_CLOEXEC);
        if (fd == -1) {
            utils::logging::error ("PerfClient::fdStart Failed to initialize a counter, eventtype", type, "eventcode", event, "on core", cpu);
            throw ProbeError("PerfClient::fdStart failed\n");
        }

        return fd;
    }

    const long long PerfClient::fdRead(int fd){
        long long count;
        read(fd, &count, sizeof(count));
        return count;
    }

    void PerfClient::fdClose (int fd) {
        ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
        close(fd);
    }

    long PerfClient::perfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags){
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
            std::ifstream f0 ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
            std::stringstream buffer;
            buffer << f0.rdbuf();
            buffer >> _maxFreqCPU;
            f0.close();
            buffer.str("");
            buffer.clear();
            std::ifstream f1 ("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
            buffer << f1.rdbuf();
            buffer >> _minFreqCPU;
            f1.close();
	    }

        long long sum = 0;
        unsigned int freq;
	    for (int i = 0 ; i < this-> _numCPU; i++) {
            std::ifstream f (this-> _cpuPath[i]);
            std::stringstream buffer;
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

    const int PerfClient::getVCPUs() {
        return _numCPU;
    }

    const int PerfClient::getMaxFreq() {
        return _maxFreqCPU;
    }

    const int PerfClient::getMinFreq() {
        return _minFreqCPU;
    }

}
