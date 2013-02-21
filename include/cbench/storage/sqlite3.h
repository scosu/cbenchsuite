#ifndef _CBENCH_STORAGE_SQLITE3_H_
#define _CBENCH_STORAGE_SQLITE3_H_

#include <cbench/config.h>
#include <cbench/storage.h>

#ifdef CONFIG_STORAGE_SQLITE3
extern const struct storage_ops storage_sqlite3;
#else
static const struct storage_ops storage_sqlite3 = {};
#endif

#endif  /* _CBENCH_STORAGE_SQLITE3_H_ */
