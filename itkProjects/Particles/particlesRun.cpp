#include "piParticleCore.h"
#include "piParticleBSpline.h"
#include "piParticleSystemSolver.h"
#include "piParticleTools.h"
#include "piParticleTrace.h"
#include "piOptions.h"
#include "piParticleForces.h"
#include "piParticleBSpline.h"
#include "piImageProcessing.h"

using namespace std;
using namespace pi;


#define PRINT_IDX() cout << "using src: " << srcIdx << ", dst: " << dstIdx << endl

int main(int argc, char* argv[]) {
    CSimpleOpt::SOption specs[] = {
        { 9, "--seeConfig", SO_NONE },
        { 1, "-w", SO_NONE },
        { 8, "-d", SO_NONE },
        { 2, "-o", SO_REQ_SEP },
        { 3, "--mean", SO_NONE },
        { 4, "--srcidx", SO_REQ_SEP },
        { 7, "--dstidx", SO_REQ_SEP },
        { 6, "--noTrace", SO_NONE },
        { 10, "--markTrace", SO_NONE },
        { 23, "--markOutput", SO_NONE },
        { 11, "--srcsubj", SO_REQ_SEP },
        { 12, "--inputimage", SO_REQ_SEP },
        { 13, "--inputlabel", SO_REQ_SEP },
        { 14, "--normalize", SO_NONE },
        { 15, "--magnitude", SO_NONE },
        { 16, "--distancemap", SO_NONE },
        { 17, "--createmask", SO_NONE },
        { 18, "--rescaletoshort", SO_NONE },
        { 19, "--mask", SO_REQ_SEP },
        { 20, "--align", SO_NONE },
        { 21, "--warp", SO_NONE },
        { 22, "--norigidalign", SO_NONE },
        { 24, "--onlyrigidalign", SO_NONE },
        { 25, "--showpoints", SO_NONE },
        { 26, "--eval", SO_NONE },
        SO_END_OF_OPTIONS
    };

    cout << argv[0] << " version compiled at " << __TIMESTAMP__ << endl;
    Options parser;
    parser.ParseOptions(argc, argv, specs);
    StringVector& args = parser.GetStringVector("args");
    string output = parser.GetString("-o", "");

    ParticleSystemSolver solver;
    ParticleSystem& system = solver.m_System;
    Options& options = solver.m_Options;


    int srcIdx = atoi(parser.GetString("--srcidx", "1").c_str());
    int dstIdx = atoi(parser.GetString("--dstidx", "0").c_str());

    
    itkcmds::itkImageIO<RealImage> realIO;
    itkcmds::itkImageIO<LabelImage> labelIO;


    if (parser.GetBool("--seeConfig")) {
        solver.LoadConfig(args[0].c_str());
        cout << "Option Contents:\n\n" << options << endl;
        if (args.size() > 1) {
            solver.SaveConfig(args[1].c_str());
        }
    } else if (parser.GetBool("-w", false)) {
        if (args.size() < 1 || output == "") {
            cout << "warping requires [config.txt] -o [outputimage]" << endl;
            return 0;
        }

        // load data
        solver.LoadConfig(args[0].c_str());
        ImageContext& imageCtx = solver.m_ImageContext;

        // bspline resampling
        ParticleBSpline particleTransform;
        particleTransform.SetReferenceImage(imageCtx.GetLabel(0));

        int srcIdx = atoi(parser.GetString("--srcidx", "1").c_str());
        int dstIdx = atoi(parser.GetString("--dstidx", "0").c_str());

        if (parser.GetBool("--mean")) {
            system.ComputeMeanSubject();
            particleTransform.EstimateTransform(system.GetMeanSubject(), system[srcIdx]);
        } else {
            cout << "warping from " << srcIdx << " to " << dstIdx << endl;
            particleTransform.EstimateTransform(system[dstIdx], system[srcIdx]);
        }


        string input = parser.GetString("--inputimage", "");
        string label = parser.GetString("--inputlabel", "");

        cout << parser << endl;

        bool doingSomething = false;
        if (label != "") {
			// write image
			itkcmds::itkImageIO<LabelImage> io;
            LabelImage::Pointer outputImage = particleTransform.WarpLabel(io.ReadImageT(label.c_str()));
			io.WriteImageT(output.c_str(), outputImage);
            doingSomething = true;
        }
        if (input != "") {
			itkcmds::itkImageIO<RealImage> io;
        	RealImage::Pointer outputImage = particleTransform.WarpImage(io.ReadImageT(input.c_str()));
        	io.WriteImageT(output.c_str(), outputImage);
            doingSomething = true;
        }
        if (!doingSomething) {
            cout << "-w requires --inputimage or --inputlabel to warp" << endl;
        }
    } else if (parser.GetBool("--warp")) {
        if (args.size() < 2) {
        	cout << "--warp requires [output.txt] [source-image] [warped-output-image]" << endl;
            return 0;
        }

        PRINT_IDX();
        string outputName = args[1];
        string inputImage, inputLabel, refImageName;
        parser.GetStringTo("--inputimage", inputImage);
        parser.GetStringTo("--inputlabel", inputLabel);
        parser.GetStringTo("--reference", refImageName);

        solver.LoadConfig(args[0].c_str());

        
        if (system.size() < 2) {
            cout << "system is not loaded successfully" << endl;
            return 0;
        }
        if (inputImage != "") {
            RealImage::Pointer refImage;
            // warp from srcidx to dstidx
            if (refImageName != "") {
                refImage = realIO.ReadImageT(refImageName.c_str());
            }
            RealImage::Pointer output = warp_image<RealImage>(system[dstIdx], system[srcIdx], realIO.ReadImageT(inputImage.c_str()), refImage, false, parser.GetBool("--norigidalign"), parser.GetBool("--onlyrigidalign"));
            realIO.WriteImageT(outputName.c_str(), output);
        }
        if (inputLabel != "") {
            LabelImage::Pointer refImage;
            // warp from srcidx to dstidx
            if (refImageName != "") {
                refImage = labelIO.ReadImageT(refImageName.c_str());
            }
            LabelImage::Pointer output = warp_image<LabelImage>(system[dstIdx], system[srcIdx], labelIO.ReadImageT(inputLabel.c_str()), refImage, true, parser.GetBool("--norigidalign"), parser.GetBool("--onlyrigidalign"));
            labelIO.WriteImageT(outputName.c_str(), output);
        }
    } else if (parser.GetBool("--markTrace")) {
        if (args.size() < 2) {
        	cout << "--markTrace requires [output.txt] [reference-image] [output-image] --srcidx [point-index] --srcsubj [subject-index]" << endl;
            return 0;
        }
        ifstream in(args[0].c_str());
        ParticleTrace trace;
        trace.Read(in);
        in.close();
        cout << trace << endl;

        int srcIdx = atoi(parser.GetString("--srcidx", "-1").c_str());
        int srcSubj = atoi(parser.GetString("--srcsubj", "-1").c_str());
        
        itkcmds::itkImageIO<LabelImage> io;
        LabelImage::Pointer ref = io.ReadImageT(args[1].c_str());
        LabelImage::Pointer canvas = io.NewImageT(ref);
        for (int i = 0; i < trace.system.size(); i++) {
            if (srcSubj == -1 || srcSubj == i) {
                for (int j = 0; j < trace.system[i].timeSeries.size(); j++) {
                    for (int k = 0; k <= trace.system[i].maxIdx; k++) {
                        if (srcIdx == -1 || srcIdx == k) {
                            Particle& p = trace.system[i].timeSeries[j][k];
                            IntIndex idx;
                            fordim (l) {
                                idx[l] = p.x[l] + 0.5;
                            }
                            (*canvas)[idx] = j;
                        }
                    }
                }
            }
        }
    } else if (parser.GetBool("--markOutput")) {
        if (args.size() < 1) {
            cout << "--markOutput requires [output.txt] [output-image]" << endl;
        }
        solver.LoadConfig(args[0].c_str());
        LabelImage::Pointer label = labelIO.NewImageT(solver.m_ImageContext.GetLabel(srcIdx));
        int nPoints = system[srcIdx].GetNumberOfPoints();
        cout << "Marking " << nPoints << " points ..." << endl;
        MarkAtImage(system[srcIdx], system[srcIdx].GetNumberOfPoints(), label, 1);
        labelIO.WriteImageT(args[1].c_str(), label);
    } else if (parser.GetBool("--showpoints")) {
        if (args.size() < 1) {
            cout << "--showpoints requires [output.txt]" << endl;
            return 0;
        }
        solver.LoadConfig(args[0].c_str());
        for (int i = 0; i < system.size(); i++) {
            cout << "Subject " << i << endl;
            for (int j = 0; j < system[i].GetNumberOfPoints(); j++) {
                cout << system[i][j] << endl;
            }
        }

    } else if (parser.GetBool("--normalize")) {
        if (args.size() < 3) {
            cout << "--normalize requires [input-image] [mask-image] [output-image]" << endl;
            return 0;
        }
        itkcmds::itkImageIO<RealImage> iod;
        itkcmds::itkImageIO<LabelImage> iol;
        
        RealImage::Pointer input = iod.ReadImageT(args[0].c_str());
        LabelImage::Pointer label = iol.ReadImageT(args[1].c_str());
        
        ImageProcessing proc;
        RealImage::Pointer output = proc.NormalizeIntensity(input, label);
        
        iod.WriteImageT(args[2].c_str(), output);
    } else if (parser.GetBool("--magnitude")) {
        if (args.size() < 2) {
            cout << "--magnitude requires [input-vector-image] [output-float-image]" << endl;
            return 0;
        }
        itkcmds::itkImageIO<VectorImage> vio;
        itkcmds::itkImageIO<RealImage> rio;

        VectorImage::Pointer input = vio.ReadImageT(args[0].c_str());
        ImageProcessing proc;
        RealImage::Pointer output = proc.ComputeMagnitudeMap(input);
        rio.WriteImageT(args[1].c_str(), output);
    } else if (parser.GetBool("--rescaletoshort")) {
        if (args.size() < 2) {
            cout << "--rescaletoshort requires [input-real-image] [output-short-image] [output-min] [output-max] (--mask mask-input)" << endl;
            return 0;
        }
        itkcmds::itkImageIO<RealImage> rio;
        itkcmds::itkImageIO<LabelImage> lio;


        string maskInput;
        parser.GetStringTo("--mask", maskInput);
        LabelImage::Pointer mask;
        if (maskInput != "") {
            mask = lio.ReadImageT(maskInput.c_str());
        }

        LabelPixel outMax = 10000;
        LabelPixel outMin = 0;
        if (args.size() > 2) {
            outMin = atoi(args[2].c_str());
        }
        if (args.size() > 3) {
            outMax = atoi(args[3].c_str());
        }
        RealImage::Pointer input = rio.ReadImageT(args[0].c_str());
        ImageProcessing proc;
        LabelImage::Pointer output = proc.NormalizeToIntegralType(input, outMin, outMax, mask);
        lio.WriteImageT(args[1].c_str(), output);
    } else if (parser.GetBool("--distancemap")) {
        if (args.size() < 2) {
            cout << "--distancemap requires [input-label-image] [output-vector-image]" << endl;
            return 0;
        }
        itkcmds::itkImageIO<LabelImage> lio;
        itkcmds::itkImageIO<VectorImage> vio;

        LabelImage::Pointer input = lio.ReadImageT(args[0].c_str());
        ImageProcessing proc;
        VectorImage::Pointer output = proc.ComputeDistanceMap(input);
        vio.WriteImageT(args[1].c_str(), output);
    } else if (parser.GetBool("--createmask")) {
        if (args.size() < 2) {
            cout << "--createmask requires [input-label-image] [input-label-image]" << endl;
            return 0;
        }
        itkcmds::itkImageIO<LabelImage> lio;
        LabelImage::Pointer input = lio.ReadImageT(args[0].c_str());
        ImageProcessing proc;
        LabelImage::Pointer output = proc.ThresholdToBinary(input);
        lio.WriteImageT(args[1].c_str(), output);
    } else if (parser.GetBool("--transform")) {
//        string realInput, labelInput;
//        parser.GetStringTo("--inputimage", realInput);
//        parser.GetStringTo("--inputlabel", labelInput);
//        string transform;
//        ImageProcessing proc;
//        if (realInput != "") {
//            itkcmds::itkImageIO<RealImage> io;
//            RealImage::Pointer output = proc.TransformImage<RealImage>(io.ReadImageT(realInput.c_str()));
//            io.WriteImageT(args[0].c_str(), output);
//        }
//        if (labelInput != "") {
//            itkcmds::itkImageIO<LabelImage> io;
//            LabelImage::Pointer output = proc.TransformImage<LabelImage>(io.ReadImageT(labelInput.c_str()));
//            io.WriteImageT(args[0].c_str(), output);
//        }
    } else if (parser.GetBool("--align")) {
        // we align #1 to #0
        if (args.size() > -1) {
            cout << "--align requires [config.txt] [output.vtk]" << endl;
        }
        solver.LoadConfig(args[0].c_str());
        if (system.points() == 0) {
            cout << "no points loaded" << endl;
        }

        system[srcIdx].ComputeAlignment(system[dstIdx]);
        system[srcIdx].AlignmentTransformX2Y();
        cout << system[srcIdx].alignment << endl;
        cout << system[srcIdx].inverseAlignment << endl;
        export2vtk(system[srcIdx], "src.vtk", 0);
        export2vtk(system[dstIdx], "dst.vtk", 1);
    } else if (parser.GetBool("--eval")) {
        if (args.size() < 2) {
            cout << "--eval requires [label1] [label2]" << endl;
            return 0;
        }
        AtlasSimilarityScore score;
        score.Compute(labelIO.ReadImageT(args[0].c_str()), labelIO.ReadImageT(args[1].c_str()));
        cout << score << endl;
        return 0; 
    } else {
        if (args.size() < 2) {
        	cout << "registration requires [config.txt] [output.txt]" << endl;
            return 0;
        }

        if (!solver.LoadConfig(args[0].c_str())) {
            return 0;
        }
        
        if (parser.GetBool("--noTrace")) {
            options.SetString("PreprocessingTrace:", string(""));
            options.SetString("RunTrace:", string(""));
            cout << "Trace disabled..." << endl;
        }
        
        if (!options.GetBool("no_preprocessing")) {
            solver.Preprocessing();
        } else {
            cout << "Skipping preprocessing; continue with given particles from the input" << endl;
            cout << "the first particle of each subject: " << endl;
            for (int i = 0; i < system.GetNumberOfSubjects(); i++) {
                cout << system[i][0] << endl;
            }
            cout << "continue optimization;" << endl;
        }
        
        solver.Run();
        solver.SaveConfig(args[1].c_str());

        // final point location marking onto image
        StringVector& markingImages = solver.m_Options.GetStringVector("FinalMarking:");
        if (markingImages.size() > 0) {
            itkcmds::itkImageIO<LabelImage> io;

            for (int i = 0; i < markingImages.size(); i++) {
                LabelImage::Pointer label = solver.m_ImageContext.GetLabel(i);
                LabelImage::Pointer canvas = io.NewImageT(label);
                ParticleArray& data = system[i].m_Particles;
                MarkAtImage<ParticleArray>(data, data.size(), canvas, 1);
                io.WriteImageT(markingImages[i].c_str(), canvas);
            }
        }

        StringVector& warpedLabels = solver.m_Options.GetStringVector("OutputLabelToFirst:");
        if (warpedLabels.size() > 0) {
            itkcmds::itkImageIO<LabelImage> io;
            StringVector& labelImages = solver.m_Options.GetStringVector("LabelImages:");
            if (warpedLabels.size() != labelImages.size()) {
                cout << "OutputLabelToFirst: and LabeImages: size are different" << endl;
            } else {
                for (int i = 1; i < warpedLabels.size(); i++) {
                    LabelImage::Pointer label = io.ReadImageT(labelImages[i].c_str());
                    // bspline resampling
                    LabelImage::Pointer output = warp_image<LabelImage>(system[0], system[1], label, label, true, false, false);
                    io.WriteImageT(warpedLabels[i].c_str(), output);
                }
            }
        }
    }
    return 0;
}
