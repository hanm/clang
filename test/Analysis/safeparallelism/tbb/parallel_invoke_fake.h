namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke[[asap::param("P1, P2")]]
                        (const Func0& f0[[asap::arg("P1")]], const Func1& f1[[asap::arg("P2")]]) {
        f0();
        f1();
        }
    class task_scheduler_init {
    public:
        void init(int);
        static int default_num_threads();
        }; // end class task_scheduler_init
    } // end namespace

