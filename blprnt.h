#ifndef BLPRNT_H
#define BLPRNT_H

#include "vnode.h"
#include <cstddef>

/***
** @field num_indirs is number of indirections to data
** @field type is type of struct stored
**/
struct field_t
{
    size_t num_indirs;
    size_t type;
};

/***
** @field num_bytes32 is size in 32 bit mode
** @field num_bytes64 is size in 64 bit mode
** @field fields is head of field list
***/
struct blprnt_t
{
    size_t num_bytes32;
    size_t num_bytes64;
    vnode_t<field_t>* fields;
};

#endif
