SUBDIRS := shm_sample

.PHONY: all clean

all:
	@for subdir in $(SUBDIRS);                \
	do                                        \
		echo "making $@ in $$subdir";     \
		(cd $$subdir && $(MAKE)) || exit; \
	done;


clean:
	@for subdir in $(SUBDIRS);                        \
	do                                                \
		echo "making $@ in $$subdir";             \
		(cd $$subdir && $(MAKE) clean) || exit;   \
	done;
