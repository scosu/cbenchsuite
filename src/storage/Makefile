
obj-$(CONFIG_STORAGE_CSV) += csv.o
obj-$(CONFIG_STORAGE_SQLITE3) += sqlite3.o

ifeq  ($(CONFIG_STORAGE_SQLITE3),y)
CORE_LDFLAGS += -lsqlite3
endif
