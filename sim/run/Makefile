all:
	$(MAKE) -C ../bin
verilator:
	$(MAKE) -f Makefile.verilator -C ../bin
sim_verilator:
	$(MAKE) -f Makefile.verilator -C ../bin sim
sim:
	$(MAKE) -C ../bin sim
clean:
	$(MAKE) -C ../bin clean
	$(MAKE) -f Makefile.verilator -C ../bin clean
