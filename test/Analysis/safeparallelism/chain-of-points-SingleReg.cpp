// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker %s -verify
// expected-no-diagnostics

class [[asap::param("class")]] point  {
    double m_x [[asap::arg("class")]];
    double m_y [[asap::arg("class")]];

public:
    point() {}
    point(double x, double y) : m_x(x), m_y(y) {}
 
    [[asap::param("Q"), asap::reads("Q")]] point(
        const point &p [[asap::arg("Q")]]
        )
        : m_x(p.m_x), m_y(p.m_y)
        {}

    point &operator= [[asap::arg("class"), asap::param("P"), asap::reads("P"), asap::writes("class")]] (
        const point &p [[asap::arg("P")]]
        )
        {
        m_x = p.m_x;
        m_y = p.m_y;
        return *this;
        }

    void set [[asap::writes("class")]] (double x_in, double y_in)
        {
        m_x = x_in;
        m_y = y_in;
        }

    double x [[asap::reads("class")]] ()  const {return m_x;}
    double y [[asap::reads("class")]] () const {return m_y;}
    };

// helper class for chain. (Could be moved inside chain.)
struct [[asap::param("P")]] link {
    point pos [[asap::arg("P")]];
    link *next [[asap::arg("P, P")]];

    [[asap::param("Q"), asap::reads("Q")]]
    link(const point &pos_in[[asap::arg("Q")]])
        :
        pos(pos_in),
        next(nullptr) {}
    };

void delete_all[[asap::param("Q"), asap::writes("Q")]]
    (link *lnk[[asap::arg("Q")]])
    {
    if(lnk)
        {
        delete_all(lnk->next);
        delete lnk;
        }
    }
 
class [[asap::param("P")]] chain
    {
    link *start [[asap::arg("P, P")]];
public:
    chain() : start(nullptr) {}
    [[asap::writes("P")]] ~chain() { delete_all(start); }
   
    // Add a link to the start of the chain
    void add_link [[asap::writes("P")]] (const point &pos[[asap::arg("P")]])
        {
        link *new_start[[asap::arg("P, P")]] = new link(pos);
        new_start->next = start;
        start = new_start;
        }
    // count and return the number of nodes in the list
    int n_points [[asap::reads("P")]]() const
        {
        link *lp[[asap::arg("Local, P")]] = start;
        int count[[asap::arg("Local")]] = 0;
        while(lp)
            {
            //++count;
            count = count + 1;
            lp = lp->next;
            }
 
        return count;
        }
 
    // Split the chain at link n (>= 1) and move the later links into 'rest'.
    // If there are fewer than n links in the chain return false, and do
    // not change either chain.
    bool split [[asap::writes("P")]] (int n, chain &rest[[asap::arg("P")]]) {
        link *lp[[asap::arg("Local, P")]] = start;
        for(int i = 1; lp && i < n; ++i)
            {
            lp = lp->next;
            }
 
        if(lp)
            {
            //delete any links that were in 'rest'
            delete_all(rest.start);
 
            rest.start = lp->next;
            lp->next = nullptr;
            }
 
        return lp != nullptr;
        }
    };
