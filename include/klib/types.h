#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <stdint.h>

#ifndef __ASSEMBLY__

#define DECLARE_BITMAP(name,bits) \
	unsigned long name[BITS_TO_LONGS(bits)]

typedef int			bool;

typedef uint32_t		uid_t;
typedef uint32_t		gid_t;
typedef uint16_t		uid16_t;
typedef uint16_t		gid16_t;

typedef unsigned long		uintptr_t;

/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef uint8_t			u_int8_t;
typedef	uint16_t		u_int16_t;
typedef	uint32_t		u_int32_t;

#endif /* !(__BIT_TYPES_DEFINED__) */

typedef	uint64_t		u_int64_t;

typedef int8_t			s8;
typedef int16_t			s16;
typedef int32_t			s32;
typedef int64_t			s64;
typedef uint8_t			u8;
typedef uint16_t		u16;
typedef uint32_t		u32;
typedef uint64_t		u64;

/* this is a special 64bit data type that is 8-byte aligned */
#define aligned_u64 uint64_t __attribute__((aligned(8)))


#endif /*  __ASSEMBLY__ */

struct list_head {
	struct list_head *next, *prev;
};

struct hlist_head {
	struct hlist_node *first;
};

struct hlist_node {
	struct hlist_node *next, **pprev;
};

#endif /* _LINUX_TYPES_H */
