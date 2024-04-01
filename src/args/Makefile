BUILD_PATH  = ../../build
SRC_FILES 	= args.c
CFLAGS       = -Wall -s -Os
FILE_OBJ	= $(addprefix $(BUILD_PATH)/, $(addsuffix .o,$(basename $(SRC_FILES))))

$(BUILD_PATH)/%.o: %.c
	@echo "Compile $<"
	$(CC) -c -o $@ $< $(CFLAGS) -DLOG_LEVEL=$(LOGLEVEL)

all: $(FILE_OBJ)
	@echo "BUILD args.a"
	$(AR) rsc $(BUILD_PATH)/args.a $^
