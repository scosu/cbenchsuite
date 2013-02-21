#ifndef _CBENCH_STORAGE_CSV_H_
#define _CBENCH_STORAGE_CSV_H_

#include <cbench/storage.h>
#include <cbench/config.h>

#ifdef CONFIG_STORAGE_CSV
extern const struct storage_ops storage_csv;
#else
static const struct storage_ops storage_csv;
#endif

#endif  /* _CBENCH_STORAGE_CSV_H_ */
