#include "readPair.hpp" 

readPair::readPair(vector<string*> &linedata) {
  int offset;
  readgroupName = "";
  // check that the line has the right number of values
  if (linedata.size() == 15) {
    offset = 0;
  }
  else if (linedata.size() == 16) {
    readgroupName = *(linedata[0]);
    offset = 1;
  }
  else {
    throw "Wrong number of values in read pair line"; 
  }

  readID = *(linedata[0 + offset]);
  chrA = *(linedata[1 + offset]);
  chrB = *(linedata[4 + offset]);
  posA = atol((*linedata[2 + offset]).c_str());
  posB = atol((*linedata[5 + offset]).c_str());
  scoreA = atoi((*linedata[8 + offset]).c_str());
  scoreB = atoi((*linedata[10 + offset]).c_str());  
  lenA = atoi((*linedata[7 + offset]).c_str());
  lenB = atoi((*linedata[9 + offset]).c_str());  
  flagA = atoi((*linedata[3 + offset]).c_str());
  flagB = atoi((*linedata[6 + offset]).c_str());  
  
  seqA = *(linedata[11 + offset]);
  qualA = *(linedata[12 + offset]);

  seqB = *(linedata[13 + offset]);
  qualB = *(linedata[14 + offset]);
  
}

// readPair::readPair(bam1_t *a, bam1_t *b) {
//  readID = "readID";
//  chrA = 
//}

void readPair::print() {
  if (this->readgroupName.compare("") != 0) {
    cout << this->readgroupName << "\t";
  }
  cout << this->readID << "\t";
  cout << this->chrA << "\t";
  cout << this->chrB << "\t";
  cout << this->posA << "\t";
  cout << this->posB << "\t";
  cout << this->scoreA << "\t";
  cout << this->scoreB << "\t";
  cout << this->lenA << "\t";
  cout << this->lenB << "\t";
  cout << this->flagA << "\t";
  cout << this->flagB << "\t";

  cout << this->seqA << "\t";
  cout << this->seqB << "\t";

  cout << this->qualA << "\t";
  cout << this->qualB << "\t";

  cout << endl;
  
}

readPair::~readPair() {
 
}
