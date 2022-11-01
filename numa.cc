#include <tbb/enumerable_thread_specific.h>
#include <hwloc.h>
#include <vector>
#include <thread>
#include <iostream>
#include <sstream>

using sout_t = tbb::enumerable_thread_specific<std::stringstream>;
void alloc_thread_per_node(hwloc_topology_t topo, sout_t &sout)
{
    int numa_nodes = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NUMANODE);
    std::vector<std::thread> threads;
    for (int i = 0; i < numa_nodes; i++){
        threads.push_back(std::thread([i, numa_nodes, &topo, &sout](){
            int err;
            sout.local() << "Master thread " << i << " out of " << numa_nodes << "\n";
            hwloc_obj_t numa_node = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NUMANODE, i);
            err = hwloc_set_cpubind(topo, numa_node->cpuset, HWLOC_CPUBIND_THREAD);
            assert(!err);
            sout.local() << "After: Thread: " << i << " with tid " << std::this_thread::get_id() << " on core " << sched_getcpu() << "\n"; 
        }));
    }
    for (auto &thread : threads)
        thread.join();
}

void alloc_mem_per_node(hwloc_topology_t topo, double **data, long size){
    int numa_nodes = hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NUMANODE);
    for(int i=0; i<numa_nodes; i++){
        hwloc_obj_t numa_node = hwloc_get_obj_by_type(topo, HWLOC_OBJ_NUMANODE, i);
        char *s;
        hwloc_bitmap_asprintf(&s, numa_node->cpuset);
        std::cout << "NUMA node " << i << " has cpu bitmask: "<< s << std::endl;
        free(s);
        data[i] = (double *) hwloc_alloc_membind(topo,
                                                 size*sizeof(double),
                                                 numa_node->nodeset,
                                                 HWLOC_MEMBIND_BIND,
                                                 HWLOC_MEMBIND_BYNODESET);
    }
}

int main(){
    hwloc_topology_t topo;
    hwloc_topology_init(&topo);
    hwloc_topology_load(topo);

    int numa_nodes = hwloc_get_nbobjs_by_type(topo,
                                              HWLOC_OBJ_NUMANODE);
    std::cout << "Numa Node(s): " << numa_nodes;

    double** data = new double*[numa_nodes];
    long size = 1024 * 1024;
    alloc_mem_per_node(topo, data, size);

    sout_t sout;
    // one master thread per NUMA node
    alloc_thread_per_node(topo, sout);

    for(auto &s:sout){
        std::cout << s.str() << std::endl;
    }
    // free the allocated data and topology
    for(int i=0; i<numa_nodes; i++){
        hwloc_free(topo, data[i], size);
    }
    hwloc_topology_destroy(topo);
    delete[] data;
    return 0;
}