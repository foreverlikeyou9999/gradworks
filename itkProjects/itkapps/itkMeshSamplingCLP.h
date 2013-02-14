// This file was automatically generated by:
//  /tools/Slicer3/Slicer3-3.4.2-2010-01-06-linux-x86_64/bin/GenerateCLP --InputXML /home/ljoohwi/devel/src/itkapps/itkMeshSampling.xml --OutputCxx /home/ljoohwi/devel/build/itkapps/itkMeshSamplingCLP.h
//
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <map>

#include <itksys/ios/sstream>

#include "tclap/CmdLine.h"
#include "ModuleProcessInformation.h"

#ifdef WIN32
#define Module_EXPORT __declspec(dllexport)
#else
#define Module_EXPORT 
#endif

#if defined(main) && !defined(REGISTER_TEST)
// If main defined as a preprocessor symbol, redefine it to the expected entry point.
#undef main
#define main ModuleEntryPoint

extern "C" {
  Module_EXPORT char *GetXMLModuleDescription();
  Module_EXPORT int ModuleEntryPoint(int, char*[]);
}
#endif

extern "C" {
Module_EXPORT char XMLModuleDescription[] = 
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
"<executable>\n"
"  <category>Surface Model</category>\n"
"  <title>Sampling image values at corresponding vertex</title>\n"
"  <description/>\n"
"  <version>0.1</version>\n"
"  <documentation-url/>\n"
"  <license/>\n"
"  <acknowledgements/>\n"
"  <parameters>\n"
"    <label>Input</label>\n"
"    <description>Input mesh and image for value extraction</description>\n"
"    <image>\n"
"      <name>imageName</name>\n"
"      <label>Image</label>\n"
"      <channel>input</channel>\n"
"      <index>0</index>\n"
"      <description>Image which its value is extracted</description>\n"
"    </image>\n"
"    <file>\n"
"      <name>meshName</name>\n"
"      <label>Mesh</label>\n"
"      <channel>input</channel>\n"
"      <index>1</index>\n"
"      <description>Mesh file to extract vertex value</description>\n"
"    </file>\n"
"		<string-enumeration>\n"
"			<label>Interpolation</label>\n"
"			<description>Interpolation method for sampling</description>\n"
"			<name>interpolation</name>\n"
"			<default>nn</default>\n"
"			<flag>i</flag>\n"
"			<longflag>interpolation</longflag>\n"
"			<channel>input</channel>\n"
"			<element>nn</element>\n"
"			<element>linear</element>\n"
"		</string-enumeration>\n"
"    <file>\n"
"      <name>distanceVector</name>\n"
"      <label>Distance Vector</label>\n"
"      <flag>d</flag>\n"
"      <longflag>distanceVector</longflag>\n"
"      <channel>input</channel>\n"
"    </file>\n"
"    <double>\n"
"      <name>nanValue</name>\n"
"      <description>Specify a value for nil area</description>\n"
"      <label>NaN Value</label>\n"
"      <default>0</default>\n"
"      <flag>n</flag>\n"
"      <longflag>nan</longflag>\n"
"      <channel>input</channel>\n"
"    </double>\n"
"    <file>\n"
"      <name>attrfileName</name>\n"
"      <description>Attribute file name</description>\n"
"      <label>Attribute file name</label>\n"
"      <default>attribute.txt</default>\n"
"      <flag>a</flag>\n"
"      <longflag>attributeFile</longflag>\n"
"      <channel>input</channel>\n"
"    </file>\n"
"  </parameters>\n"
"</executable>\n"
"\n"
;

}

void
splitString (const std::string &text,
             const std::string &separators,
             std::vector<std::string> &words)
{
  const std::string::size_type n = text.length();
  std::string::size_type start = text.find_first_not_of(separators);
  while (start < n)
    {
    std::string::size_type stop = text.find_first_of(separators, start);
    if (stop > n) stop = n;
    words.push_back(text.substr(start, stop - start));
    start = text.find_first_not_of(separators, stop+1);
    }
}

void
splitFilenames (const std::string &text,
                std::vector<std::string> &words)
{
  const std::string::size_type n = text.length();
  bool quoted;
  std::string comma(",");
  std::string quote("\"");
  std::string::size_type start = text.find_first_not_of(comma);
  while (start < n)
    {
    quoted = false;
    std::string::size_type startq = text.find_first_of(quote, start);
    std::string::size_type stopq = text.find_first_of(quote, startq+1);
    std::string::size_type stop = text.find_first_of(comma, start);
    if (stop > n) stop = n;
    if (startq != std::string::npos && stopq != std::string::npos)
      {
      while (startq < stop && stop < stopq && stop != n)
         {
         quoted = true;
         stop = text.find_first_of(comma, stop+1);
         if (stop > n) stop = n;
         }
      }
    if (!quoted)
      {
      words.push_back(text.substr(start, stop - start));
      }
    else
      {
      words.push_back(text.substr(start+1, stop - start-2));
      }
    start = text.find_first_not_of(comma, stop+1);
    }
}

char *GetXMLModuleDescription()
{
   return XMLModuleDescription;
}

#define GENERATE_LOGO
#define GENERATE_XML \
  if (argc >= 2 && (strcmp(argv[1],"--xml") == 0)) \
    { \
    std::cout << GetXMLModuleDescription(); \
    return EXIT_SUCCESS; \
    }
#define GENERATE_TCLAP \
    std::string imageName; \
    std::string meshName; \
    std::string interpolation = "nn"; \
    std::vector<std::string> interpolationAllowed; \
    interpolationAllowed.push_back("nn");  \
    interpolationAllowed.push_back("linear");  \
    TCLAP::ValuesConstraint<std::string> interpolationAllowedVals (interpolationAllowed);  \
    std::string distanceVector; \
    double nanValue = 0; \
    std::string attrfileName = "attribute.txt"; \
    bool echoSwitch = false; \
    bool xmlSwitch = false; \
    std::string processInformationAddressString = "0"; \
    std::string fullDescription("Description: "); \
    fullDescription += ""; \
    if (!std::string("").empty()) \
      { \
      fullDescription += "\nAuthor(s): "; \
      } \
    if (!std::string("").empty()) \
      { \
      fullDescription += "\nAcknowledgements: "; \
      } \
    TCLAP::CmdLine commandLine (fullDescription, \
       ' ', \
      "0.1" ); \
 \
      itksys_ios::ostringstream msg; \
    msg.str("");msg << "Image which its value is extracted";    TCLAP::UnlabeledValueArg<std::string> imageNameArg("imageName", msg.str(), 1, imageName, "std::string", commandLine); \
 \
    msg.str("");msg << "Mesh file to extract vertex value";    TCLAP::UnlabeledValueArg<std::string> meshNameArg("meshName", msg.str(), 1, meshName, "std::string", commandLine); \
 \
    msg.str("");msg << "Interpolation method for sampling (default: " << interpolation << ")"; \
    TCLAP::ValueArg<std::string> interpolationArg("i", "interpolation", msg.str(), 0, interpolation, &interpolationAllowedVals, commandLine); \
 \
    msg.str("");msg << "";    TCLAP::ValueArg<std::string > distanceVectorArg("d", "distanceVector", msg.str(), 0, distanceVector, "std::string", commandLine); \
 \
    msg.str("");msg << "Specify a value for nil area (default: " << nanValue << ")"; \
    TCLAP::ValueArg<double > nanValueArg("n", "nan", msg.str(), 0, nanValue, "double", commandLine); \
 \
    msg.str("");msg << "Attribute file name (default: " << attrfileName << ")"; \
    TCLAP::ValueArg<std::string > attrfileNameArg("a", "attributeFile", msg.str(), 0, attrfileName, "std::string", commandLine); \
 \
    msg.str("");msg << "Echo the command line arguments (default: " << echoSwitch << ")"; \
    TCLAP::SwitchArg echoSwitchArg("", "echo", msg.str(), commandLine, echoSwitch); \
 \
    msg.str("");msg << "Produce xml description of command line arguments (default: " << xmlSwitch << ")"; \
    TCLAP::SwitchArg xmlSwitchArg("", "xml", msg.str(), commandLine, xmlSwitch); \
 \
    msg.str("");msg << "Address of a structure to store process information (progress, abort, etc.). (default: " << processInformationAddressString << ")"; \
    TCLAP::ValueArg<std::string > processInformationAddressStringArg("", "processinformationaddress", msg.str(), 0, processInformationAddressString, "std::string", commandLine); \
 \
try \
  { \
    /* Build a map of flag aliases to the true flag */ \
    std::map<std::string,std::string> flagAliasMap; \
    std::map<std::string,std::string> deprecatedFlagAliasMap; \
    std::map<std::string,std::string> longFlagAliasMap; \
    std::map<std::string,std::string> deprecatedLongFlagAliasMap; \
    /* Remap flag aliases to the true flag */ \
    std::vector<std::string> targs; \
    std::map<std::string,std::string>::iterator ait; \
    std::map<std::string,std::string>::iterator dait; \
    size_t ac; \
    for (ac=0; ac < static_cast<size_t>(argc); ++ac)  \
       {  \
       if (strlen(argv[ac]) == 2 && argv[ac][0]=='-') \
         { \
         /* short flag case */ \
         std::string tflag(argv[ac], 1, strlen(argv[ac])-1); \
         ait = flagAliasMap.find(tflag); \
         dait = deprecatedFlagAliasMap.find(tflag); \
         if (ait != flagAliasMap.end() || dait != deprecatedFlagAliasMap.end()) \
           { \
           if (ait != flagAliasMap.end()) \
             { \
             /* remap the flag */ \
             targs.push_back("-" + (*ait).second); \
             } \
           else if (dait != deprecatedFlagAliasMap.end()) \
             { \
             std::cout << "Flag \"" << argv[ac] << "\" is deprecated. Please use flag \"-" << (*dait).second << "\" instead. " << std::endl; \
             /* remap the flag */ \
             targs.push_back("-" + (*dait).second); \
             } \
           } \
         else \
           { \
           targs.push_back(argv[ac]); \
           } \
         } \
       else if (strlen(argv[ac]) > 2 && argv[ac][0]=='-' && argv[ac][1]=='-') \
         { \
         /* long flag case */ \
         std::string tflag(argv[ac], 2, strlen(argv[ac])-2); \
         ait = longFlagAliasMap.find(tflag); \
         dait = deprecatedLongFlagAliasMap.find(tflag); \
         if (ait != longFlagAliasMap.end() || dait != deprecatedLongFlagAliasMap.end()) \
           { \
           if (ait != longFlagAliasMap.end()) \
             { \
             /* remap the flag */ \
             targs.push_back("--" + (*ait).second); \
             } \
           else if (dait != deprecatedLongFlagAliasMap.end()) \
             { \
             std::cout << "Long flag \"" << argv[ac] << "\" is deprecated. Please use long flag \"--" << (*dait).second << "\" instead. " << std::endl; \
             /* remap the flag */ \
             targs.push_back("--" + (*dait).second); \
             } \
           } \
         else \
           { \
           targs.push_back(argv[ac]); \
           } \
         } \
       else if (strlen(argv[ac]) > 2 && argv[ac][0]=='-' && argv[ac][1]!='-') \
         { \
         /* short flag case where multiple flags are given at once ala */ \
         /* "ls -ltr" */ \
         std::string tflag(argv[ac], 1, strlen(argv[ac])-1); \
         std::string rflag("-"); \
         for (std::string::size_type fi=0; fi < tflag.size(); ++fi) \
           { \
           std::string tf(tflag, fi, 1); \
           ait = flagAliasMap.find(tf); \
           dait = deprecatedFlagAliasMap.find(tf); \
           if (ait != flagAliasMap.end() || dait != deprecatedFlagAliasMap.end()) \
             { \
             if (ait != flagAliasMap.end()) \
               { \
               /* remap the flag */ \
               rflag += (*ait).second; \
               } \
             else if (dait != deprecatedFlagAliasMap.end()) \
               { \
               std::cout << "Flag \"-" << tf << "\" is deprecated. Please use flag \"-" << (*dait).second << "\" instead. " << std::endl; \
               /* remap the flag */ \
               rflag += (*dait).second; \
               } \
             } \
           else \
             { \
             rflag += tf; \
             } \
           } \
         targs.push_back(rflag); \
         } \
       else \
         { \
         /* skip the argument without remapping (this is the case for any */ \
         /* arguments for flags */ \
         targs.push_back(argv[ac]); \
         } \
       } \
 \
   /* Remap args to a structure that CmdLine::parse() can understand*/ \
   std::vector<char*> vargs; \
   for (ac = 0; ac < targs.size(); ++ac) \
     {  \
     vargs.push_back(const_cast<char *>(targs[ac].c_str())); \
     } \
    commandLine.parse ( vargs.size(), (char**) &(vargs[0]) ); \
    imageName = imageNameArg.getValue(); \
    meshName = meshNameArg.getValue(); \
    interpolation = interpolationArg.getValue(); \
    distanceVector = distanceVectorArg.getValue(); \
    nanValue = nanValueArg.getValue(); \
    attrfileName = attrfileNameArg.getValue(); \
    echoSwitch = echoSwitchArg.getValue(); \
    xmlSwitch = xmlSwitchArg.getValue(); \
    processInformationAddressString = processInformationAddressStringArg.getValue(); \
  } \
catch ( TCLAP::ArgException e ) \
  { \
  std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; \
  return ( EXIT_FAILURE ); \
  }
#define GENERATE_ECHOARGS \
if (echoSwitch) \
{ \
std::cout << "Command Line Arguments" << std::endl; \
std::cout << "    imageName: " << imageName << std::endl; \
std::cout << "    meshName: " << meshName << std::endl; \
std::cout << "    interpolation: " << interpolation << std::endl; \
std::cout << "    distanceVector: " << distanceVector << std::endl; \
std::cout << "    nanValue: " << nanValue << std::endl; \
std::cout << "    attrfileName: " << attrfileName << std::endl; \
std::cout << "    echoSwitch: " << echoSwitch << std::endl; \
std::cout << "    xmlSwitch: " << xmlSwitch << std::endl; \
std::cout << "    processInformationAddressString: " << processInformationAddressString << std::endl; \
}
#define GENERATE_ProcessInformationAddressDecoding \
ModuleProcessInformation *CLPProcessInformation = 0; \
if (processInformationAddressString != "") \
{ \
sscanf(processInformationAddressString.c_str(), "%p", &CLPProcessInformation); \
}
#define PARSE_ARGS GENERATE_LOGO;GENERATE_XML;GENERATE_TCLAP;GENERATE_ECHOARGS;GENERATE_ProcessInformationAddressDecoding;