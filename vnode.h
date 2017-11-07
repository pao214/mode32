#ifndef VNODE_H
#define VNODE_H

/***
** TODO: remove unwanted API
** TODO: remove unwanted data
** wrapper template class to help with linear linked list
** store head to store list
***/
#include <cassert>

template <typename T>
struct vnode_t
{
    T val;
    vnode_t *next;
};

/***
** allocates new node
** @param val is content of new node
** @return pointer to allocated node
***/
template <typename T>
vnode_t<T>* vnode_alloc(T& val)
{
    vnode_t<T>* lvnode = (vnode_t<T>*)malloc(sizeof(vnode_t<T>));
    lvnode->val = val; // extra copy
    lvnode->next = NULL;
    return lvnode;
}

/***
** appends val to head of list
** @param lvnode is head of list
** @param val is node to be appended
** @return new head of list
***/
template <typename T>
vnode_t<T>* vnode_insert(vnode_t<T>* lvnode, vnode_t<T>* val)
{
    val->next = lvnode;
    return val;
}

/***
** appends new node at head of list
** @param lvnode is head of list
** @param val is content of new node to be appended
** @return new head of list
***/
template <typename T>
vnode_t<T>* vnode_insert(vnode_t<T>* lvnode, T& val)
{
    vnode_t<T>* tvnode = vnode_alloc<T>(val);
    return vnode_insert(lvnode, tvnode);
}

/***
** appends new node only when content not already present
** @param lvnode is head of list
** @param val is new content
** @return new head of list
***/
template <typename T>
vnode_t<T>* vnode_distinct_insert(vnode_t<T>* lvnode, T& val)
{
    vnode_t<T>* tvnode;
    tvnode = lvnode;

    while (tvnode && ((tvnode->val) != val))
    {
        tvnode = tvnode->next;
    }

    if (tvnode)
    {
        return lvnode;
    }

    return vnode_insert(lvnode, val);
}

#endif
