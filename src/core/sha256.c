#include <cbench/sha256.h>

#include <stdio.h>

void sha256_finish_str(sha256_context *ctx, char *buf)
{
	unsigned char digest[32];
	int i;
	sha256_finish(ctx, digest);

	for (i = 0; i != 32; ++i) {
		sprintf(&buf[i * 2], "%02x", digest[i]);
	}
}
