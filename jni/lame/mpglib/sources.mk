C_SOURCES := common.c dct64_i386.c decode_i386.c interface.c layer1.c layer2.c layer3.c tabinit.c
LOCAL_SRC_FILES += $(addprefix mpglib/, $(C_SOURCES))