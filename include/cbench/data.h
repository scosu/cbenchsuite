#ifndef _CBENCH_DATA_H_
#define _CBENCH_DATA_H_

enum data_type {
	DATA_TYPE_MONITOR = 0x1,
	DATA_TYPE_RESULT = 0x2,
};

struct data {
	enum data_type type;
	unsigned int run;
	void *data;
	struct list_head run_data;
};

#endif  /* _CBENCH_DATA_H_ */
