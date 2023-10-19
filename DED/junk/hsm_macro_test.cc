
#include <string>
#include <list>
#include <vector>
#include <stdarg.h>
#include <stdio.h>

#define STATE_LIST                                              \
    STATE_DEFN(not_present)                                     \
    STATE_DEFN(present,                                         \
               STATE_DEFN(not_mounted,                          \
                          STATE_DEFN(scanning)                  \
                          STATE_DEFN(bad)                       \
                          STATE_DEFN(formatting)                \
                          STATE_DEFN(remove_safe))              \
               STATE_DEFN(mounted,                              \
                          STATE_DEFN(available)                 \
                          STATE_DEFN(online,                    \
                                     STATE_DEFN(in_service))))


#define STATE_DEFN(name,list...)  STATE_##name, list
enum state_t { STATE_TOP, STATE_LIST STATE_NUM_STATES };
#undef STATE_DEFN

#define STATE_DEFN(name, list...)   #name, list
const std::string state_names[] = { "TOP", STATE_LIST "STATE_NUM_STATES" };
#undef STATE_DEFN

typedef bool (*/*classname::*/transition_function_t)(struct event *);

bool /*classname::*/top(struct event *evt) { return true; }

#define STATE_DEFN(name, list...) \
    bool /*classname::*/name(struct event *evt) { return true; } list
STATE_LIST ;
#undef STATE_DEFN

struct state_info;
typedef std::list<state_info*> state_list;
struct state_vector : public std::vector<state_info*> {
    void set_parents(void);
    static state_info * add_state(
        state_t id, transition_function_t func, /*child state list */...);
    void init(void) {
        resize(STATE_NUM_STATES);
#define STATE_DEFN(name, list...) \
        at(STATE_##name) = add_state(STATE_##name,                      \
                                     &/*classname::*/name, list 0),
        at(0) = add_state(STATE_TOP, &/*classname::*/top, STATE_LIST 0);
#undef STATE_DEFN
        set_parents();
    }
};

struct state_info {
    state_info(state_t _id, transition_function_t _func) :
        id(_id), func(_func) { }
    state_t  id;
    transition_function_t func;
    state_list children;
    typedef state_list::iterator child_iterator;
    typedef state_list::const_iterator child_citerator;
    state_vector parents;
    void push_children(va_list ap) {
        while (1) {
            state_info * ns = va_arg(ap,state_info *);
            if (ns == NULL)
                break;
            children.push_back(ns);
        }
    }
    void set_parents(state_info * parent) {
        child_iterator it;
        for (it = children.begin(); it != children.end(); it++)
            (*it)->set_parents(parent);
        if (this != parent)
            parents.push_back(parent);
    }
    const void print(int level = 0) const {
        if (level >= 0)
            for (int count = 0; count < level; count++)
                printf("   ");
        printf("id: %2d, name: %-11s, parents: ",
               id, state_names[id].c_str());
        for (unsigned int count = 0; count < parents.size(); count++)
            printf("%-11s ", state_names[parents.at(count)->id].c_str());
        printf("\n");
        if (level >= 0)
            for (child_citerator it = children.begin();
                 it != children.end(); it++)
                (*it)->print(level+1);
    }
};

void state_vector::set_parents(void) {
    for (int count = size()-1; count >= 0; count--)
        at(count)->set_parents(at(count));
}

//static
state_info * state_vector::add_state(
    state_t id, transition_function_t func, /*child state list */...)
{
    state_info * ret = new state_info(id, func);
    va_list ap;
    va_start(ap,func);
    ret->push_children(ap);
    va_end(ap);
    return ret;
}

int main()
{
    state_vector states;
    states.init();
    printf("\ntree dump:\n");
    states.at(0)->print();
    printf("\nvector dump:\n");
    for (unsigned int count = 0; count < states.size(); count++)
        states.at(count)->print(-1);
    printf("\n");
    return 0;
}
