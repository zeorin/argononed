BUILD_PATH  = ../../build
SRC_FILES 	= argononed.c argonone_config.c argonone_shm.c argonone_ipc.c
FILE_OBJ	= $(addprefix $(BUILD_PATH)/, $(addsuffix .o,$(basename $(SRC_FILES))))
INC_PATH	= -I ../common -I ../args

$(BUILD_PATH)/%.o: %.c
	@echo "Compile $<"
	$(CC) -c -o $@ $< $(CFLAGS) $(INC_PATH) -DLOG_LEVEL=$(LOGLEVEL) -Wno-missing-braces


$(BIN_DAEMON): $(FILE_OBJ) $(BUILD_PATH)/args.a $(BUILD_PATH)/logger.o $(BUILD_PATH)/event_timer.o
	@echo "BUILD $@"
	$(CC) -o $(BUILD_PATH)/$@ $^ $(CFLAGS) $(LFLAGS)

$(BIN_SHUTDOWN): argonone-shutdown.c
	@echo "BUILD $@"
	$(CC) -o $(BUILD_PATH)/$@ $^ $(CFLAGS) $(LFLAGS)