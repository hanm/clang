namespace tbb {
    template<typename Index, typename Func>
    Func parallel_for( Index first, Index last, const Func& f);

    template<typename Index, typename Func>
    Func parallel_for( Index first, Index last, 
                       Index step, const Func& f );

    template<typename Range, typename Body> 
    void parallel_for( const Range& range, const Body& body );

    class task_scheduler_init {
    public:
        void init(int);
        static int default_num_threads();
        }; // end class task_scheduler_init
    } // end namespace

