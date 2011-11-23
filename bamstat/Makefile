SAMTOOLS = ~/code/ext/samtools-0.1.16

CFLAGS = -I $(SAMTOOLS) -L $(SAMTOOLS) -lbam -lz

bamstat: bamstat.cpp histogram.o discbins.o
	g++ -g -o bamstat bamstat.cpp histogram.o discbins.o $(CFLAGS) 

histogram.o: histogram.cpp
	g++ -c histogram.cpp $(CFLAGS)

discbins.o: discbins.cpp
	g++ -c discbins.cpp $(CFLAGS)
