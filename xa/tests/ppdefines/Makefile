
XA=../../xa

all: test1 test2 test3 test4 test5 test6 test6a test6b test7 clean

clean:
	rm a.o65

%:%.a65
	${XA} -XC $<
	cmp a.o65 $@.o65


