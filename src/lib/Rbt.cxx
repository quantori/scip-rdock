/***********************************************************************
* The rDock program was developed from 1998 - 2006 by the software team 
* at RiboTargets (subsequently Vernalis (R&D) Ltd).
* In 2006, the software was licensed to the University of York for 
* maintenance and distribution.
* In 2012, Vernalis and the University of York agreed to release the 
* program as Open Source software.
* This version is licensed under GNU-LGPL version 3.0 with support from
* the University of Barcelona.
* http://rdock.sourceforge.net/
***********************************************************************/

//Misc non-member functions in Rbt namespace

#include <unistd.h> //For POSIX getcwd
#include <stdlib.h> //For getenv
#include <algorithm> //For sort
#include <dirent.h> //For directory handling
#include <time.h> //For time functions
#include <limits.h> //For PATH_MAX
#include <fstream> //For ifstream
//#include <ios>
#include "Rbt.h"
#include "RbtResources.h"
#include "RbtFileError.h"
using std::istream;

char *global_rbtroot = NULL;
char *global_rbthome = NULL;

//GetRbtRoot - returns value of RBT_ROOT env variable
RbtString Rbt::GetRbtRoot() {
  if (global_rbtroot != NULL)
    return global_rbtroot;
  char* szRbtRoot = getenv("RBT_ROOT");
  if (szRbtRoot != (char*) NULL) {
    return RbtString(szRbtRoot);
  }
  else {
    return GetCurrentDirectory();
  }
}

//DM 02 Aug 2000
//GetRbtHome - returns value of RBT_HOME env variable
//or HOME if RBT_HOME is not defined
//If HOME is undefined, returns current working directory
RbtString Rbt::GetRbtHome() {
  if (global_rbthome != NULL)
    return global_rbthome;
  char* szRbtHome = getenv("RBT_HOME");
  if (szRbtHome != (char*) NULL) {
    return RbtString(szRbtHome);
  }
  else {
    szRbtHome = getenv("HOME");
    if (szRbtHome != (char*) NULL) {
      return RbtString(szRbtHome);
    }
    else {
      return GetCurrentDirectory();
    }
  }
}
  
//Rbt::GetCopyright - returns legalese statement
RbtString Rbt::GetCopyright() {return IDS_COPYRIGHT;}
//Rbt::GetVersion - returns current library version
RbtString Rbt::GetVersion() {return IDS_VERSION;}
//GetBuildNum - returns current library build number
RbtString Rbt::GetBuild() {return IDS_BUILDNUM;}
//GetProduct - returns library product name
RbtString Rbt::GetProduct() {return IDS_PRODUCT;}
//GetTime - returns current time as an RbtString
RbtString Rbt::GetTime()
{
  time_t t = ::time(NULL);//Get time in seconds since 1970
  tm* pLocalTime = ::localtime(&t);//Convert to local time struct
  return RbtString(::asctime(pLocalTime));//Convert to ascii string
}
//GetCurrentDirectory - returns current working directory
RbtString Rbt::GetCurrentDirectory()
{
  RbtString strCwd(".");
  char* szCwd = new char[PATH_MAX+1];//Allocate a temp char* array
  if (::getcwd(szCwd,PATH_MAX) != (char*)NULL) {//Get the cwd
    strCwd = szCwd;
    //strCwd += "/";
  }
  delete [] szCwd; //Delete the temp array
  return strCwd;
}

//Rbt::GetRbtDirName
//Returns the full path to a subdirectory in the rDock directory structure
//
//For example, if RBT_ROOT environment variable is ~dave/ribodev/molmod/ribodev
//then GetRbtDirName("data") would return ~dave/ribodev/molmod/ribodev/data/
//
RbtString Rbt::GetRbtDirName(const RbtString& strSubDir)
{
  RbtString strRbtDir = GetRbtRoot();
  if (strSubDir.size() > 0) {
    strRbtDir += "/";
    strRbtDir += strSubDir;
  }
  return strRbtDir;
}

//Rbt::GetRbtFileName
//DM 17 Dec 1998 - slightly different behaviour
//First check if the file exists in the CWD, if so return this path
//Next check RBT_HOME directory, if so return this path
//Finally, return the path to the file in the rDock directory structure
//(without checking if the file is actually present)
RbtString Rbt::GetRbtFileName(const RbtString& strSubdir, const RbtString& strFile)
{
  //First see if the file exists in the current directory
  RbtString strFullPathToFile(strFile);
  //Just open it, don't try and parse it (after all, we don't know what format the file is)
  std::ifstream fileIn(strFullPathToFile.c_str(),std::ios_base::in);
  if (fileIn) {
    fileIn.close();
    return strFullPathToFile;
  }
  else {
    //DM 02 Aug 200 - check RBT_HOME directory
    strFullPathToFile = GetRbtHome()+"/"+strFile;
    //DM 27 Apr 2005 - under gcc 3.4.3 there are problems reusing the same ifstream object
    //after the first "file open" fails
    //fileIn.open(strFullPathToFile.c_str(),std::ios_base::in);
    std::ifstream fileIn2(strFullPathToFile.c_str(),std::ios_base::in);
    if (fileIn2) {
      fileIn2.close();
      return strFullPathToFile;
    }
    else {
      return GetRbtDirName(strSubdir)+"/"+strFile;
    }
  }
}

//GetFileType
//Returns the string following the last "." in the file name.
//e.g. GetFileType("receptor.psf") would return "psf"
//If no "." is present, returns the whole file name
RbtString Rbt::GetFileType(const RbtString& strFile)
{
  return strFile.substr(strFile.rfind(".")+1);
}

//Rbt::GetDirList
//Returns a list of files in a directory (strDir) whose names begin with strFilePrefix (optional)
//and whose type is strFileType (optional, as returned by GetFileType)
RbtStringList Rbt::GetDirList(const RbtString& strDir, const RbtString& strFilePrefix, const RbtString& strFileType)
{
  RbtStringList dirList;
  RbtBool bMatchPrefix = (strFilePrefix.size() > 0);//Check if we need to match on file prefix
  RbtBool bMatchType = (strFileType.size() > 0);//Check if we need to match on file type (suffix)

  DIR* pDir = opendir(strDir.c_str());//Open directory
  if (pDir != NULL) {

    //DM 6 Dec 1999 - dirent_t appears to be SGI MIPSPro specific
    //At least on Linux g++, it is called dirent
#ifdef __sgi
    dirent_t* pEntry;
#else
    dirent* pEntry;
#endif

    while ( (pEntry = readdir(pDir)) != NULL ) {//Read each entry
      RbtString strFile = pEntry->d_name;
      if ( (strFile == ".") || (strFile == "..") ) continue;//Don't need to consider . and ..
      RbtBool bMatch = true;
      if (bMatchPrefix && (strFile.find(strFilePrefix) != 0) )//Eliminate those that don't match the prefix
    bMatch = false;
      if (bMatchType && (GetFileType(strFile) != strFileType))//Eliminate those that don't match the type
    bMatch = false;
      if (bMatch)//Only store the hits
    dirList.push_back(strFile);
    }
    closedir(pDir);
  }

  //Sort the file list into alphabetical order
  std::sort(dirList.begin(),dirList.end());

  return dirList;
}

//Converts (comma)-delimited string of segment names to segment map
RbtSegmentMap Rbt::ConvertStringToSegmentMap(const RbtString& strSegments, const RbtString& strDelimiter)
{
#ifdef _DEBUG
  //cout << "ConvertStringToSegmentMap: " << strSegments << " delimiter=" << strDelimiter << endl;
#endif //_DEBUG

  RbtString::size_type nDelimiterSize = strDelimiter.size();
  RbtSegmentMap segmentMap;

  //Check for null string or null delimiter
  if ( (strSegments.size() > 0) && (nDelimiterSize > 0) ) {
    RbtString::size_type iBegin = 0;//indicies into string (sort-of iterators)
    RbtString::size_type iEnd;
    //Not often a do..while loop is used, but in this case it's what we want
    //Even if no delimiter is present, we still need to extract the whole string
    do {
      iEnd = strSegments.find(strDelimiter,iBegin);
#ifdef _DEBUG
      //cout << strSegments.substr(iBegin, iEnd-iBegin) << endl;
#endif //_DEBUG
      segmentMap[strSegments.substr(iBegin, iEnd-iBegin)] = 0;
      iBegin = iEnd+nDelimiterSize;
    } while (iEnd != RbtString::npos);//This is the check for delimiter not found
  }

  return segmentMap;
}

//Converts segment map to (comma)-delimited string of segment names
RbtString Rbt::ConvertSegmentMapToString(const RbtSegmentMap& segmentMap, const RbtString& strDelimiter)
{
  RbtString strSegments;

  //Check for empty segment map
  if (segmentMap.size() > 0) {
    RbtSegmentMapConstIter iter = segmentMap.begin();
    strSegments += (*iter++).first; //Add first string
    //Now loop over remaining entries, adding delimiter before each string
    while (iter != segmentMap.end()) {
      strSegments += strDelimiter;
      strSegments += (*iter++).first;
    }
  }
  return strSegments;
}

//Returns a segment map containing the members of map1 which are not in map2
//I know, should really be a template so as to be more universal...one day maybe.
//Or maybe there is already an STL algorithm for doing this.
RbtSegmentMap Rbt::SegmentDiffMap(const RbtSegmentMap& map1, const RbtSegmentMap& map2)
{
  RbtSegmentMap map3 = map1;//Init return value to map1
  for (RbtSegmentMapConstIter iter = map2.begin(); iter != map2.end(); iter++)
    map3.erase((*iter).first);//Now delete everything in map2
  return map3;
}

//DM 30 Mar 1999
//Converts (comma)-delimited string to string list (similar to ConvertStringToSegmentMap, but returns list not map)
RbtStringList Rbt::ConvertDelimitedStringToList(const RbtString& strValues, const RbtString& strDelimiter)
{
  RbtString::size_type nDelimiterSize = strDelimiter.size();
  RbtStringList listOfValues;

  //Check for null string or null delimiter
  if ( (strValues.size() > 0) && (nDelimiterSize > 0) ) {
    RbtString::size_type iBegin = 0;//indicies into string (sort-of iterators)
    RbtString::size_type iEnd;
    //Not often a do..while loop is used, but in this case it's what we want
    //Even if no delimiter is present, we still need to extract the whole string
    do {
      iEnd = strValues.find(strDelimiter,iBegin);
      RbtString strValue = strValues.substr(iBegin, iEnd-iBegin);
      listOfValues.push_back(strValue);
      iBegin = iEnd+nDelimiterSize;
    } while (iEnd != RbtString::npos);//This is the check for delimiter not found
  }
  return listOfValues;
}

//Converts string list to (comma)-delimited string (inverse of above)
RbtString Rbt::ConvertListToDelimitedString(const RbtStringList& listOfValues, const RbtString& strDelimiter)
{
  RbtString strValues;

  //Check for empty string list
  if (!listOfValues.empty()) {
    RbtStringListConstIter iter = listOfValues.begin();
    strValues += *iter++; //Add first string
    //Now loop over remaining entries, adding delimiter before each string
    while (iter != listOfValues.end()) {
      strValues += strDelimiter;
      strValues += *iter++;
    }
  }
  return strValues;
}

////////////////////////////////////////////////////////////////
//I/O ROUTINES
//
//PrintStdHeader
//Print a standard header to an output stream (for log files etc)
//Contains copyright info, library version, date, time etc
//DM 19 Feb 1999 - include executable information
std::ostream& Rbt::PrintStdHeader(std::ostream& s, const RbtString& strExecutable)
{
  s << "***********************************************" << std::endl;
  s << GetCopyright() << std::endl;
  if (!strExecutable.empty())
    s << "Executable:\t" << strExecutable << std::endl;
  s << "Library:\t" << GetProduct() << "/" << GetVersion()
    << "/" << GetBuild() << std::endl;
  s << "RBT_ROOT:\t" << GetRbtRoot() << std::endl;
  s << "RBT_HOME:\t" << GetRbtHome() << std::endl;
  s << "Current dir:\t" << GetCurrentDirectory() << std::endl;
  s << "Date:\t\t" << GetTime();
  s << "***********************************************" << std::endl;
  return s;
}

//Helper functions to read/write chars from iostreams
//Throws error if stream state is not Good() before and after the read/write
//It appears the STL ios_base exception throwing is not yet implemented
//at least on RedHat 6.1, so this is a temporary workaround (yeah right)
void Rbt::WriteWithThrow(std::ostream& ostr, const char* p, streamsize n) throw (RbtError) {
  if (!ostr)
        throw RbtFileWriteError(_WHERE_,"Error writing to output stream");
  ostr.write(p,n);
  if (!ostr)
        throw RbtFileWriteError(_WHERE_,"Error writing to output stream");
}

void Rbt::ReadWithThrow(std::istream& istr, char* p, streamsize n) throw (RbtError) {
  if (!istr)
        throw RbtFileReadError(_WHERE_,"Error reading from input stream");
  istr.read(p,n);
  if (!istr)
        throw RbtFileReadError(_WHERE_,"Error reading from input stream");
}

// Used to read RbtCoord. The separator between x y z should be a
// ',', but this takes care of small mistakes, reading any white
// space or commas there is between each variable.
// If necessary, it can be modified to accept the ',' as a
// parameter to be able to use it when other separators are
// needed. Also, it should be possible to implemente it as a
// manipulator of istream. 
istream& eatSeps(istream& is)
{
    char c;
    while (is.get(c))
    {
        if (!isspace(c) && (c != ','))
        {
            is.putback(c);
            break;
        }
    }
    return is;
}


/***********************************************************************
* The rDock program was developed from 1998 - 2006 by the software team
* at RiboTargets (subsequently Vernalis (R&D) Ltd).
* In 2006, the software was licensed to the University of York for
* maintenance and distribution.
* In 2012, Vernalis and the University of York agreed to release the
* program as Open Source software.
* This version is licensed under GNU-LGPL version 3.0 with support from
* the University of Barcelona.
* http://rdock.sourceforge.net/
***********************************************************************/

//Main docking application
#include <iomanip>
using std::setw;

#include <popt.h>		// for command-line parsing
#include <errno.h>

#include "RbtBiMolWorkSpace.h"
#include "RbtMdlFileSource.h"
#include "RbtMdlFileSink.h"
#include "RbtCrdFileSink.h"
#include "RbtParameterFileSource.h"
#include "RbtPRMFactory.h"
#include "RbtSFFactory.h"
#include "RbtTransformFactory.h"
#include "RbtRand.h"
#include <cstring>
#include "RbtModelError.h"
#include "RbtDockingError.h"
#include "RbtLigandError.h"
#include "RbtFilter.h"
#include "RbtSFRequest.h"
#include "RbtFileError.h"

const RbtString EXEVERSION = " ($Id: //depot/dev/client3/rdock/2021.1/src/exe/rbdock.cxx#4 $)";
//Section name in docking prm file containing scoring function definition
const RbtString _ROOT_SF = "SCORE";
const RbtString _RESTRAINT_SF = "RESTR";
const RbtString _ROOT_TRANSFORM = "DOCK";


void PrintUsage(void)
{
    cout << endl << "Usage:" << endl;
    cout << "rbdock -i <sdFile> -o <outputRoot> -r <recepPrmFile> -p <protoPrmFile> [-n <nRuns>] [-ap] [-an] [-allH]" << endl;
    cout << "       [-t <targetScore|targetFilterFile>] [-c] [-T <traceLevel>] [-s <rndSeed>]" << endl;
    cout << endl << "Options:\t-i <sdFile> - input ligand SD file" << endl;
    cout << "\t\t-o <outputRoot> - root name for output file(s)" << endl;
    cout << "\t\t-r <recepPrmFile> - receptor parameter file " << endl;
    cout << "\t\t-p <protoPrmFile> - docking protocol parameter file" << endl;
    cout << "\t\t-n <nRuns> - number of runs/ligand (default=1)" << endl;
    cout << "\t\t-ap - protonate all neutral amines, guanidines, imidazoles (default=disabled)" << endl;
    cout << "\t\t-an - deprotonate all carboxylic, sulphur and phosphorous acid groups (default=disabled)" << endl;
    cout << "\t\t-allH - read all hydrogens present (default=polar hydrogens only)" << endl;
    cout << "\t\t-t - score threshold OR filter file name" << endl;
    cout << "\t\t-c - continue if score threshold is met (use with -t <targetScore>, default=terminate ligand)" << endl;
    cout << "\t\t-T <traceLevel> - controls output level for debugging (0 = minimal, >0 = more verbose)" << endl;
    cout << "\t\t-s <rndSeed> - random number seed (default=from sys clock)" << endl;
}

/////////////////////////////////////////////////////////////////////
// MAIN PROGRAM STARTS HERE
/////////////////////////////////////////////////////////////////////

typedef struct{
    char* errorMessage;
    char* data;
    int error;
} RbtResult;

extern "C" {

void freemem(RbtResult *res)
{
    free(res->errorMessage);
    free(res->data);
    free(res);
}
    
#include <stdlib.h>
__attribute__ ((visibility ("default")))
void* runRdock(char *rbtroot, char *rbthome, int argc, char* argv[]){
    global_rbtroot = rbtroot;
    global_rbthome = rbthome;
    RbtResult *r = (RbtResult *)malloc(sizeof(RbtResult));
    std::stringbuf errmsgbuf;
    std::ostream errmsgstream (&errmsgbuf);
    cout.setf(ios_base::left,ios_base::adjustfield);
    std::string conf = "";
    std::string res = "";
    //char* cstr = new char[200];
    //Strip off the path to the executable, leaving just the file name
    RbtString strExeName(argv[0]);
    RbtString::size_type i = strExeName.rfind("/");

    r->error = 0;
    
    if (i != RbtString::npos)
        strExeName.erase(0,i+1);

    //Print a standard header
    Rbt::PrintStdHeader(cout,strExeName+EXEVERSION);

    //Command line arguments and default values
    RbtString	strLigandMdlFile;
    RbtBool		bOutput(false);
    RbtString	strRunName;
    RbtString	strReceptorPrmFile;		//Receptor param file
    RbtString	strParamFile;			//Docking run param file
    RbtString       strFilterFile; // Filter file
    RbtInt		nDockingRuns(0);//Init to zero, so can detect later whether user explictly typed -n

    //Params for target score
    RbtBool		bTarget(false);
    RbtBool		bStop(true);			//DM 25 May 2001 - if true, stop once target is met
    RbtBool         bDockingRuns(false); // is argument -n present?
    RbtDouble	dTargetScore(0.0);
    RbtBool         bFilter(false);

    RbtBool		bPosIonise(false);
    RbtBool		bNegIonise(false);
    RbtBool		bImplH(true);			//if true, read only polar hydrogens from SD file, else read all H's present
    RbtBool		bSeed(false);			//Random number seed (default = from system clock)
    RbtInt		nSeed(0);
    RbtBool         bTrace(false);
    RbtInt		iTrace(0);//Trace level, for debugging

    // variables for popt command-line parsing
    char 			c;						// for argument parsing
    poptContext		optCon;					// ditto

    char 			*inputFile=NULL;		// will be 'strLigandMdlFile'
    char 			*outputFile=NULL;		// will be 'strRunName'
    char 			*receptorFile=NULL;		// will be 'strReceptorPrmFile'
    char 			*protocolFile=NULL;		// will be 'strParamFile'
    char 			*strTargetScr=NULL;		// will be 'dTargetScore'
    struct poptOption optionsTable[] = {	// command line options
            {"input",		'i',POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,&inputFile,   'i',"input file"},
            {"output",		'o',POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,&outputFile,  'o',"output file"},
            {"receptor",	'r',POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,&receptorFile,'r',"receptor file"},
            {"protocol",	'p',POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,&protocolFile,'p',"protocol file"},
            {"runs",		'n',POPT_ARG_INT   |POPT_ARGFLAG_ONEDASH,&nDockingRuns,'n',"number of runs"},
            {"trace",		'T',POPT_ARG_INT   |POPT_ARGFLAG_ONEDASH,&iTrace,'T',"trace level for debugging"},
            {"seed",		's',POPT_ARG_INT   |POPT_ARGFLAG_ONEDASH,&nSeed,       's',"random seed"},
            {"ap",			'P',POPT_ARG_NONE  |POPT_ARGFLAG_ONEDASH,0,            'P',"protonate groups"},
            {"an",			'D',POPT_ARG_NONE  |POPT_ARGFLAG_ONEDASH,0,            'D',"DEprotonate groups"},
            {"allH",        'H',POPT_ARG_NONE  |POPT_ARGFLAG_ONEDASH,0,            'H',"read all Hs"},
            {"target",      't',POPT_ARG_STRING|POPT_ARGFLAG_ONEDASH,&strTargetScr,'t',"target score"},
            {"cont",        'C',POPT_ARG_NONE  |POPT_ARGFLAG_ONEDASH,0,            'C',"continue even if target met"},
            POPT_AUTOHELP
                    {NULL,0,0,NULL,0}
    };

    optCon	= poptGetContext(NULL, argc, argv, optionsTable, 0);
    poptSetOtherOptionHelp(optCon, "-r<receptor.prm> -p<protocol.prm> -i<infile> -o<outfile> [options]");
    //Brief help message

    RbtDouble val(0.0);
    while ( (c=poptGetNextOpt(optCon)) != (char)(-1)) {
        switch(c) {
            case 'P':	// protonate
                bPosIonise = true;
                break;
            case 'D':	// deprotonate
                bNegIonise = true;
                break;
            case 'H':	// all-H model
                bImplH = false;
                break;
            case 'C':
                bStop = false;
                break;
            case 't':
                // If str can be translated to an integer, I assume is a
                // threshold. Otherwise, I assume is the filter file name
                char *error;
                errno = 0;
                val = strtod(strTargetScr, &error);
                if (!errno && !*error)  // Is it a number?
                {
                    dTargetScore = val;
                    bTarget = true;
                }
                else // Assume is the filter file name
                {
                    bFilter = true;
                    strFilterFile = strTargetScr;
                }
                break;
            case 's':
                bSeed = true;
                break;
            case 'T':
                bTrace = true;
                break;
            default:
                break;
        }
    }
    cout << endl;
    poptFreeContext(optCon);

    // print out arguments
    // input ligand file, receptor and parameter is compulsory
    cout << endl << "Command line args:" << endl;
    if(!inputFile || !receptorFile || !protocolFile) { // if any of them is missing
        //SRC poptPrintUsage(optCon, stderr, 0);
        PrintUsage();
        exit(1);
    } else {
        strLigandMdlFile	= inputFile;
        strReceptorPrmFile	= receptorFile;
        strParamFile		= protocolFile;
        cout << " -i " << strLigandMdlFile		<< endl;
        cout << " -r " << strReceptorPrmFile	<< endl;
        cout << " -p " << strParamFile			<< endl;
    }
    // output is not that important but good to have
    if(outputFile) {
        strRunName	= outputFile;
        bOutput = true;
        cout << " -o " << strRunName << endl;
    } else {
        cout << "WARNING: output file name is missing."<< endl;
    }
    // docking runs
    if(nDockingRuns >= 1) {//User typed -n explicitly, so set bDockingRuns to true
        bDockingRuns = true;
        cout << " -n " << nDockingRuns << endl;
    } else {
        nDockingRuns = 1;//User didn't type -n explicitly, so fall back to the default of n=1
        cout << " -n " << nDockingRuns << " (default) " <<endl;
    }
    if(bSeed)		// random seed (if provided)
        cout << " -s " << nSeed << endl;
    if(bTrace)		// random seed (if provided)
        cout << " -T " << iTrace << endl;
    if(bPosIonise)	// protonate
        cout << " -ap " << endl;
    if(bNegIonise)	// deprotonate
        cout << " -an " << endl;
    if(!bImplH)		// all-H
        cout << " -allH " << endl;
    if(!bStop)		// stop after target
        cout << " -cont " << endl;
    if(bTarget)
        cout << " -t " << dTargetScore << endl;

    //BGD 26 Feb 2003 - Create filters to simulate old rbdock
    //behaviour
    ostrstream strFilter;
    if (!bFilter)
    {
        if (bTarget) // -t<TS>
        {
            if (!bDockingRuns) // -t<TS> only
            {
                strFilter << "0 1 - SCORE.INTER " << dTargetScore << endl;
            }
            else  // -t<TS> -n<N> need to check if -cont present
                // for all other cases it doesn't matter
            if (!bStop) // -t<TS> -n<N> -cont
            {
                strFilter << "1 if - SCORE.NRUNS " << (nDockingRuns - 1)
                          << " 0.0 -1.0,\n1 - SCORE.INTER " << dTargetScore << endl;
            }
            else // -t<TS> -n<N>
            {
                strFilter << "1 if - " << dTargetScore << " SCORE.INTER 0.0 "
                          << "if - SCORE.NRUNS " << (nDockingRuns - 1)
                          << " 0.0 -1.0,\n1 - SCORE.INTER " << dTargetScore << endl;
            }
        } // no target score, no filter
        else if (bDockingRuns) // -n<N>
        {
            strFilter << "1 if - SCORE.NRUNS " << (nDockingRuns - 1)
                      << " 0.0 -1.0,\n0";
        }
        else // no -t no -n
        {
            strFilter << "0 0\n";
        }
    }

    //DM 20 Apr 1999 - set the auto-ionise flags
    if (bPosIonise)
        cout << "Automatically protonating positive ionisable groups (amines, imidazoles, guanidines)" << endl;
    if (bNegIonise)
        cout << "Automatically deprotonating negative ionisable groups (carboxylic acids, phosphates, sulphates, sulphonates)" << endl;
    if (bImplH)
        cout << "Reading polar hydrogens only from ligand SD file" << endl;
    else
        cout << "Reading all hydrogens from ligand SD file" << endl;

    if (bTarget) {
        cout << endl << "Lower target intermolecular score = " << dTargetScore << endl;
    }

    try {
        //Create a bimolecular workspace
        RbtBiMolWorkSpacePtr spWS(new RbtBiMolWorkSpace());
        //Set the workspace name to the root of the receptor .prm filename
        RbtStringList componentList = Rbt::ConvertDelimitedStringToList(strReceptorPrmFile,".");
        RbtString wsName = componentList.front();
        spWS->SetName(wsName);

        //Read the docking protocol parameter file
        RbtParameterFileSourcePtr spParamSource(new RbtParameterFileSource(Rbt::GetRbtFileName("data/scripts",strParamFile)));
        //Read the receptor parameter file
        RbtParameterFileSourcePtr spRecepPrmSource(new RbtParameterFileSource(Rbt::GetRbtFileName("data/receptors",strReceptorPrmFile)));
        cout << endl << "DOCKING PROTOCOL:" << endl << spParamSource->GetFileName() << endl << spParamSource->GetTitle() << endl;
        cout << endl << "RECEPTOR:" << endl << spRecepPrmSource->GetFileName() << endl << spRecepPrmSource->GetTitle() << endl;

        //Create the scoring function from the SCORE section of the docking protocol prm file
        //Format is:
        //SECTION SCORE
        //    INTER    RbtInterSF.prm
        //    INTRA RbtIntraSF.prm
        //END_SECTION
        //
        //Notes:
        //Section name must be SCORE. This is also the name of the root SF aggregate
        //An aggregate is created for each parameter in the section.
        //Parameter name becomes the name of the subaggregate (e.g. SCORE.INTER)
        //Parameter value is the file name for the subaggregate definition
        //Default directory is $RBT_ROOT/data/sf
        RbtSFFactoryPtr spSFFactory(new RbtSFFactory());//Factory class for scoring functions
        RbtSFAggPtr spSF(new RbtSFAgg(_ROOT_SF));//Root SF aggregate
        spParamSource->SetSection(_ROOT_SF);
        RbtStringList sfList(spParamSource->GetParameterList());
        //Loop over all parameters in the SCORE section
        for (RbtStringListConstIter sfIter = sfList.begin(); sfIter != sfList.end(); sfIter++) {
            //sfFile = file name for scoring function subaggregate
            RbtString sfFile(Rbt::GetRbtFileName("data/sf",spParamSource->GetParameterValueAsString(*sfIter)));
            RbtParameterFileSourcePtr spSFSource(new RbtParameterFileSource(sfFile));
            //Create and add the subaggregate
            spSF->Add(spSFFactory->CreateAggFromFile(spSFSource,*sfIter));
        }

        //Add the RESTRAINT subaggregate scoring function from any SF definitions in the receptor prm file
        spSF->Add(spSFFactory->CreateAggFromFile(spRecepPrmSource,_RESTRAINT_SF));

        //Create the docking transform aggregate from the transform definitions in the docking prm file
        RbtTransformFactoryPtr spTransformFactory(new RbtTransformFactory());
        spParamSource->SetSection();
        RbtTransformAggPtr spTransform(spTransformFactory->CreateAggFromFile(spParamSource,_ROOT_TRANSFORM));

        //Override the TRACE levels for the scoring function and transform
        //Dump details to cout
        //Register the scoring function and the transform with the workspace
        if (bTrace) {
            RbtRequestPtr spTraceReq(new RbtSFSetParamRequest("TRACE",iTrace));
            spSF->HandleRequest(spTraceReq);
            spTransform->HandleRequest(spTraceReq);
        }
        if (iTrace > 0) {
            cout << endl << "SCORING FUNCTION DETAILS:" << endl << *spSF << endl;
            cout << endl << "SEARCH DETAILS:" << endl << *spTransform << endl;
        }
        spWS->SetSF(spSF);
        spWS->SetTransform(spTransform);

        //DM 18 May 1999
        //Variants describing the library version, exe version, parameter file, and current directory
        //Will be stored in the ligand SD files
        RbtVariant vLib(Rbt::GetProduct()+" ("+Rbt::GetVersion()+", Build"+Rbt::GetBuild()+")");
        RbtVariant vExe(strExeName+EXEVERSION);
        RbtVariant vRecep(spRecepPrmSource->GetFileName());
        RbtVariant vPrm(spParamSource->GetFileName());
        RbtVariant vDir(Rbt::GetCurrentDirectory());

        spRecepPrmSource->SetSection();
        //Read docking site from file and register with workspace
        RbtString strASFile = spWS->GetName()+".as";
        RbtString strInputFile = Rbt::GetRbtFileName("data/grids",strASFile);
        //DM 26 Sep 2000 - ios_base::binary is invalid with IRIX CC
#if defined(__sgi) && !defined(__GNUC__)
        ifstream istr(strInputFile.c_str(),ios_base::in);
#else
        ifstream istr(strInputFile.c_str(),ios_base::in|ios_base::binary);
#endif
        //DM 14 June 2006 - bug fix to one of the longest standing rDock issues
        //(the cryptic "Error reading from input stream" message, if cavity file was missing)
        if (!istr) {
            RbtString message = "Cavity file (" + strASFile + ") not found in current directory or $RBT_HOME";
            message += " - run rbcavity first";
            throw RbtFileReadError(_WHERE_,message);
        }
        RbtDockingSitePtr spDS(new RbtDockingSite(istr));
        istr.close();
        spWS->SetDockingSite(spDS);
        cout << endl << "DOCKING SITE" << endl << (*spDS) << endl;

        //Prepare the SD file sink for saving the docked conformations for each ligand
        //DM 3 Dec 1999 - replaced ostrstream with RbtString in determining SD file name
        //SRC 2014 moved here this block to allow WRITE_ERROR TRUE
        if (bOutput) {
            RbtMolecularFileSinkPtr spMdlFileSink(new RbtMdlFileSink(strRunName+".sd",RbtModelPtr()));
            spWS->SetSink(spMdlFileSink);
        }

        RbtPRMFactory prmFactory(spRecepPrmSource, spDS);
        prmFactory.SetTrace(iTrace);
        //Create the receptor model from the file names in the receptor parameter file
        RbtModelPtr spReceptor = prmFactory.CreateReceptor();
        spWS->SetReceptor(spReceptor);

        //Register any solvent
        RbtModelList solventList = prmFactory.CreateSolvent();
        spWS->SetSolvent(solventList);
        if (spWS->hasSolvent()) {
            RbtInt nSolvent = spWS->GetSolvent().size();
            cout << endl << nSolvent << " solvent molecules registered" << endl;
        }
        else {
            cout << endl << "No solvent" << endl;
        }

        //SRC 2014 removed sector bOutput from here to some blocks above, for WRITEERRORS TRUE

        //Seed the random number generator
        RbtRand& theRand = Rbt::GetRbtRand();//ref to random number generator
        if (bSeed) {
            theRand.Seed(nSeed);
        }

        //Create the filter object for controlling early termination of protocol
        RbtFilterPtr spfilter;
        if (bFilter) {
            spfilter = new RbtFilter(strFilterFile);
            if (bDockingRuns) {
                spfilter->SetMaxNRuns(nDockingRuns);
            }
        }
        else {
            spfilter = new RbtFilter(strFilter.str(), true);
        }
        if (bTrace) {
            RbtRequestPtr spTraceReq(new RbtSFSetParamRequest("TRACE",iTrace));
            spfilter->HandleRequest(spTraceReq);
        }

        //Register the Filter with the workspace
        spWS->SetFilter(spfilter);

        //MAIN LOOP OVER LIGAND RECORDS
        //DM 20 Apr 1999 - add explicit bPosIonise and bNegIonise flags to MdlFileSource constructor
        RbtMolecularFileSourcePtr spMdlFileSource(new RbtMdlFileSource(strLigandMdlFile,bPosIonise,bNegIonise,bImplH));
        for (RbtInt nRec=1; spMdlFileSource->FileStatusOK(); spMdlFileSource->NextRecord(), nRec++) {
            cout.setf(ios_base::left,ios_base::adjustfield);
            cout << endl
                 << "**************************************************" << endl
                 << "RECORD #" << nRec << endl;
            RbtError molStatus = spMdlFileSource->Status();
            if (!molStatus.isOK()) {
                cout << endl << molStatus << endl
                     << "************************************************" << endl;
                continue;
            }

            //DM 26 Jul 1999 - only read the largest segment (guaranteed to be called H)
            //BGD 07 Oct 2002 - catching errors created by the ligands,
            //so rbdock continues with the next one, instead of
            //completely stopping
            try
            {
                spMdlFileSource->SetSegmentFilterMap
                        (Rbt::ConvertStringToSegmentMap("H"));

                if (spMdlFileSource->isDataFieldPresent("Name"))
                    cout << "NAME:   " << spMdlFileSource->GetDataValue("Name") << endl;
                res = res + std::string(spMdlFileSource->GetDataValue("Name")) + " ";
                if (spMdlFileSource->isDataFieldPresent("REG_Number"))
                    cout << "REG_Num:" << spMdlFileSource->GetDataValue("REG_Number")
                         << endl;
                cout << setw(30) << "RANDOM_NUMBER_SEED:" << theRand.GetSeed() << endl;

                //Create and register the ligand model
                RbtModelPtr spLigand = prmFactory.CreateLigand(spMdlFileSource);
                RbtString strMolName = spLigand->GetName();
                spWS->SetLigand(spLigand);
                //Update any model coords from embedded chromosomes in the ligand file
                spWS->UpdateModelCoordsFromChromRecords(spMdlFileSource, iTrace);

                //DM 18 May 1999 - store run info in model data
                //Clear any previous Rbt.* data fields
                spLigand->ClearAllDataFields("Rbt.");
                spLigand->SetDataValue("Rbt.Library",vLib);
                spLigand->SetDataValue("Rbt.Executable",vExe);
                spLigand->SetDataValue("Rbt.Receptor",vRecep);
                spLigand->SetDataValue("Rbt.Parameter_File",vPrm);
                spLigand->SetDataValue("Rbt.Current_Directory",vDir);

                //DM 10 Dec 1999 - if in target mode, loop until target score is reached
                RbtBool bTargetMet = false;

                ////////////////////////////////////////////////////
                //MAIN LOOP OVER EACH SIMULATED ANNEALING RUN
                //Create a history file sink, just in case it's needed by any
                //of the transforms
                RbtInt iRun = 1;
                // need to check this here. The termination
                // filter is only run once at least
                // one docking run has been done.
                if (nDockingRuns < 1)
                    bTargetMet = true;
                while (!bTargetMet) {
                    //Catching errors with this specific run
                    try {
                        if (bOutput) {
                            ostrstream histr;
                            histr << strRunName << "_" << strMolName << nRec << "_his_"
                                  << iRun << ".sd" << ends;
                            RbtMolecularFileSinkPtr spHistoryFileSink
                                    (new RbtMdlFileSink(histr.str(),spLigand));
                            delete histr.str();
                            spWS->SetHistorySink(spHistoryFileSink);
                        }
                        spWS->Run(); //Dock!
                        RbtBool bterm = spfilter->Terminate();
                        RbtBool bwrite = spfilter->Write();
                        if (bterm)
                            bTargetMet = true;
                        if (bOutput && bwrite) {
                            spWS->Save();
                            RbtStringList lst = spWS->GetConf();
                            for(auto i:lst)
                                conf = conf + i + "\n";
                            cout << "Сonformation " << iRun << " " << "SCORE: " << spWS->Scores() << std::endl;
                            res = res + "ITERATION" + spWS->Scores() + " " + conf;

                            spWS->ClearCache();
                        }
                        iRun++;
                    }
                    catch (RbtDockingError& e) {
                        errmsgstream << e << endl;
                    }
                }
                //END OF MAIN LOOP OVER EACH SIMULATED ANNEALING RUN
                ////////////////////////////////////////////////////
            }
                //END OF TRY
            catch (RbtLigandError& e) {
                r->error = -1;
                errmsgstream << e << endl;
            }
        }
        //END OF MAIN LOOP OVER LIGAND RECORDS
        ////////////////////////////////////////////////////
        cout << endl << "END OF RUN" << endl;
        //    if (bOutput && flexRec) {
        //      RbtMolecularFileSinkPtr spRecepSink(new RbtCrdFileSink(strRunName+".crd",spReceptor));
        //      spRecepSink->Render();
        //    }
    }
    catch (RbtError& e) {
        r->error = -2;
        errmsgstream << e << endl;
    }
    catch (...) {
        r->error = -3;
        errmsgstream << "Unknown exception" << endl;
    }

    _RBTOBJECTCOUNTER_DUMP_(cout)
        
    r->errorMessage = strdup(errmsgbuf.str().c_str());
    r->data = strdup(res.c_str());
    return r;
}
}
