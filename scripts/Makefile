.PHONY: all clean

TOP_DIR   := $(shell cd .. && pwd)
SRC_DIR   := $(TOP_DIR)/src
INC_DIR   := $(TOP_DIR)/inc
RELEASE   := $(TOP_DIR)/release
BUILD_DIR := $(TOP_DIR)/build

CFLAGS += -g -std=gnu99 -I$(INC_DIR) -Wall -Werror

BIN          += statdev
statdev-objs += xutils/file_map.o

BIN          += dumpdev
dumpdev-objs += xutils/file_map.o

BIN          += dumpzfs
dumpzfs-objs += xutils/file_map.o
dumpzfs-objs += tools/zfs_dummy.o

all: $(BIN)

clean:
	$(RM) -r $(BIN) $(BUILD_DIR)

$(BUILD_DIR)/src/%.o: $(SRC_DIR)/%.c
	@echo "CC $<"
	@mkdir -p $(shell dirname $@)
	@$(CC) $(CFLAGS) -c -o $@ $<

define LINK
$(1)-objs += tools/$(1).o
$(1): $$(patsubst %.o,$$(BUILD_DIR)/src/%.o,$$($(1)-objs))
	@echo "LD $$(shell pwd)/$$@"
	@$$(CC) $$(CFLAGS) $$^ -o $$@ $$($(1)-libs)
endef

$(foreach bin,$(BIN),$(eval $(call LINK,$(bin))))

.SECONDARY:
