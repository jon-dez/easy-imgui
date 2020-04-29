#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include "TP.hpp"

namespace TP {
    namespace {
        static std::stringstream msg_str;

        static bool keep_alive = true;
        static std::vector<std::thread> thread_pool;
        static std::mutex job_q_mutex;
        static std::condition_variable job_avail;
        static std::queue<std::function<void()>> jobs;

        void worker_thread(int id){
            msg_str << "Worker thread " << id << " has started." << std::endl;

            int num_jobs = 0;
            while(keep_alive){
                std::function<void()> job;
                /* Let the unique lock be destroyed after this code block. */{
                    std::unique_lock<std::mutex> lock(job_q_mutex);
                    job_avail.wait(lock, [](){ return jobs.size() > 0; });
                    job = jobs.front();
                    jobs.pop();
                }
                job();
                num_jobs++;
            }

            msg_str << "Worker thread has completed " << num_jobs << " job(s)." << std::endl;
        }
    }

    /**
     * Use the default argument, zero, so that the number of threads match the CPU.
     */
    void prepare_pool(uint32_t num_threads){
        if(num_threads == 0)
            num_threads = std::thread::hardware_concurrency();
        msg_str << "Creating a thread pool of size " << num_threads << std::endl;

        thread_pool.resize(num_threads); // Default construct the threads.

        // Start all the threads. They will start in their default idle state if no jobs are available.
        for(int i=0; i < thread_pool.size(); i++){
            thread_pool[i] = std::thread(
                worker_thread,
                i
            );
        }
    }

    void add_job(std::function<void()> job){
        {
            std::lock_guard<std::mutex> lock(job_q_mutex);
            jobs.push(job);
        }
        job_avail.notify_one();
    }

    void join_pool(){
        {
            std::lock_guard<std::mutex> lock(job_q_mutex);
            keep_alive = false;
        }
        job_avail.notify_all();
        for(auto& worker: thread_pool)
            worker.join();
        
        msg_str << "Thread pool has joined." << std::endl;
    }

    const std::stringstream& message_stream(){
        return msg_str;
    }
}