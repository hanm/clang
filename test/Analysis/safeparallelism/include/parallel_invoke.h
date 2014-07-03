namespace tbb {
    template<typename Func0, typename Func1>
    void parallel_invoke(const Func0 &F0, const Func1 &F1);

    template<typename Func0, typename Func1, typename Func2>
    void parallel_invoke(const Func0 &F0, const Func1 &F1, const Func2 &F2);

    template<typename Func0, typename Func1, typename Func2, typename Func3>
    void parallel_invoke(const Func0 &F0, const Func1 &F1, const Func2 &F2, const Func3 &F3);

} // end namespace

