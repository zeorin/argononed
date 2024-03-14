BUILD_PATH  = ../../build
SRC_FILES 	= $(wildcard *.c)
FILE_OBJ	= $(addprefix $(BUILD_PATH)/, $(addsuffix .o,$(basename $(SRC_FILES))))
CFLAGS		+= -I ../daemon -I ../common -I ../args

$(BUILD_PATH)/%.o: %.c
	@echo "Compile $<"
	$(CC) -c -o $@ $< $(CFLAGS) -DLOG_LEVEL=$(LOGLEVEL) -Wno-missing-braces

.PHONY: all
all: $(FILE_OBJ)