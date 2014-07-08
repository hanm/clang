#ifndef __TBB_H__
#define __TBB_H__

#include "blocked_range.h"
#include "parallel_for.h"
#include "parallel_invoke.h"
namespace tbb {
    class task_scheduler_init {
    public:
        void init(int);
        static int default_num_threads();
        }; // end class task_scheduler_init
    typedef int spin_mutex;
} // end namespace

#endif 
