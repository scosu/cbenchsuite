#ifndef _VERSION_H_
#define _VERSION_H_


struct comp_version {
	char *name;
	char *version;
};

struct version {
	char *name;
	struct comp_version *comp_versions;
	void *data;
};


#endif  /* _VERSION_H_ */
