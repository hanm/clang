namespace tbb {
    template<typename Index, typename Func>
    Func parallel_for( Index first, Index last, const Func& f);

    template<typename Index, typename Func>
    Func parallel_for( Index first, Index last, 
                       Index step, const Func& f );

    template<typename Range, typename Body> 
    void parallel_for( const Range& range, const Body& body );

} // end namespace

