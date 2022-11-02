# vmprobe

Expose in a file readable by a vanilla prometheus node_exporter the specified performance counters for the current node and all its KVM VMs. Also add data exposed by libvirt and by some procfs (such as sched_wait_time)

## Requirements

- QEMU/KVM with standard libvirt packages
- libvirt-devel :
    - On CentOS :  
        ```bash
        sudo dnf --enablerepo=crb install -y libvirt-devel
        ```
    - On Ubuntu :
        ```bash
        sudo apt install -y libvirt-dev 
        ```
- g++ cmake make
- perf

## Configuration

Configuration should be written in config.yaml file with the following data:

- prefix : all metrics will be prefixed by this string
- delay : in ms, the duration between two "read session"
- endpoint : the file where metrics will be written
- url : qemu url (should be local as perf counters cannot be read remotely)
- perfhardware : hardware counters to be registered (*)
- perfsoftware : software counters to be registered (*)
- perfhardwarecache : hardwarecache counters to be registered
- perftracepoint : tracepoint counters to be registered (*)

(*) : Will expose counters for each VM AND the host (reset after each "read session", you only get values corresponding to specified delta)

## Miscellaneous

- cgroup v1 only for now (todo : cgroup v2)
- domain name must be unique
- be careful with high number of counters and VM as we may open a lot of file descriptors on each core (todo : multiplexing)
- output format is for now
    ```bash
    [prefix]_[global|domain]_[{if domain : domain_name}]_[probe|cpu|memory|perf|sched]_[metric]
    ```
    - type of metrics:
        - probe : probe data (configured metrics, last "read session" epoch)
        - cpu : cpu stats
        - memory : memory stats
        - perf : perf stats (for a list of supported events, please read below)
        - sched : scheduler stats

## Supported perf event

### perfhardware

```bash
    PERF_COUNT_HW_CPU_CYCLES
    PERF_COUNT_HW_INSTRUCTIONS
    PERF_COUNT_HW_CACHE_REFERENCES
    PERF_COUNT_HW_CACHE_MISSES
    PERF_COUNT_HW_BRANCH_INSTRUCTIONS
    PERF_COUNT_HW_BRANCH_MISSES
    PERF_COUNT_HW_BUS_CYCLES
    PERF_COUNT_HW_STALLED_CYCLES_FRONTEND
    PERF_COUNT_HW_STALLED_CYCLES_BACKEND
    PERF_COUNT_HW_REF_CPU_CYCLES
```

### perfsoftware

```bash
    PERF_COUNT_HW_CACHE_L1D
    PERF_COUNT_HW_CACHE_L1I
    PERF_COUNT_HW_CACHE_LL
    PERF_COUNT_HW_CACHE_DTLB
    PERF_COUNT_HW_CACHE_ITLB
    PERF_COUNT_HW_CACHE_BPU
    PERF_COUNT_HW_CACHE_NODE
```

### perfhardwarecache

```bash
    PERF_COUNT_SW_CPU_CLOCK
    PERF_COUNT_SW_TASK_CLOCK
    PERF_COUNT_SW_PAGE_FAULTS
    PERF_COUNT_SW_CONTEXT_SWITCHES
    PERF_COUNT_SW_CPU_MIGRATIONS
    PERF_COUNT_SW_PAGE_FAULTS_MIN
    PERF_COUNT_SW_PAGE_FAULTS_MAJ
    PERF_COUNT_SW_ALIGNMENT_FAULTS
    PERF_COUNT_SW_EMULATION_FAULTS
    PERF_COUNT_SW_DUMMY
    PERF_COUNT_SW_BPF_OUTPUT
```

### perftracepoint

A list of tracepoint ids can be specified but they are kernel dependents. You can list them for your configuration with:
```bash
(root) grep '' /sys/kernel/debug/tracing/events/*/*/id
```

## Quickstart

```bash
./compil.sh
sudo ./vmprobe
```