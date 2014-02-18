SRCS = src 

all:
	@for src in $(SRCS); do\
		echo $$src;\
		cd $$src; make; cd ..;\
	done

clean:
	@for src in $(SRCS); do\
		echo $$src;\
		cd $$src; make clean; cd ..;\
	done

debug:
	@for src in $(SRCS); do\
		echo $$src;\
		cd $$src; make debug; cd ..;\
	done

opt:
	@for src in $(SRCS); do\
		echo $$src;\
		cd $$src; make opt; cd ..;\
	done

solaris:
	@for src in $(SRCS); do\
		echo $$src;\
		cd $$src; make -f makefile.sol; cd ..;\
	done
