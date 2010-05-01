/*
Copyright 2010 CowboyCoders. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are
permitted provided that the following conditions are met:

   1. Redistributions of source code must retain the above copyright notice, this list of
      conditions and the following disclaimer.

   2. Redistributions in binary form must reproduce the above copyright notice, this list
      of conditions and the following disclaimer in the documentation and/or other materials
      provided with the distribution.

THIS SOFTWARE IS PROVIDED BY COWBOYCODERS ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COWBOYCODERS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those of the
authors and should not be interpreted as representing official policies, either expressed
or implied, of CowboyCoders.
*/

#ifndef ___libcow_dispatcher___
#define ___libcow_dispatcher___

#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/utility.hpp>

namespace libcow {

   /**
    * The dispatcher class offers a thread safe way to run jobs (function
    * objects) in a "worker" thread. The worker thread is created
    * by the dispatcher and is accessed by posting "jobs" to a queue.
    * Since the jobs are not run in parallell, it's safe to share resources
    * between them.
    * This class is a wrapper around the boost::asio::io_service class.
    */
    class LIBCOW_EXPORT dispatcher : public boost::noncopyable
    {
    public:
       /**
        * Creates a new dispatcher and spawns an internal worker thread. When
        * created, the dispatcher immediately starts processing any incoming
        * jobs (use the dispatcher::post method to put a job on the queue).
        * @param timer_delay The delay in milliseconds to use for the deadline timer.
        */
        dispatcher(int timer_delay);

       /**
        * Stops processing jobs and then destructs the dispatcher. Note that
        * the dispatcher will wait until any currently running job has completed
        * before destructing.
        */
        // TODO: Maybe we shouldn't wait here, or at least add completion timeout.
        ~dispatcher();

       /**
        * Adds an argument-less function object to the job queue.
        * It's safe to call this function from multiple threads.
        * @param handler A function object, perhaps created using boost::bind.
        */
        template<typename CompletionHandler>
        void post(const CompletionHandler& handler)
        {
            io_service.post(handler);
        }

       /**
        * Adds an argument-less function object to the job queue.
        * It's safe to call this function from multiple threads.
        * The (asynchronous) invocation of the function object will be 
        * delayed by the specified number of milliseconds.
        * @param handler A function object, perhaps created using boost::bind.
        */
        template<typename CompletionHandler>
        void post_delayed(const CompletionHandler& handler)
        {
            deadline_timer.async_wait(handler);
        }

     private:

        // don't reorder these!
        boost::asio::io_service io_service;
        boost::asio::io_service::work work;
        boost::thread thread;
        boost::asio::deadline_timer deadline_timer;
    };
}
#endif // ___libcow_dispatcher___
