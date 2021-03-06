#include "pthread.h"

#include "itkImageCommon.h"
#include "PixelMathImageFilter.h"
typedef itk::Image<unsigned int,3> ImageType;

#include "itkUnaryFunctorImageFilter.h"

#include "boost/program_options.hpp"
namespace po = boost::program_options;

#include "string"
#include "vector"
#include "iostream"
using namespace std;

#include "muParser.h"

void print_help_message(char* argv[]) {
	printf("\
PixelMath - pixel-wise math expression evaluator\n\n\
	usage: %s [options] files \n\
	--eq,-e 	equation for PixelMath \n\
	--output,-o	output image file \n\
	\n\
	In the equation, each pixel of images are represented as variable A,B,C,D, and E .\n\
	\n\
	Examples)\n\
		+1 for each pixel >> PixelMath -e \"A+1\" -o plus1.nrrd yourinput.nrrd\n\
		label swap 1 to 2; 2 to 1 >> PixelMath -e \"(A==1)?2:((A==2)?1:A)\" -o a.nrrd input.nrrd\n\
		adding two images >> PixelMath -e \"A+B\" -o outputsum.nrrd input.nrrd input2.nrrd\n\
		threshold between 1 to 20 >> PixelMath -e \"(A>0)?((A<=20)?:1):0\" -o thresholded.nrrd input.nrrd\n\
", argv[0]);
}


pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;

void run(vector<string> &inputfiles, string &outfile, string &eq, bool query) {
	using namespace mu;
	if (!query) {
		try {
			int ret;
			typedef PixelMathImageFilter<ImageType,ImageType> PixelMathFilter;
			PixelMathFilter::Pointer pixelMath = PixelMathFilter::New();
			if (inputfiles.size() > 5) {
				cout << "Currently, up to 5 images can be processed" << endl;
				return;
			}
			for (unsigned int i = 0; i < inputfiles.size(); i++) {
				ImageType::Pointer input = ReadImageT<ImageType>(inputfiles[i].c_str(),ret);
				pixelMath->PushBackInput(input);
			}
			pixelMath->SetEquation(eq);
			pixelMath->Update();
			ImageType::Pointer output = pixelMath->GetOutput();
			WriteImageT<ImageType>(outfile.c_str(), output);
		} catch (Parser::exception_type &e) {
			cout << e.GetMsg() << endl;
		}
	} else {
		double a = 0;
		Parser parser;
		parser.DefineVar("A", &a);
		parser.SetExpr(eq);
		varmap_type vmap = parser.GetVar();
		for (varmap_type::iterator item = vmap.begin(); item != vmap.end(); ++item) {
			cout << item->first << " == " << (item->second) << endl;
		}
	}
}

int main(int argc, char* argv[]) {
	bool query = false;
	string outfile, eq;
	po::options_description desc("Options");
	desc.add_options()
		("help,h", "produce help messages")
		("eq,e", po::value<string>(&eq), "equation for pixels ex) A+1")		
		("query,q", po::value<bool>(&query), "query for variables")
		("output,o", po::value<string>(&outfile)->default_value("result.nrrd"), "output filename")
	;

	po::options_description args("Arguments");
	args.add_options()
		("files", po::value< vector<string> >(), "files... ")
	;

	po::positional_options_description p;
	p.add("files", -1);

	po::options_description all_opts("All options");
	all_opts.add(args).add(desc);

	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(all_opts).positional(p).run(), vm);
	po::notify(vm);

	if (vm.count("files") < 1 || eq == "" || vm.count("help") > 0) {
		print_help_message(argv);
		return 0;
	}

	vector<string> inputFiles = vm["files"].as< vector<string> >();
	run(inputFiles, outfile, eq, query);
}
