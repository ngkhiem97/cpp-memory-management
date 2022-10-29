#include <iostream>
#include <vector>
#include <random>
#include <tbb/tick_count.h>
#include <tbb/tbb.h>
#include <tbb/tbb_allocator.h>
#include <tbb/cache_aligned_allocator.h>
#include <atomic>
#include <new>

using namespace std;

#ifdef __cpp_lib_hardware_interference_size
using std::hardware_destructive_interference_size; //<new>
#else
// 64 bytes on
// x86-64│L1_CACHE_BYTES│L1_CACHE_SHIFT│__cacheline_aligned│...
constexpr std::size_t hardware_destructive_interference_size = 64;
#endif
struct bin
{
    alignas(hardware_destructive_interference_size) atomic<int> count;
};

void no_cache_alignment(std::vector<uint8_t> image, int num_bins)
{
    std::cout << "Memory Allocation without Cache Alignment" << std::endl;
    // Initialize histogram
    std::vector<atomic<int>> hist(num_bins);
    // Serial execution
    tbb::tick_count t0 = tbb::tick_count::now();
    std::for_each(image.begin(), image.end(),
                  [&](uint8_t i)
                  { hist[i]++; });
    tbb::tick_count t1 = tbb::tick_count::now();
    double t_serial = (t1 - t0).seconds();
    // Parallel execution
    std::vector<atomic<int>> hist_p(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                 [&](const tbb::blocked_range<size_t> &r)
                 {
                     for (size_t i = r.begin(); i < r.end(); ++i)
                         hist_p[image[i]]++;
                 });
    t1 = tbb::tick_count::now();
    double t_parallel = (t1 - t0).seconds();
    std::cout << "Serial: "
              << t_serial
              << ", ";
    std::cout << "Parallel: " << t_parallel << ", ";
    std::cout << "Speed-up: " << t_serial / t_parallel << std::endl;
}

void with_cache_alignment(std::vector<uint8_t> image, int num_bins)
{
    std::cout << "Memory Allocation with Cache Alignment" << std::endl;
    // Initialize histogram
    std::vector<bin, tbb::cache_aligned_allocator<bin>> hist(num_bins);
    // Serial execution
    tbb::tick_count t0 = tbb::tick_count::now();
    std::for_each(image.begin(), image.end(),
                  [&](uint8_t i)
                  { (hist[i].count)++; });
    tbb::tick_count t1 = tbb::tick_count::now();
    double t_serial = (t1 - t0).seconds();
    // Parallel execution
    std::vector<bin, tbb::cache_aligned_allocator<bin>> hist_p(num_bins);
    t0 = tbb::tick_count::now();
    parallel_for(tbb::blocked_range<size_t>{0, image.size()},
                 [&](const tbb::blocked_range<size_t> &r)
                 {
                     for (size_t i = r.begin(); i < r.end(); ++i)
                         (hist_p[image[i]].count)++;
                 });
    t1 = tbb::tick_count::now();
    double t_parallel = (t1 - t0).seconds();
    std::cout << "Serial: "
              << t_serial
              << ", ";
    std::cout << "Parallel: " << t_parallel << ", ";
    std::cout << "Speed-up: " << t_serial / t_parallel << std::endl;
}

int main(int argc, char **argv)
{
    long int n = 1000000000;
    int threads = 4;
    constexpr int num_bins = 256;
    // Initialize random number generator
    std::random_device seed;
    // Random device seed
    std::mt19937 mte{seed()};
    // mersenne_twister_engine
    std::uniform_int_distribution<> uniform{0, num_bins};
    // Initialize image
    std::vector<uint8_t> image; // empty vector
    image.reserve(n);
    // image vector preallocated
    std::generate_n(std::back_inserter(image), n,
                    [&]
                    { return uniform(mte); });
    tbb::global_control gc(tbb::global_control::max_allowed_parallelism, threads);
    no_cache_alignment(image, num_bins);
    with_cache_alignment(image, num_bins);
    return 0;
}