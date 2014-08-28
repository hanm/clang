// RUN: %clang_cc1 -std=c++11 -analyze -analyzer-checker=alpha.SafeParallelismChecker -analyzer-config -asap-default-scheme=inference %s -verify
//

class //[[asap::param("class")]] 
      point  {
    double m_x; // expected-warning{{Inferred region arguments: double, IN:[r0_m_x], ArgV:}}
    double m_y; // expected-warning{{Inferred region arguments: double, IN:[r1_m_y], ArgV:}}

public:
    point() {}
    point(double x, double y) : m_x(x), m_y(y) {} //expected-warning{{Inferred Effect Summary for point: [reads(rpl([rLOCAL],[]))]}}
 
    [[asap::param("Q")]]//, asap::reads("Q")]] 
    point(const point &p [[asap::arg("Q")]]
        )
        : m_x(p.m_x), m_y(p.m_y)
      {} //expected-warning{{Inferred Effect Summary for point: [reads(rpl([r0_m_x],[])),reads(rpl([r1_m_y],[]))]}}

    point &operator= //[[asap::arg("class"), asap::param("P"), asap::reads("P"), asap::writes("class")]]  // expected-warning{{Inferred region arguments: class point &(const class point &), IN:<empty>, ArgV:[p0_point]}}
                     [[asap::param("P")]] 
                     (const point &p [[asap::arg("P")]])
        { // expected-warning{{Inferred Effect Summary for operator=: [reads(rpl([r0_m_x],[])),reads(rpl([r1_m_y],[])),writes(rpl([r0_m_x],[])),writes(rpl([r1_m_y],[]))]}}
        m_x = p.m_x;
        m_y = p.m_y;
        return *this;
        }

    void set /*[[asap::writes("class")]]*/ (double x_in, double y_in)
      { //expected-warning{{Inferred Effect Summary for set: [reads(rpl([rLOCAL],[])),writes(rpl([r0_m_x],[])),writes(rpl([r1_m_y],[]))]}}
        m_x = x_in;
        m_y = y_in;
        }

    double x /*[[asap::reads("class")]]*/ ()  const { return m_x; } // expected-warning{{Inferred Effect Summary for x: [reads(rpl([r0_m_x],[]))]}}
    double y /*[[asap::reads("class")]]*/ () const { return m_y; }  // expected-warning{{Inferred Effect Summary for y: [reads(rpl([r1_m_y],[]))]}}
    };

// helper class for chain. (Could be moved inside chain.)
struct //[[asap::param("Pl")]] 
       link {
    point pos;  // expected-warning{{Inferred region arguments: class point, IN:<empty>, ArgV:[r3_pos]}}
    link *next; // expected-warning{{Inferred region arguments: struct link *, IN:[r4_next], ArgV:[rSTAR]}}

    //[[asap::param("Q"), asap::reads("Q")]]
    link(const point &pos_in /*[[asap::arg("Pl")]]*/) // expected-warning{{Inferred region arguments: const class point &, IN:<empty>, ArgV:[rSTAR]}}
        :
        pos(pos_in),
        next(nullptr) {}
    // copy constructor
    link(link &l /*[[asap::arg("Pl")]]*/) : pos(l.pos), next(l.next) {} //expected-warning{{Inferred Effect Summary for link: [reads(rpl([r4_next],[]))]}} // expected-warning{{Inferred region arguments: struct link &, IN:<empty>, ArgV:[r7_l]}}
    // move "constructor"
    link &operator = /*[[asap::arg("Pl")]]*/ (link && l/*[[asap::arg("Pl")]]*/) { return l; }  // expected-warning{{Inferred region arguments: struct link &(struct link &&), IN:<empty>, ArgV:[rSTAR]}}  // expected-warning{{Infered region arguments: struct link &&, IN:<empty>, ArgV:[r9_l]}}
    };

void delete_all[[asap::param("Q")]]//, asap::writes("Q")]]
    (link *lnk[[asap::arg("Q")]])
{ //expected-warning{{Inferred Effect Summary for delete_all: [reads(rpl([r4_next],[])),reads(rpl([rLOCAL],[]))]}}
    if(lnk)
        {
        delete_all(lnk->next);
        delete lnk;
        }
    }
 
class //[[asap::param("P")]] 
    chain {
    link *start;  // expected-warning{{Inferred region arguments: struct link *, IN:[r10_start], ArgV:[rSTAR]}}
public:
    chain() : start(nullptr) {}
    /*[[asap::writes("P")]]*/ ~chain() { delete_all(start); } //expected-warning{{Inferred Effect Summary for ~chain: [reads(rpl([r10_start],[])),reads(rpl([r4_next],[])),reads(rpl([rLOCAL],[]))]}}
   
    // Add a link to the start of the chain
    void add_link /*[[asap::writes("P")]]*/ (const point &pos /*[[asap::arg("P")]]*/)  // expected-warning{{Inferred region arguments: const class point &, IN:<empty>, ArgV:[r12_pos]}}
        {//expected-warning{{Inferred Effect Summary for add_link: [reads(rpl([r10_start],[])),reads(rpl([r13_new_start],[])),writes(rpl([r10_start],[])),writes(rpl([r4_next],[]))]}}
        link *new_start /*[[asap::arg("P, P")]]*/ = new link(pos);  // expected-warning{{Inferred region arguments: struct link *, IN:[r13_new_start], ArgV:[r14_new_start]}}
        new_start->next = start;
        start = new_start;
        }
    // count and return the number of nodes in the list
    int n_points /*[[asap::reads("P")]]*/ () const
        { // expected-warning{{Inferred Effect Summary for n_points: [reads(rpl([r10_start],[])),reads(rpl([r15_lp],[])),reads(rpl([r4_next],[])),writes(rpl([r15_lp],[])),writes(rpl([rLOCAL],[]))]}}
        link *lp /*[[asap::arg("Local, P")]]*/ = start; // expected-warning{{Inferred region arguments: struct link *, IN:[r15_lp], ArgV:[rSTAR]}}
        int count /*[[asap::arg("Local")]]*/ = 0;
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
    bool split /*[[asap::writes("P")]]*/ (int n, chain &rest /*[[asap::arg("P")]]*/) {//expected-warning{{Inferred Effect Summary for split: [writes(rpl([r10_start],[])),writes(rpl([r18_lp],[])),writes(rpl([r4_next],[])),writes(rpl([rLOCAL],[]))]}}  // expected-warning{{Inferred region arguments: class chain &, IN:<empty>, ArgV:[r17_rest]}}
        link *lp/*[[asap::arg("Local, P")]]*/ = start;  // expected-warning{{Inferred region arguments: struct link *, IN:[r18_lp], ArgV:[rSTAR]}}
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
