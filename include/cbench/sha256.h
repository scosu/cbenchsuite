#ifndef _CBENCH_SHA256_H_
#define _CBENCH_SHA256_H_

#include <sha256-gpl/sha256.h>


/* buf of size at least 65 */
void sha256_finish_str(sha256_context *ctx, char *buf);

#define sha256_add(ctx, ptr) sha256_update(ctx, (uint8_t *)ptr, sizeof(*ptr))

/* Add string to sha256. strlen() + 1 to get a 0 character into it for
 * seperation */
#define sha256_add_str(ctx, ptr) sha256_update(ctx, (uint8_t *)ptr, strlen(ptr) + 1)


#endif   /* _CBENCH_SHA256_H_ */
