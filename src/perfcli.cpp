#include "perfcli.hpp"
#include <sstream>
#include <fstream>
#include <fts.h>
#include "utils/log.hpp"
#include "utils/config.hpp"
#include "error.hpp"
#include <limits>
#include <sys/resource.h>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <algorithm>

// Use cgroup v1 as v2 doesn't support perf_event yet
#define DEFAULT_CGROUP_VM_BASEPATH "/sys/fs/cgroup/perf_event/machine.slice/"

namespace server {

    PerfClient::PerfClient(){
        _numCPU = sysconf(_SC_NPROCESSORS_ONLN);
        utils::logging::info(_numCPU, "cpu(s) found");
        rlimit rl;
	    getrlimit(RLIMIT_NOFILE, &rl);
        utils::logging::info("Soft/Hard nofile limit are", rl.rlim_cur, "/", rl.rlim_max);
        if(rl.rlim_cur < rl.rlim_max){
            rl.rlim_cur = rl.rlim_max;
            if(setrlimit(RLIMIT_NOFILE, &rl) == 0)
                utils::logging::info("New nofile limit is", rl.rlim_cur);
            else
                utils::logging::error("Error: nofile limit couldn't be set:", strerror(errno));
        }
    }

    void PerfClient::perfInit() {
        perfSetCounters(&_fdGlobalCounters, -1, 0); // -1 for system wide counters and no specific flags
        perfRefreshVMs();
        utils::logging::success("Perf counters initalized");
    }

    void PerfClient::perfSetCounters(std::unordered_map<std::string ,std::vector<int> >* fdMap, int pid, int flag) {
        for(int i=0;i<_numCPU;i++){
            for(const auto& event : utils::Config::Get().perfEventHardware){
                (*fdMap)[event].push_back(fdStart(pid, i, flag, PERF_TYPE_HARDWARE, perfHwId.find(event)->second));
            }
            for(const auto& event : utils::Config::Get().perfEventHardwareCache){
                (*fdMap)[event].push_back(fdStart(pid, i, flag, PERF_TYPE_HW_CACHE, perfHwCacheId.find(event)->second));
            }
            for(const auto& event : utils::Config::Get().perfEventSoftware){
                (*fdMap)[event].push_back(fdStart(pid, i, flag, PERF_TYPE_SOFTWARE, perfSwId.find(event)->second));
            }
            for(const auto& event : utils::Config::Get().perfEventTracepoint){
                (*fdMap)[event].push_back(fdStart(pid, i, flag, PERF_TYPE_TRACEPOINT, std::stoi(event)));
            }
        }
    }

    void PerfClient::perfRefreshVMs () {
        std::unordered_map<std::string, std::string> cgroups = retrieveCgroupsVM();
        std::list<std::string> toBeDeleted;
        for(auto x : cgroups)
            if (_fdVMCounters.find(x.first) == _fdVMCounters.end()){ // New key
                utils::logging::info("New VM detected", x.first, "with cgroup", x.second);
                perfInitVM(x.first, x.second);
            }
        // Check if a VM disappeared
        for(auto x : _fdVMCounters)
            if (cgroups.find(x.first) == cgroups.end()){
                perfCloseSpecific(&x.second);
                toBeDeleted.push_back(x.first);
            }
        for(auto x : toBeDeleted){
            _fdVMCounters.erase(x);
            close(std::get<0>(_fdVmCgroup[x]));
            _fdVmCgroup.erase(x);
            utils::logging::info("VM", x, "is no longer active, counters cleared");
        }
    }

    void PerfClient::perfInitVM(std::string vmname, std::string vmCgroupPath) {
        int perf_flags = 0;
        perf_flags |= PERF_FLAG_PID_CGROUP;
        errno = 0;
        int cgroup_fd = open(vmCgroupPath.c_str(), O_RDONLY); 
        if (cgroup_fd < 1) {
            utils::logging::error("cannot open cgroup dir path=", vmCgroupPath, "for vm", vmname, "errno=", errno);
            return;
        }
        perfSetCounters(&_fdVMCounters[vmname], cgroup_fd, perf_flags);
        _fdVmCgroup[vmname] = std::make_tuple(cgroup_fd, vmCgroupPath); // Keep track of fd (to properly close them) and procfs
    }

    std::unordered_map<std::string, std::string>  PerfClient::retrieveCgroupsVM () {
        std::unordered_map<std::string, std::string> vmCgroups;
        const char *path[] = { DEFAULT_CGROUP_VM_BASEPATH, NULL };
        FTS *file_system = NULL;
        FTSENT *node = NULL;
        file_system = fts_open((char * const *)path, FTS_LOGICAL | FTS_NOCHDIR, NULL);
        if (!file_system){
            utils::logging::error("Error while(*fdMap) opening", DEFAULT_CGROUP_VM_BASEPATH);
            return vmCgroups;
        }
        for (node = fts_read(file_system); node; node = fts_read(file_system)) {
            /*
            * Filtering the directoriesf having 2 hard links leading to them to only get leaves directories.
            * The cgroup subsystems does not support hard links, so this will always work.
            */
            if (node->fts_info == FTS_D && node->fts_statp->st_nlink == 2) {
                std::string vmname(node->fts_path);
                size_t found_first = vmname.find("\\x2d");
                // format is /sys/fs/cgroup/perf_event/machine.slice/machine-qemu\x2d{id}\x2d{name which may contains \x2d}.scope
                if (found_first != std::string::npos) {
                    std::string vmname_start_at_id = vmname.substr(found_first+4);
                    size_t found_second = vmname_start_at_id.find("\\x2d");
                    if (found_second != std::string::npos) {
                        std::string vmname_start_at_name = vmname_start_at_id.substr(found_second+4);
                        size_t found_scope = vmname_start_at_name.find_last_of('.');
                        if (found_scope != std::string::npos) // remove .scope
                            vmname_start_at_name.erase(found_scope);
                        findAndReplaceAll(vmname_start_at_name, "\\x2d", "");
                        vmCgroups[vmname_start_at_name] = node->fts_path;
                    }
                } // else : not a VM, probably a podman container
            }
        }
        return vmCgroups;
    }

    void PerfClient::perfEnable () {
        perfEnableSpecific(&_fdGlobalCounters);
        for(auto& x : _fdVMCounters)
            perfEnableSpecific(&x.second);
    }

    void PerfClient::perfEnableSpecific(std::unordered_map<std::string ,std::vector<int> >* fdMap) {
        for (auto& it: (*fdMap))
            for(int i=0;i<_numCPU;i++)
                ioctl(it.second.at(i), PERF_EVENT_IOC_ENABLE, 0);
    }

    void PerfClient::perfReset() {
        perfResetSpecific(&_fdGlobalCounters);
        for(auto& x : _fdVMCounters)
            perfResetSpecific(&x.second);
    }

    void PerfClient::perfResetSpecific (std::unordered_map<std::string ,std::vector<int> >* fdMap) {
        for (auto& it: (*fdMap))
            for(int i=0;i<_numCPU;i++)
                ioctl(it.second.at(i), PERF_EVENT_IOC_RESET, 0);
    } 

    void PerfClient::perfRead(Dump* dump){
        perfRefreshVMs(); 
        perfReadSpecific("", &_fdGlobalCounters, dump);
        for(auto& x : _fdVMCounters)
            perfReadSpecific(x.first, &x.second, dump);
    }

    void PerfClient::perfReadSpecific(std::string qualifier, std::unordered_map<std::string ,std::vector<int> >* fdMap, Dump* dump){
        for (auto& it: (*fdMap)) {
            std::string key = it.first;
            long long value = 0;
            for(int i=0;i<_numCPU;i++){
                value+= fdRead(it.second.at(i));
            }
            if(!is_number(key)){
                key.erase(0,11); // remove PERF_COUNT_
                key.erase(remove(key.begin(), key.end(), '_'), key.end());
            }
            if(qualifier.empty())
                dump->addGlobalMetric("perf_" + to_lower(key), value);
            else
                dump->addSpecificMetric(qualifier, "perf_" + to_lower(key), value);
        }
    }

    void PerfClient::perfClose() {
        perfResetSpecific(&_fdGlobalCounters);
        for(auto& x : _fdVMCounters){
            perfResetSpecific(&x.second);
            close(std::get<0>(_fdVmCgroup[x.first]));
            _fdVmCgroup.erase(x.first);
        }
        utils::logging::info("Perf counters closed");
    }

    void PerfClient::perfCloseSpecific(std::unordered_map<std::string ,std::vector<int> >* fdMap) {
        for (auto& it: (*fdMap))
            for(int i=0;i<_numCPU;i++)
                fdClose(it.second.at(i));
    }

    int PerfClient::fdStart(int pid, int cpu, int perf_flags, perf_type_id type, int event) {
        struct perf_event_attr pe;
        int fd;
        perf_flags |= PERF_FLAG_FD_CLOEXEC;
        memset(&pe, 0, sizeof(pe));
        pe.type = type;
        pe.size = sizeof(pe);
        pe.config = event;
        pe.disabled = 1;
	    pe.exclude_user = 0;
		pe.exclude_kernel = 0;
		pe.exclude_hv = 0;
		pe.exclude_idle = 0;

        try
        {
            fd = perfEventOpen(&pe, pid, cpu, -1, perf_flags);
        }
        catch (std::exception& e)
        {
            std::cerr << "Exception caught : " << e.what() << std::endl;
        }
        
        if (fd == -1) {
            utils::logging::error ("PerfClient::fdStart Failed to initialize a counter, eventtype", type, "eventcode", event, "on core", cpu);
            utils::logging::error ("Errno", strerror(errno));
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

    void PerfClient::readSchedStat(Dump* dump){
        readNodeSchedStat(dump);
        readVmSchedStat(dump);
    }

    void PerfClient::readVmSchedStat(Dump* dump){
        for(auto x : _fdVMCounters)
            readVmStatSpecific(dump, x.first, std::get<1>(_fdVmCgroup[x.first]));
    }

    void PerfClient::readNodeSchedStat(Dump* dump){
        std::ifstream schedstat ("/proc/schedstat");
        std::string schedstatline;
        unsigned long long runtime = 0;
        unsigned long long waittime = 0;
        while (std::getline(schedstat, schedstatline))
            if (schedstatline.rfind("cpu", 0) == 0) // filter lines
                readSchedStatLine(schedstatline, &runtime, &waittime);
        dump->addGlobalMetric("sched_runtime", runtime);
        dump->addGlobalMetric("sched_waittime", waittime);
    }

    void PerfClient::readVmStatSpecific(Dump* dump, std::string vmname, std::string vmCgroupFs){
        std::ifstream cgroupfile (vmCgroupFs + "/cgroup.procs");
        std::string strpid;
        // Metrics to be retrieved
        unsigned long long runtime = 0;
        unsigned long long waittime = 0;
        unsigned long minflt = 0;
        unsigned long cminflt = 0;
        unsigned long majflt = 0;
        unsigned long cmajflt = 0;
        unsigned long vsize= 0;
        unsigned long rss = 0;
        unsigned long rsslim = 0;
        std::string schedstatline;
        std::string statline;
        // Iterate through pids of a given VM and sum its poi
        while (std::getline(cgroupfile, strpid)){
            std::ifstream stat("/proc/" + strpid + "/stat");
            if(std::getline(stat, statline)){
                readStatLine(statline, &minflt, &cminflt, &majflt, &cmajflt, &vsize, &rss, &rsslim);
            }
            stat.close();
            std::ifstream schedstat("/proc/" + strpid + "/schedstat");
            if(std::getline(schedstat, schedstatline)){
                readSchedStatLine(schedstatline, &runtime, &waittime);
            }
            schedstat.close();
        }
        cgroupfile.close();
        dump->addSpecificMetric(vmname, "stat_minflt", minflt);
        dump->addSpecificMetric(vmname, "stat_cminflt", cminflt);
        dump->addSpecificMetric(vmname, "stat_majflt", majflt);
        dump->addSpecificMetric(vmname, "stat_cmajflt", cmajflt);
        dump->addSpecificMetric(vmname, "stat_vsize", vsize); // in bytes
        dump->addSpecificMetric(vmname, "stat_rss", rss); // in pages
        dump->addSpecificMetric(vmname, "stat_rsslim", rsslim); // in bytes
        dump->addSpecificMetric(vmname, "sched_runtime", runtime);
        dump->addSpecificMetric(vmname, "sched_waittime", waittime);
    }

    void PerfClient::readSchedStatLine(std::string schedstatline, unsigned long long* runtime, unsigned long long* waittime){
        size_t size;
        std::vector<std::string> datasched = readLine(schedstatline, &size);
        if(size<3){
            utils::logging::warn("Unexpected schedstat format encountered : ", schedstatline);
            return;
        }
        //format is "[...] <timerun> <timewait> <timslicesrun>"
        *runtime+=std::stoul(datasched[size-2].c_str());
        *waittime+=std::stoul(datasched[size-1].c_str());
    }

    void PerfClient::readStatLine(std::string stat, unsigned long* minflt, unsigned long* cminflt, unsigned long* majflt, 
                                unsigned long* cmajflt, unsigned long* vsize, unsigned long* rss, unsigned long* rsslim){
        size_t size;
        std::vector<std::string> datastat = readLine(stat, &size);
        if(size<20){
            utils::logging::warn("Unexpected schedstat format encountered : ", stat);
            return;
        }
        //From /proc/[pid]/stat : https://man7.org/linux/man-pages/man5/proc.5.html
        *minflt+= std::stoul(datastat[10-1].c_str());
        *cminflt+= std::stoul(datastat[11-1].c_str());
        *majflt+= std::stoul(datastat[12-1].c_str());
        *cmajflt+= std::stoul(datastat[13-1].c_str());
        *vsize+= std::stoul(datastat[23-1].c_str());
        *rss+= std::stoul(datastat[24-1].c_str()); // reported as inaccurate
        *rsslim+= std::stoul(datastat[25-1].c_str());
    }

    // Return a procfs file in an array format
    std::vector<std::string> PerfClient::readLine(std::string statline, size_t* size){
        *size = std::count(statline.begin(), statline.end(), ' ')+1;
        std::vector<std::string> arr(*size);
        int i = 0;
        std::stringstream ssin(statline);
        while (ssin.good() && i < *size){
            ssin >> arr[i];
            ++i;
        }
        return arr;
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
