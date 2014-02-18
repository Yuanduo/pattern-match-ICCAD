#include <string>
#include <cstring>
#include <iostream>
#include <time.h>
#include "DataReader.h"
#include "FPMLayout.h"
#include "FPMPattern.h"
#include "FPMTempEdge.h"
#include "Plot.h"
//#include "FPMGraph.h"
using namespace FPM;
using namespace PLOT;
using namespace std;

int S1_DISTANCE;
int S2_DISTANCE;
int MATCH_COUNT;
int MEDGE_SIZE;
int ADD_COUNT;
int WIDTH_DIFF;
int HEIGHT_DIFF;
int GRAPH_EDGE_DIFF;
int POLY_EDGE_DIFF;

int main(int argc, char **argv)
{
  S1_DISTANCE = 60;
  S2_DISTANCE = 50;
  MATCH_COUNT = 2;
  MEDGE_SIZE = 5;
  ADD_COUNT = 1;
  WIDTH_DIFF = 0;
  HEIGHT_DIFF = 0;
  GRAPH_EDGE_DIFF = 100;
  POLY_EDGE_DIFF = 150;

  string inFileName = "",trainingFile = "",outputFileName = "MatchResult.txt";
  bool testFlag = true;
  if (argc < 3)
  {
    cout << "help:-in testFileName" << endl;
    cout << "help:-txt trainingFileName" << endl;	
    cout << "help:-out outputFileName" << endl;
    cout << "help:[-train]" << endl;
    cout << "Example:fpm2.exe -in MX_BenchMark1.oas -txt1 training1.txt -txt2 training2.txt -out MatchResult.txt -train " << endl;
    return 0;
  }
  for(int i = 1; i < argc; i++)
  {
    //txt input
    if (strcmp(argv[i], "-txt") == 0)
    {
       trainingFile = argv[++i];   
       cout<<"iui: "<<trainingFile<<endl;
    }
  
    if (strcmp(argv[i], "-in") == 0)
    {
      inFileName = argv[++i];
      cout<<"inFile: "<<inFileName<<endl;
    }
    if (strcmp(argv[i], "-S1D") == 0)
    {
      S1_DISTANCE = atoi(argv[++i]);
      cout<<"S1_DISTANCE: "<<S1_DISTANCE<<endl;
    }

    if (strcmp(argv[i], "-poly") == 0)
    {
      POLY_EDGE_DIFF = atoi(argv[++i]);
      cout<<"POLY_EDGE_DIFF: "<<POLY_EDGE_DIFF<<endl;
    }

    if (strcmp(argv[i], "-out") == 0)
    {
    	outputFileName = argv[++i];
    }
    if (strcmp(argv[i], "-train") == 0)
    {
        testFlag = false;
    }
    if (strcmp(argv[i], "-help") == 0)
    {
      cout << "help:-in testFileName" << endl;
      cout << "help:-txt trainingFileName" << endl;	     
      cout << "help:[-train]" << endl;
      cout << "Example:fpm2.exe -in MX_BenchMark1.oas -txt training1.txt -out MatchResult.txt -train " << endl;
      return 0;
    }
  }
  
  if (inFileName == "") {
  	cerr << "Error in arguments:Need test file" << endl;
  	cerr << "usage:-in testFileName" << endl;
  	exit(-1);
  }
  if (trainingFile == "") {
  	cerr << "Error in arguments:Need training file" << endl;
  	cerr << "usage:-txt trainingFileName" << endl;
  	exit(-1);
  }

  
//  FPMLayout fl;
//  fl.clear();
//  DataReader::ReadOASIS("Testcase/MX_Benchmark4_clip.oas",fl);
//  cout << "---check:" << fl.checkOverlapOnFrame() << "---" << endl;
//  return 0;
  
  //read training set
  string tempFile = "";

  vector<string> trainingSet;

  ifstream infile(trainingFile.c_str());
  while(infile >> tempFile)
  {
     cout<<tempFile<<endl;
     trainingSet.push_back(tempFile);
  }
  infile.close();

  
  FPMLayout tempLayout;

  vector<FPMLayout> layoutVector;
//  vector<FPMLayout> good_layoutVector;

  for(int k=0;k<trainingSet.size();k++)
  {
     cout<<trainingSet[k]<<endl;
     tempLayout.clear();
     DataReader::ReadOASIS(trainingSet[k],tempLayout);
     layoutVector.push_back(tempLayout);
  }
  /*
  for(int k=0;k<trainingSet.size();k++)
  {
     cout<<trainingSet[k]<<endl;
     tempLayout.clear();
     DataReader::ReadOASIS(trainingSet[k],tempLayout);
     good_layoutVector.push_back(tempLayout);
  }
  */
  
  FPMLayout testlayout;
  testlayout.setOutputFileName(outputFileName);
  
  testlayout.clear();
  DataReader::ReadOASIS(inFileName, testlayout);
  
  //construct pattern vector
  std::vector<FPMPattern> record_patterns;
  std::vector<FPMPattern> good_patterns;
  for(int i = 0;i < layoutVector.size();++i)
  {
     layoutVector[i].createPatterns();
     
     printf("Layout %d: Before pattern rotation and symmetry, total %d patterns\n", i, layoutVector[i].getPatterns().size());
     //testlayout.rotateAllPatterns(layoutVector[i].getPatterns());
     //printf("Layout %d: After pattern rotation and symmetry, total %d patterns\n", i, layoutVector[i].getPatterns().size());
     record_patterns.insert(record_patterns.begin(),layoutVector[i].getPatterns().begin(),layoutVector[i].getPatterns().end());
     
     cout << "record Pattern size-->" << record_patterns.size() <<endl;
  }
  /*
  for(int i=0;i<good_layoutVector.size();++i)
  {
    good_layoutVector[i].createGoodPatterns();
    good_patterns.insert(good_patterns.begin(),good_layoutVector[i].getPatterns().begin(),good_layoutVector[i].getPatterns().end()); 
  }  
  cout<<"good pattern size-->"<<good_patterns.size()<< endl;
  */
  
  //testlayout.createSubLayouts();
  testlayout.createSubLayouts(testFlag);
  testlayout.test(record_patterns,true);
  /*
  cout<<good_patterns.size();
  testlayout.test(good_patterns,false);
  cout<<"good point size: "<<testlayout.getgoodPoint().size()<<endl;
  */
  
  cout<<"bad point size: "<<testlayout.getbadPoint().size()<<endl;
  
  ofstream f(outputFileName.c_str());
  testlayout.FinalResult(testlayout.getbadPoint(),testlayout.getgoodPoint(),f);
  f.close();
  return 0;
}
