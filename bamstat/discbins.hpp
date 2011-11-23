#include <iostream>

// discrete bin frequency table

class DiscBinFreqTable {
	public:
		DiscBinFreqTable(long min, long max);
		long count(long value);
		long getMedian();
		~DiscBinFreqTable();
	
	
	private:
		long min;
		long max;
		long numCounts;
		long totalValues;
		
		// counts is an array of longs with length (max - min) + 2
		// it stores each value
		long *counts;
	
};
