BUILD_PATH  = ../../build
INC_PATH	= -I ../common -I ../args

$(BIN_CLI): argonone-cli.c
	@echo "BUILD $@"
	$(CC) -o $(BUILD_PATH)/$@ $^ $(CFLAGS) $(LFLAGS) $(INC_PATH)