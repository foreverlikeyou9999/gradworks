//
//  myImageProcessing.h
//  ParticlesGUI
//
//  Created by Joohwi Lee on 2/11/13.
//
//

#ifndef __ParticlesGUI__myImageProcessing__
#define __ParticlesGUI__myImageProcessing__

#include <iostream>
#include "piImageDef.h"
#include "itkRescaleIntensityImageFilter.h"
#include <itkImageToHistogramFilter.h>

class vtkPolyData;


using namespace std;

namespace pi {

    typedef itk::Statistics::ImageToHistogramFilter<RealImage> ImageHistogramFilterType;

    class LabelSimilarityScore {
    public:
        int L;
        int A;
        int B;
        int AB;
        double minDist;
        double maxDist;
        int total() const { return (A+B+2*AB); }
        double dice() const { return (2.0*AB)/(A+B+2*AB+0.0); }
        double overlap() const { return AB / total(); }
        LabelSimilarityScore() {
            L = A = B = AB = 0;
            minDist = maxDist = 0;
        }
    };
    typedef std::vector<LabelSimilarityScore> LabelScoreVector;

    class AtlasSimilarityScore {
    public:
        LabelScoreVector labelMap;
        void Compute(LabelImage::Pointer a, LabelImage::Pointer b);
        void Add(LabelPixel a, LabelPixel b);
        LabelSimilarityScore& operator()(int l) {
            return labelMap[l];
        }
    };

    ostream& operator<<(ostream& os, const LabelSimilarityScore& score);
    ostream& operator<<(ostream& os, const AtlasSimilarityScore& score);

    class ImageProcessing {
    public:
        // anti-aliasing, connected component, and closing morphology
        LabelImage::Pointer SmoothLabelMap(LabelImage::Pointer img);
        LabelImage::Pointer ErodedBorder(LabelImage::Pointer img);
        
        GradientImage::Pointer ComputeGaussianGradient(LabelImage::Pointer img, double sigma = -1);
        GradientImage::Pointer ComputeGradient(LabelImage::Pointer img);
        GradientImage::Pointer ComputeGaussianGradient(RealImage::Pointer img, double sigma = -1);
        GradientImage::Pointer ComputeGradient(RealImage::Pointer img);
        RealImage::Pointer ComputeMagnitudeMap(VectorImage::Pointer img);
        RealImage::Pointer ComputeMagnitudeMap(GradientImage::Pointer img);
        VectorImage::Pointer ComputeDistanceMap(LabelImage::Pointer img);

        LabelImage::Pointer ThresholdToBinary(LabelImage::Pointer img);
        RealImage::Pointer ComputeGaussianGradientMagnitude(RealImage::Pointer img, double sigma = -1);
        LabelImage::Pointer Ellipse(int* outputSize, double* center, double* radius);
        vtkPolyData* ConvertToMesh(LabelImage::Pointer image);
        RealImage::Pointer NormalizeIntensity(RealImage::Pointer image, LabelImage::Pointer label);
        LabelImage::Pointer NormalizeToIntegralType(RealImage::Pointer src, LabelPixel min, LabelPixel max, LabelImage::Pointer label);

        LabelImage::Pointer ZeroCrossing(LabelImage::Pointer src);
        ImageHistogramFilterType::HistogramPointer ComputeHistogram(RealImage::Pointer real, int nbin, DataReal rmin, DataReal rmax);
        string ComputeHistogramToString(RealImage::Pointer real, int nbin, DataReal rmin, DataReal rmax);

        template <class T>
        typename T::Pointer TransformImage(typename T::Pointer srcImg, std::string transform) {

        }
        
        template <class T>
        typename T::Pointer RescaleIntensity(typename T::Pointer srcImg, typename T::PixelType min, typename T::PixelType max) const {
            typedef itk::RescaleIntensityImageFilter<T> RescaleFilter;
            typename RescaleFilter::Pointer filter = RescaleFilter::New();
            filter->SetInput(srcImg);
            filter->SetOutputMinimum(min);
            filter->SetOutputMaximum(max);
            filter->Update();
            return filter->GetOutput();
        }
    };
}
#endif /* defined(__ParticlesGUI__myImageProcessing__) */
