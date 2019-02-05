/*
 * Authors: Khalid Al-Hawaj
 */

/**
 * @file
 *
 *  A generic queue that holds limited amount of elements.
 *
 */

#ifndef __COMPONENTS_QUEUE_HH__
#define __COMPONENTS_QUEUE_HH__

/* gem5 includes */
#include <base/trace.hh>

/* Generic stdc includes */
#include <deque>

/* gem5 includes */
#include "mem/port.hh"

namespace Components
{

template<class T>
class Queue
{
  protected:
    /** Maximum number of elements that the queue can hold */
    const unsigned int maxElementsCount;

    /** Internal type for data structure to hold elements. */
    typedef std::deque<T> queue_t;

    /** Elements inside the queue */
    queue_t q;

  public:
    /** Constructor */
    Queue(std::string name_, unsigned int max_elements_count) :
        maxElementsCount(max_elements_count)
    {
        if (max_elements_count < 1) {
            fatal("%s: maxElementsCount must be >= 1 (%d)\n", name_,
                max_elements_count);
        }
    }

    ~Queue()
    {
        /* Delete the queue */
        delete &q;
    }

    /** Is their space in the queue */
    bool canEnqueue() { return (q.size() < maxElementsCount); }

    /** Some search function? */
    T find(T ref) { return ref; }

    /** pop the head */
    T pop()
    {
        /** Get the front */
        T head = q.front();

        /** Dequeue */
        q.pop_front();

        /** Return front */
        return head;
    }

    /** Enqueue an element */
    bool enqueue(T e)
    {
        bool enqeued = false;

        if (!isFull()) {
            q.push_back(e);
            enqeued = true;
        }

        return enqeued;
    }

    /** Size */
    unsigned int size() { return q.size(); }

    /** Is empty? */
    bool isEmpty() { return q.empty(); }

    /** Is full? */
    bool isFull() { return q.size() >= maxElementsCount; }

    /** Peak! */
    T front() { return q.front(); }
};

}

#endif /* __COMPONENTS_QUEUE_HH__ */
