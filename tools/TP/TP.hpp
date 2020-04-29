#include <functional>
#include <sstream>

namespace TP {
    void prepare_pool(uint32_t number_threads = 0);
    void add_job(std::function<void()> job);
    void join_pool();
    const std::stringstream& message_stream();
}