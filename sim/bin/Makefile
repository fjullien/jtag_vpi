SOURCE_DIR=../..
INCLUDE_DIR=$(SOURCE_DIR)/include
BENCH_DIR=$(SOURCE_DIR)/bench

# Set V=1 when calling make to enable verbose output
# mainly for debugging purposes.
ifeq ($(V), 1)
Q=
QUIET=
else
Q ?=@
QUIET=-quiet
endif

IVERILOG_VPI_SRC = $(SOURCE_DIR)/jtag_vpi.c

ADV_DEBUG_SRC = $(shell ls $(BENCH_DIR)/adv_debugsys/*.v)

IVERILOG_SRC = $(SOURCE_DIR)/jtag_vpi.v \
               $(BENCH_DIR)/jtag_tap/jtag_tap.v \
               $(BENCH_DIR)/jtag_vpi_tb.v \
               $(BENCH_DIR)/ram/ram_wb_b3.v \
               $(ADV_DEBUG_SRC)


all :
	echo ##### Building ######
	$(Q)iverilog-vpi $(IVERILOG_VPI_SRC) > /dev/null
	$(Q)iverilog -I$(INCLUDE_DIR) -ojtag_vpi_tb.vvp $(IVERILOG_SRC)

	echo ##### Start the simulation ######
	$(Q)vvp -n -M. -mjtag_vpi jtag_vpi_tb.vvp +jtag_vpi_enable=1

clean:
	@rm -rf *.o *~ *.vpi *.vvp *.vcd
