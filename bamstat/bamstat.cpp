// bamstat
// Center for Human Genetic Research
// Massachusetts General Hospital
// Adrian Heilbut
// heilbut@chgr.mgh.harvard.edu
// 0.2

#include "bamstat.h"

// this function is copied from samtools
int ksprintf(kstring_t *s, const char *fmt, ...)
{
	va_list ap;
	int l;
	va_start(ap, fmt);
	l = vsnprintf(s->s + s->l, s->m - s->l, fmt, ap); // This line does not work with glibc 2.0. See `man snprintf'.
	va_end(ap);
	if (l + 1 > s->m - s->l) {
		s->m = s->l + l + 2;
		kroundup32(s->m);
		s->s = (char*)realloc(s->s, s->m);
		va_start(ap, fmt);
		l = vsnprintf(s->s + s->l, s->m - s->l, fmt, ap);
	}
	va_end(ap);
	s->l += l;
	return l;
}


// allocate a new cstring of the appropriate length and sprintf the 
// concatenation of strA and strB into it
char * concat(const char * strA,  const char * strB) {
    char * result = (char *) malloc(sizeof(char) * (strlen(strA) + strlen(strB) + 2));
    sprintf(result," %s%s", strA, strB);
    std::cout << strA << " + " << strB << " = " <<  result;
    return result;
}

// print out read pair onto a single line, suitable for sorting and clustering 
// always print pairs in order of the chromosomes
int printPair(std::ofstream &outputStream, samfile_t *samfile, bam1_t *rec1, bam1_t *rec2, bool printRG, char *defaultRG) { 
  uint8_t *s = bam1_seq(rec1), *t = bam1_qual(rec1);
  int i = 0;

  bam1_t *tmp1;
  uint8_t *auxdata;
  char * readgroup;

  // swap order of pairs if necessary
  if (rec1->core.tid < rec2->core.tid || (rec1->core.tid == rec2->core.tid && rec1->core.pos < rec2->core.pos) ) {
    rec1 = rec1; rec2 = rec2;
  } else {
    tmp1 = rec1; rec1 = rec2;  rec2 = tmp1;
  }
  
  if (printRG) {
    auxdata = bam_aux_get(rec1, "RG");
    if (auxdata != 0) {
      readgroup = bam_aux2Z(auxdata);
    } else {
      readgroup = defaultRG;
    }
    outputStream << readgroup << "\t";
  }

  if (rec1->core.tid != -1 && rec2->core.tid != -1) {
    outputStream
      << bam1_qname(rec1) << "\t"                                            
      << samfile->header->target_name[rec1->core.tid] 
      << "\t" << rec1->core.pos
      << "\t" << rec1->core.flag
      << "\t" << samfile->header->target_name[rec2->core.tid] 
      << "\t" << rec2->core.pos
      << "\t" << rec2->core.flag
      << "\t" << rec1->core.l_qseq
      << "\t" << rec1->core.qual 
      << "\t" << rec2->core.l_qseq
      << "\t" << rec2->core.qual << "\t";

    // seq A
    s = bam1_seq(rec1); t = bam1_qual(rec1);
    for(i = 0; i < rec1->core.l_qseq; ++i) {
      outputStream << bam_nt16_rev_table[bam1_seqi(s, i)];
    }
    outputStream << "\t";

    // qual A
    if (t[0] == 0xff) { outputStream << "*";  }                                                                   
    else for (i = 0; i < rec1->core.l_qseq; ++i) {
	outputStream << (char) (t[i] + 33);    
      }
    outputStream << "\t";
    
    // seq B
    s = bam1_seq(rec2); t = bam1_qual(rec2);
    for (i = 0; i < rec2->core.l_qseq; ++i) {
      outputStream << bam_nt16_rev_table[bam1_seqi(s, i)];
    } 
    outputStream << "\t";

    // qual B 
    if (t[0] == 0xff) {
      outputStream << "*";
    }                                                                                                                            
    else for (i = 0; i < rec2->core.l_qseq; ++i) {
	outputStream << (char) (t[i] + 33);    
      }

    outputStream << std::endl;  
  }
}

int writeFASTQ(FILE *outfile, samfile_t * samfile, bam1_t * b) {
 fprintf(outfile, "@%s\n", bam1_qname(b));
 
 const bam1_core_t *c = &b->core;
 uint8_t *s = bam1_seq(b), *t = bam1_qual(b);
 int i = 0;
 
 kstring_t str;
 str.l = str.m = 0; str.s = 0;

 if (c->l_qseq) {
	for (i = 0; i < c->l_qseq; ++i) kputc(bam_nt16_rev_table[bam1_seqi(s, i)], &str);
	kputc('\n', &str);
	ksprintf(&str, "+");
	ksprintf(&str, bam1_qname(b));
	kputc('\n', &str);
	if (t[0] == 0xff) kputc('*', &str);
	else for (i = 0; i < c->l_qseq; ++i) kputc(t[i] + 33, &str);
 } // else ksprintf(&str, "*\t*");

 fprintf(outfile, "%s\n", str.s); 
 return 0; 
}

class alignmentStats {
public:

 alignmentStats();
 ~alignmentStats() {}
 int writeResults(std::string filename);

 long mapped;       // number of mapped reads
 long unmapped;     // number of unmapped reads
 long total;        // number of reads total
 long posmapq;      // nubmer of reads with positive mapping quality scores
 
 long proper_RR_count;  
 long proper_RF_count;
 long proper_FR_count;
 long proper_FF_count;
 
 long notproper_RR_count;
 long notproper_RF_count;
 long notproper_FR_count;
 long notproper_FF_count;

 long notproper_neitherMapped;
 long notproper_oneEndMapped;
 long notproper_bothEndsMapped;

 long proper_sameChrom;
 long proper_diffChrom;
 
 long notproper_sameChrom;
 long notproper_diffChrom;
 
 long putative_RF_deletions;
 long putative_FR_deletions;
 long putative_RR_deletions;
 long putative_FF_deletions;

 long putative_RF_insertions;
 long putative_FR_insertions;
 long putative_RR_insertions;
 long putative_FF_insertions;

 
 DiscBinFreqTable * db_longReads;
 
 RunningStat stat_longReads;
 RunningStat stat_shortReads;
 
};

// alignmentStats constructor
alignmentStats::alignmentStats() {
 mapped = 0;
 unmapped = 0;
 total = 0;
 posmapq = 0;
 
 proper_RR_count = 0;
 proper_RF_count = 0;
 proper_FR_count = 0;
 proper_FF_count = 0;
 
 notproper_RR_count = 0;
 notproper_RF_count = 0;
 notproper_FR_count = 0;
 notproper_FF_count = 0;

 notproper_neitherMapped = 0;
 notproper_oneEndMapped = 0;
 notproper_bothEndsMapped = 0;

 proper_sameChrom = 0;
 proper_diffChrom = 0;
 
 notproper_sameChrom = 0;
 notproper_diffChrom = 0;    
  
 putative_RF_deletions = 0;
 putative_FR_deletions = 0;
 putative_RR_deletions = 0;
 putative_FF_deletions = 0;

 putative_RF_insertions = 0;
 putative_FR_insertions = 0;
 putative_RR_insertions = 0;
 putative_FF_insertions = 0;
 
 this->db_longReads = new DiscBinFreqTable(500, 20000);
}

int alignmentStats::writeResults(std::string filename) {
 
 std::ofstream statsfile(filename.c_str());
 
 statsfile << "Mapped Reads" << "\t" << mapped << std::endl;
 statsfile << "Total Reads" << "\t" << total << std::endl;
 
 statsfile << "Counts of (Mapped) Pairs" << std::endl;
 statsfile << "----" << std::endl;
 statsfile << "Orientation" << "\t" << "Proper Pairs" << "\t" << "Improper Pairs" << std::endl;
 statsfile << "RR" << "\t" << proper_RR_count << "\t" << notproper_RR_count << std::endl;
 statsfile << "RF" << "\t" << proper_RF_count << "\t" << notproper_RF_count << std::endl;
 statsfile << "FR" << "\t" << proper_FR_count << "\t" << notproper_FR_count << std::endl;
 statsfile << "FF" << "\t" << proper_FF_count << "\t" << notproper_FF_count << std::endl;

 statsfile << std::endl; 
 statsfile << "Properly Paired / Same Chrom" << "\t" << proper_sameChrom << std::endl;
 statsfile << "Properly Paired / Diff Chrom" << "\t" << proper_diffChrom << std::endl;

 statsfile << std::endl; 
 statsfile << "Not Paired / Same Chrom" << "\t" << notproper_sameChrom << std::endl;
 statsfile << "Not Paired / Diff Chrom" << "\t" << notproper_diffChrom << std::endl;
 
 statsfile << std::endl; 
 statsfile << "Not Paired / Neither End Mapped" << "\t" << notproper_neitherMapped << std::endl;
 statsfile << "Not Paired / One End Mapped" << "\t" << notproper_oneEndMapped << std::endl;
 statsfile << "Not Paired / Both Ends Mapped" << "\t" << notproper_bothEndsMapped << std::endl;
 
 statsfile << std::endl;
 statsfile << "Reads Mapped" << "\t" << mapped << std::endl; 
 statsfile << "Reads Unmapped" << "\t" << unmapped << std::endl; 
 statsfile << "Reads Total" << "\t" << total << std::endl; 
 statsfile << "Reads with Positive MAPQ" << "\t" << posmapq << std::endl;
 
 statsfile << "actual RF Long-Insert Mean isize:" << "\t" <<  stat_longReads.Mean() << std::endl;
 statsfile << "actual RF Long-Insert Mean stddev:" << "\t" <<  stat_longReads.StandardDeviation() << std::endl;

 statsfile << "actual RF Long-Insert Median isize" << "\t" << db_longReads->getMedian() << std::endl;

 statsfile << "actual FR Short-Insert Mean isize:" << "\t" <<  stat_shortReads.Mean() << std::endl;
 statsfile << "actual FR Short-Insert Mean stddev:" << "\t" <<  stat_shortReads.StandardDeviation() << std::endl;

 statsfile << "# potential deletions " << std::endl; 
 statsfile << "putative RF deletions" << "\t" << putative_RF_deletions << std::endl;
 statsfile << "putative FR deletions" << "\t" << putative_FR_deletions << std::endl;
 statsfile << "putative RR deletions" << "\t" << putative_RR_deletions << std::endl;
 statsfile << "putative FF deletions" << "\t" << putative_FF_deletions << std::endl;
 
 statsfile.close();
 
}

int printUsage() {
    std::cout << "bamstat - read statistics and extraction - ";
    std::cout << "MGH CHGR <heilbut@chgr.mgh.harvard.edu> " << std::endl;
    std::cout << "bamstat usage:" << std::endl;
    std::cout << "bamstats -i <inputfile> -o <outputdir>  [-b] [-u] [-g DefaultRG] " << std::endl;
    //    std::cout << "[-ha min max numbins]" << std::endl;
    return 0;

    // [-sm <shortmean> -ssd <shortstddev>] [-lm <longmean> -lsd <longstddev>]
}

int main(int argc, char **argv) {

 bam1_t *rec1 = bam_init1();
 bam1_t *rec2 = bam_init1();

 int status;
 
 alignmentStats stats;
  
 histogram hist_shortReads(0, 1000, 20);
 histogram hist_longReads(1000, 10000, 20);
 histogram hist_all(0, 6000, 60);

 
 char * bamfilename = NULL; 
 char * outputDir;
 int c;
 bool OPT_BAMinput = false;
 bool OPT_writeUnmapped = false;
 bool OPT_printRGinfo = false;  // by default, don't print out read group identifier
 char * defaultRG = 0;

 bool somethingNotMapped = false;

 long sampleCount = 0;
 long properSampleCount = 0;

 float INDEL_THRESHOLD = 3;

 outputDir = (char *) malloc(20 * sizeof(char));
 sprintf(outputDir, "./");

 RunningStat samplestat_longReads;
 RunningStat samplestat_shortReads; 

 // get command line options
 while ((c = getopt(argc, argv,"i:buo:s:g:")) != -1) {
    switch (c)
      {
      case 'i':
        // input file name
	    bamfilename = optarg; 
        break;
      case 'b':
        // input is BAM, not SAM
        OPT_BAMinput = true;
        break;
      case 'u':
        // output unmapped read pairs for re-mapping
        OPT_writeUnmapped = true;
        break;
      case 'o':
        // output directory name
        outputDir = optarg;
	break;
      case 's':
	INDEL_THRESHOLD = atof(optarg);
	break;
      case 'g':
	OPT_printRGinfo = true;
	defaultRG = optarg;
      }
 } 
 
 if (bamfilename == NULL) {
   printUsage();
   return 0;
 }
 
 std::ofstream fsUnmappedReads("unmappedPairs.list"); 
 
 // if reading a bam file, then the need to specify b in the type flag
 samfile_t *samfile;

 // samfile outputs
 samfile_t *fpDeletions;
 samfile_t *fpInsertions;
 samfile_t *fpInversions;
 samfile_t *fpTranslocations;

 FILE *outreads1;
 FILE *outreads2;

 std::ofstream translocPairs("translocPairs.txt");
 std::ofstream deletionPairs("deletionPairs.txt");
 std::ofstream inversionPairs("inversionPairs.txt");

 std::ofstream mappedLowScore("mappedLowScore.txt");

  if (OPT_writeUnmapped) {
    outreads1 = fopen(concat(outputDir, "/unaligned_reads1.fastq"), "w");
    outreads2 = fopen(concat(outputDir,"/unaligned_reads2.fastq"), "w");
 }
 
 if (OPT_BAMinput) {
    samfile = samopen(bamfilename, "rb", NULL);
 } else {
    samfile = samopen(bamfilename, "r", NULL);
 }
 

 // open output files
 fpDeletions = samopen("deletions.bam", "wb", samfile->header);
 fpInsertions = samopen("insertions.bam", "wb", samfile->header);
 fpInversions = samopen("inversions.bam", "wb", samfile->header);
 fpTranslocations = samopen("translocations.bam", "wb", samfile->header);
 // fpdeletions = samopen(concat(outputDir, "/RFdeletions.bam"), "wb", samfile->header);
 


 // sample first N records to get insert size statistics
 while (sampleCount < 1000000 && samread(samfile, rec1) > 0) { 
    samread(samfile, rec2);
    sampleCount += 2;

    if (strncmp(bam1_qname(rec1), bam1_qname(rec2), strlen(bam1_qname(rec1)-2)) != 0) {
   	printf("Error.  Paired-ends must be sequential in the input SAM/BAM file for this program to work correctly\n");
   	printf("Please sort the input alignments by the read ID.\n");
	return 0;
   }
        
   if (rec1->core.isize != 0 && rec2->core.isize != 0 
       && !testFlag(rec1->core.flag, BAM_FUNMAP) 
       && !testFlag(rec2->core.flag, BAM_FUNMAP) 
       && rec1->core.tid == rec2->core.tid 
       && (abs(rec1->core.isize) < 20000) 
       && (abs(rec2->core.isize) < 20000)) { // (testFlag(rec1->core.flag, BAM_FPROPER_PAIR)) {
    
     properSampleCount +=2; 
    if (rec1->core.pos < rec2->core.pos) {
      // RF 
      if (testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) {      
	// std::cout << rec1->core.isize << "\t" << rec1->core.pos << "\t" << rec2->core.pos << "\t" << bam1_qname(rec1) << std::endl;
        if (rec1->core.isize > 0 ) samplestat_longReads.Push(rec1->core.isize);
        if (rec2->core.isize > 0 ) samplestat_longReads.Push(rec2->core.isize);
       }
    
      // FR      
      if (!testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) { 
        if (rec1->core.isize > 0 ) samplestat_shortReads.Push(rec1->core.isize);
        if (rec2->core.isize > 0 ) samplestat_shortReads.Push(rec2->core.isize);
      }
    
    } else {
      // rec2 maps after rec1 
      // FR  
      if (testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) {
        if (rec1->core.isize > 0 ) samplestat_shortReads.Push(rec1->core.isize);
        if (rec2->core.isize > 0 ) samplestat_shortReads.Push(rec2->core.isize);
      }
      
      // RF
      if (!testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) { 
	// std::cout << rec1->core.isize << "\t" << rec1->core.pos << "\t" << rec2->core.pos << "\t" << bam1_qname(rec1) << std::endl;
        if (rec1->core.isize > 0 ) samplestat_longReads.Push(rec1->core.isize);       // add to size statistics
        if (rec2->core.isize > 0 ) samplestat_longReads.Push(rec2->core.isize);       // add to size statistics
      }
    }
   }
 
 }
 samclose(samfile);
 // end of sampling phase

 // print out sampling statistics
 std::cout << "Using insert size threshold of " << INDEL_THRESHOLD << " std deviations for possible indels" << std::endl;
 std::cout << BLUE << "[[[[ Sampling Phase ]]]]" << COL_RESET << std::endl;
 std::cout << "Sampled " << sampleCount << "; " << properSampleCount << " in proper pairs" << std::endl;
 std::cout << "sample RF mean insert size: " << samplestat_longReads.Mean() << std::endl;
 std::cout << "sample RF stddev: " << samplestat_longReads.StandardDeviation() << std::endl;
 std::cout << "sample FR mean insert size: " << samplestat_shortReads.Mean() << std::endl;
 std::cout << "sample FR stddev: " << samplestat_shortReads.StandardDeviation() << std::endl;
 

 // reopen sam/bam file for the actual processing
 
 if (OPT_BAMinput) {
    samfile = samopen(bamfilename, "rb", NULL);
 } else {
    samfile = samopen(bamfilename, "r", NULL);
 }
  

 std::cout << std::endl << GREEN << "[[[[ Processing Phase ]]]]" << COL_RESET << std::endl;  
 // iterate through SAM / BAM file and accumulate statistics
 while (samread(samfile, rec1) > 0) { 
   
   status = samread(samfile, rec2);
    
   somethingNotMapped = false;
   stats.total += 2;  // total number of reads, not pairs
   

   //   printf("%d %d %d %d\n", rec1->core.tid, rec2->core.tid, rec3->core.tid, rec4->core.tid);
   //   printf("%d %d %d %d\n", rec1->core.qual, rec2->core.qual, rec3->core.qual, rec4->core.qual);
   //   printf("%d %d %d %d\n", rec1->core.isize, rec2->core.isize, rec3->core.isize, rec4->core.isize);
 
   // depending on the alignment program, the reads may or may not have been classified as properly paired 
   // given their orientations
   
   // stats concerning individual reads: count both ends of the pair
   if (testFlag(rec1->core.flag, BAM_FUNMAP)) { stats.unmapped++; } else { stats.mapped++; }
   if (testFlag(rec2->core.flag, BAM_FUNMAP)) { stats.unmapped++; } else { stats.mapped++; }
 
   // reads with positive mapq score
   if (rec1->core.qual > 0) stats.posmapq += 1; 
   if (rec2->core.qual > 0) stats.posmapq += 1; 

 
   // stats concerning pairs
   if (testFlag(rec1->core.flag, BAM_FPROPER_PAIR)) {
    // PROPERLY PAIRED
    if (testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) { 
        stats.proper_RR_count++;
	samwrite(fpInversions, rec1);
	samwrite(fpInversions, rec2);
    }
        
    if (!testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) { 
        stats.proper_FF_count++;
	samwrite(fpInversions, rec1);
	samwrite(fpInversions, rec2);
    }

    // the R/F or F/R orientation is dependent on the mapped genomic location
    // have been read from either direction  
    // tabulate insert size distributions
    // only count the ends that have positive insert size - every pair is going to have one read with a positive value,
    // and one that is negative with the same magnitude  
    // for the small inserts, we only count reads that are in the expected F-R orientation 

    if (rec1->core.pos < rec2->core.pos) {
      // RF 
      if (testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) {      
        stats.proper_RF_count++;
        if (rec1->core.isize > 0 ) {
         hist_longReads.count(rec1->core.isize);          // add to histogram
         stats.stat_longReads.Push(rec1->core.isize);     // add to size statistics
         stats.db_longReads->count(rec1->core.isize);
         
         // if isize is bigger than 2 x sampled stddev, output the reads as potential deletions
         if (rec1->core.isize > samplestat_longReads.Mean() + INDEL_THRESHOLD * samplestat_longReads.StandardDeviation()) {
           stats.putative_RF_deletions++;
	   samwrite(fpDeletions, rec1);
	   samwrite(fpDeletions, rec2);
	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
         } 
         
         }
         
        if (rec2->core.isize > 0 ) { 
         hist_longReads.count(rec2->core.isize);  
         stats.stat_longReads.Push(rec2->core.isize);
         stats.db_longReads->count(rec2->core.isize);

         // if isize is bigger than INDEL_THRESHOLD x sampled stddev, output the reads as potential deletions
         if (rec2->core.isize > samplestat_longReads.Mean() + INDEL_THRESHOLD * samplestat_longReads.StandardDeviation()) {
           stats.putative_RF_deletions++;
	       samwrite(fpDeletions, rec1);
    	   samwrite(fpDeletions, rec2);
	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
         } 
        }
       }
    
      // FR      
      if (!testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) { 
        stats.proper_FR_count++;
        if (rec1->core.isize > 0 ) { 
            hist_shortReads.count(rec1->core.isize);
            stats.stat_shortReads.Push(rec1->core.isize);

         // if isize is bigger than INDEL_THRESHOLD x sampled stddev, output the reads as potential deletions
         if (rec1->core.isize > samplestat_shortReads.Mean() + INDEL_THRESHOLD * samplestat_shortReads.StandardDeviation()) {
           stats.putative_FR_deletions++;
           samwrite(fpDeletions, rec1);
           samwrite(fpDeletions, rec2); 
         } 


        }
        
        if (rec2->core.isize > 0 ) { 
            hist_shortReads.count(rec2->core.isize);
            stats.stat_shortReads.Push(rec2->core.isize);

         // if isize is bigger than INDEL_THRESHOLD x sampled stddev, output the reads as potential deletions
         if (rec2->core.isize > samplestat_shortReads.Mean() + INDEL_THRESHOLD * samplestat_shortReads.StandardDeviation()) {
           stats.putative_FR_deletions++;
           samwrite(fpDeletions, rec1);                      
           samwrite(fpDeletions, rec2); 
         } 

        }
      }
    
    } else {
      // rec2 maps after rec1 
      // FR  
      if (testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) {
        stats.proper_FR_count++;
        if (rec1->core.isize > 0 ) { 
            hist_shortReads.count(rec1->core.isize);
            stats.stat_shortReads.Push(rec1->core.isize);

         if (rec1->core.isize > samplestat_shortReads.Mean() + INDEL_THRESHOLD * samplestat_shortReads.StandardDeviation()) {
           stats.putative_FR_deletions++;
	   samwrite(fpDeletions, rec1);
	   samwrite(fpDeletions, rec2);

	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
         } 
        }
        
        if (rec2->core.isize > 0 ) { 
            hist_shortReads.count(rec2->core.isize);        
            stats.stat_shortReads.Push(rec2->core.isize);

         if (rec2->core.isize > samplestat_shortReads.Mean() + INDEL_THRESHOLD * samplestat_shortReads.StandardDeviation()) {
           stats.putative_FR_deletions++;
	   samwrite(fpDeletions, rec1);
	   samwrite(fpDeletions, rec2);

	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
         } 
        }  
      }
      
      // RF
      if (!testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) { 
        stats.proper_RF_count++;    
        if (rec1->core.isize > 0 ) { 
         hist_longReads.count(rec1->core.isize);            
         stats.stat_longReads.Push(rec1->core.isize);       // add to size statistics
         stats.db_longReads->count(rec1->core.isize);

         
         // if isize is bigger than 2 x sampled stddev, output the reads as potential deletions
         if (rec1->core.isize > samplestat_longReads.Mean() + INDEL_THRESHOLD * samplestat_longReads.StandardDeviation()) {
           stats.putative_RF_deletions++;
	   samwrite(fpDeletions, rec1);
	   samwrite(fpDeletions, rec2);

	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);

         }
        } 
        
        if (rec2->core.isize > 0 ) { 
         hist_longReads.count(rec2->core.isize);
         stats.stat_longReads.Push(rec2->core.isize);       // add to size statistics
         stats.db_longReads->count(rec2->core.isize);

         // if isize is bigger than 2 x sampled stddev, output the reads as potential deletions
         if (rec2->core.isize > samplestat_longReads.Mean() + INDEL_THRESHOLD * samplestat_longReads.StandardDeviation()) {
           stats.putative_RF_deletions++;
	   samwrite(fpDeletions, rec1);
	   samwrite(fpDeletions, rec2);

	   printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);

         }
        }
      }
    }
    

    // proper pair that maps to different chromosomes
    if (rec1->core.tid != rec2->core.tid) { 
      
      // this is not possible!!
//      std::cerr << "ERROR! It shouldn't be possible for a proper pair to map to different chromosomes" << std::endl;
      stats.proper_diffChrom++; 
      samwrite(fpTranslocations, rec1);
      samwrite(fpTranslocations, rec2);
     
      if (rec1->core.tid != -1 && rec2->core.tid != -1) { 
    	printPair(translocPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);

	}

 //     std::cerr << bam1_qname(rec1) << "\t"                                                                                                          
//		    << samfile->header->target_name[rec1->core.tid] << "\t" << rec1->core.pos << "\t" << rec1->core.flag << "\t"
//		    << samfile->header->target_name[rec2->core.tid] << "\t" << rec2->core.pos << "\t" << rec2->core.flag << "\t"
//		    << rec1->core.l_qseq << "\t" << rec1->core.qual << "\t"
//		    << rec2->core.l_qseq << "\t" << rec2->core.qual << std::endl;  

    } else {
      stats.proper_sameChrom++; 
    }
        hist_all.count(rec1->core.isize);
        hist_all.count(rec2->core.isize);
   } 
   
   
   else {  // Not Mapped in Proper Pair
    
    // iff *both* ends mapped, count pair orientations
    // not paired; both ends mapped
    if (!testFlag(rec1->core.flag, BAM_FUNMAP) && !testFlag(rec2->core.flag, BAM_FUNMAP)) { 
    
        stats.notproper_bothEndsMapped++;
        //    fsUnmappedReads << bam1_qname(rec1) << "\n";

        // this is a potential deletion or translocation
        // if the chromosomes are different, it's a potential translocation, regardless of orientations
        if (rec1->core.tid != rec2->core.tid) {
          samwrite(fpTranslocations, rec1);
          samwrite(fpTranslocations, rec2);

          // write out read pair tab-delimited text file for clustering input
          //      writeDL(f_chimericPair, [a.qname, a.rname, a.pos, a.flag, b.rname, b.pos, b.flag, len(a.seq), a.mapq, len(b.seq), b.mapq])
          printPair(translocPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
    
        } else {
          // it's a possible inversion, which will look like a deletion but have wrong orientation
          // the "mapped in proper pair flag can't necessarily be trusted
          
          // for now, it's an inversion if both reads map to the same strand
          if (testFlag(rec1->core.flag, BAM_FREVERSE) == testFlag(rec2->core.flag, BAM_FREVERSE)) { 
	    printPair(inversionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
          }
          
          // deletion
          else {
	    // it's a possible deletion, check if insert size is greater than 2 times std dev
	    if (rec1->core.isize > samplestat_longReads.Mean() + INDEL_THRESHOLD * samplestat_longReads.StandardDeviation()) {
	      printPair(deletionPairs, samfile, rec1, rec2, OPT_printRGinfo, defaultRG);
	      samwrite(fpDeletions, rec1);
	      samwrite(fpDeletions, rec2);
	    }
          }
                                                                                                                          
    }

    if (testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) stats.notproper_RR_count++;
    if (testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) stats.notproper_RF_count++;
    if (!testFlag(rec1->core.flag, BAM_FREVERSE) && testFlag(rec2->core.flag, BAM_FREVERSE)) stats.notproper_FR_count++;
    if (!testFlag(rec1->core.flag, BAM_FREVERSE) && !testFlag(rec2->core.flag, BAM_FREVERSE)) stats.notproper_FF_count++;   

    }

    // both ends mapped
    if (!testFlag(rec1->core.flag, BAM_FUNMAP) && !testFlag(rec2->core.flag, BAM_FUNMAP) && rec1->core.tid != rec2->core.tid)
      { stats.notproper_diffChrom++; } 
    else if (!testFlag(rec1->core.flag, BAM_FUNMAP) && !testFlag(rec2->core.flag, BAM_FUNMAP) && rec1->core.tid == rec2->core.tid)
      { stats.notproper_sameChrom++; }

    // not paired; neither end mapped
    if (testFlag(rec1->core.flag, BAM_FUNMAP) && testFlag(rec2->core.flag, BAM_FUNMAP)) {
    	 stats.notproper_neitherMapped++;
         somethingNotMapped = true;
    }
    
    // not paired; one end mapped
    if (!testFlag(rec1->core.flag, BAM_FUNMAP) && testFlag(rec2->core.flag, BAM_FUNMAP)) { 
     stats.notproper_oneEndMapped++;
     somethingNotMapped = true; 
    } 
    
    if (testFlag(rec1->core.flag, BAM_FUNMAP) && !testFlag(rec2->core.flag, BAM_FUNMAP)) { 
     stats.notproper_oneEndMapped++;
     somethingNotMapped = true;
    }


    // write out to list of unmapped read pairs
    // this list gets used to generate a new set of (trimmed) reads for iterative searching
    if (somethingNotMapped) {
        fsUnmappedReads << bam1_qname(rec1) << "\t";
        if (testFlag(rec1->core.flag, BAM_FUNMAP)) {
            fsUnmappedReads << "1" << "\t";
        } else {
            fsUnmappedReads << "0" << "\t";            
        }
         
        if (testFlag(rec2->core.flag, BAM_FUNMAP)) {
            fsUnmappedReads << "1";
        } else {
            fsUnmappedReads << "0";            
        }

        fsUnmappedReads << std::endl;
    }

    // even if both reads were mapped, they could have been mapped incorrectly
    // write out the ids of that have mapping scores below threshold
    else if (rec1->core.qual < 50 || rec2->core.qual < 50) {
      mappedLowScore << bam1_qname(rec1) << "\t" << "1\t1"  <<  std::endl;
    }

    // write out unmapped reads to a new fastq
    // if (somethingNotMapped && OPT_writeUnmapped) {
    //  writeFASTQ(outreads1, samfile, rec1);
    //  writeFASTQ(outreads2, samfile, rec2);      
    //}

    // not paired; both ends mapped
    if (!testFlag(rec1->core.flag, BAM_FUNMAP) && !testFlag(rec2->core.flag, BAM_FUNMAP)) stats.notproper_bothEndsMapped++;
   
    }  // not mapped in proper pair
   
   if ((stats.total % 1000000) == 0) {
    printf("%ld reads processed\n",stats.total);
   }
   
 } // main loop

 printf("%ld reads processed\n",stats.total);

 // clean up
 samclose(samfile);

 samclose(fpDeletions);
 samclose(fpInsertions);
 samclose(fpInversions);
 samclose(fpTranslocations);


 // if (OPT_writeUnmapped) {
 //    fclose(outreads1);
 //    fclose(outreads2);
 //}

 // print results 

 stats.writeResults("stats.file");
 hist_shortReads.writeFile("shortReads.histdata");
 hist_longReads.writeFile("longReads.histdata");
 hist_all.writeFile("allReads.histdata");
 
 fsUnmappedReads.close();
 mappedLowScore.close();

 return 0;

} 
