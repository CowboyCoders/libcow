#ifndef ___dispatcher_h___
#define ___dispatcher_h___

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/utility.hpp>

/**
 * The dispatcher class offers a thread safe way to run jobs (function
 * objects) in a "worker" thread. The worker thread is created
 * by the dispatcher and is accessed by posting "jobs" to a queue.
 * Since the jobs are not run in parallell, it's safe to share resources
 * between them.
 * This class is a wrapper around the boost::asio::io_service class.
 */
class dispatcher : public boost::noncopyable
{
public:
    /** 
     * Creates a new dispatcher and spawns an internal worker thread. When 
     * created, the dispatcher immediately starts processing any incoming
     * jobs (use the dispatcher::post method to put a job on the queue).
     */
    dispatcher();

    /**
     * Stops processing jobs and then destructs the dispatcher. Note that
     * the dispatcher will wait until any currently running job has completed
     * before destructing.
     * TODO: Maybe we shouldn't wait here, or at least add completion timeout.
     */
    ~dispatcher();

    /**
     * Adds an argument-less function object to the job queue. 
     * It's safe to call this function from multiple threads.
     * See dispatcher_example for usage examples. 
     * @param handler A function object, perhaps created using boost::bind.
     */
    template<typename CompletionHandler>
    void post(const CompletionHandler& handler)
    {
        io_service.post(handler);
    }

private:

    // don't reorder these!
    boost::asio::io_service io_service;
    boost::asio::io_service::work work;
    boost::thread thread;
};

#endif // ___dispatcher_h___
