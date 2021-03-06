//
//  myImageProcessing.cpp
//  ParticlesGUI
//
//  Created by Joohwi Lee on 2/11/13.
//
//

#include "piImageProcessing.h"
#include "itkAntiAliasBinaryImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkConnectedComponentImageFilter.h"
#include "itkRelabelComponentImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkBinaryCrossStructuringElement.h"
#include "itkBinaryDilateImageFilter.h"
#include "itkBinaryErodeImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkSubtractImageFilter.h"
#include "itkRecursiveGaussianImageFilter.h"
#include "itkGradientRecursiveGaussianImageFilter.h"
#include "itkEllipseSpatialObject.h"
#include "itkSpatialObjectToImageFilter.h"
#include "itkMesh.h"
#include "itkVTKPolyDataWriter.h"
#include "itkUnaryFunctorImageFilter.h"
#include <itkGradientMagnitudeRecursiveGaussianImageFilter.h>
#include "itkSignedDanielssonDistanceMapImageFilter.h"
#include "itkDanielssonDistanceMapImageFilter.h"
#include "itkVectorMagnitudeImageFilter.h"
#include "itkMesh.h"
#include "itkBinaryMask3DMeshSource.h"
#include <itkStatisticsImageFilter.h>

#include "piImageIO.h"

#ifdef DIMENSION3
#include "itkBinaryMask3DMeshSource.h"
#endif

namespace pi {
#ifdef DIMENSION3
    typedef itk::EllipseSpatialObject<__Dim> EllipseType;
    typedef itk::SpatialObjectToImageFilter<EllipseType, LabelImage> SpatialObjectToImageFilterType;
    typedef itk::Mesh<PointReal> MeshType;
    typedef itk::BinaryMask3DMeshSource<LabelImage, MeshType> MeshSourceType;
#endif
    typedef itk::ZeroCrossingImageFilter<LabelImage, LabelImage> ZeroCrossingFilterType;
    typedef itk::CastImageFilter<LabelImage ,RealImage > CastToRealFilterType;
    typedef itk::AntiAliasBinaryImageFilter<RealImage, RealImage> AntiAliasFilterType;
    typedef itk::RescaleIntensityImageFilter<RealImage, LabelImage > RescaleFilter;
    typedef itk::ConnectedComponentImageFilter<LabelImage, LabelImage> ConnectedComponentFilterType;
    typedef itk::RelabelComponentImageFilter<LabelImage, LabelImage> RelabelFilterType;
    typedef itk::BinaryThresholdImageFilter<LabelImage, LabelImage> BinaryThreshFilterType;
    typedef itk::BinaryBallStructuringElement<LabelImage::PixelType,__Dim> StructuringElementType;
    typedef itk::BinaryCrossStructuringElement<LabelImage::PixelType,__Dim> AsymStructuringElementType;
    typedef itk::BinaryDilateImageFilter<LabelImage, LabelImage, StructuringElementType> DilateFilterType;
    typedef itk::BinaryErodeImageFilter<LabelImage, LabelImage, StructuringElementType> ErodeFilterType;
    typedef itk::SubtractImageFilter<LabelImage, LabelImage, LabelImage> SubtractFilterType;
    typedef itk::RecursiveGaussianImageFilter<RealImage> GaussianFilterType;
    typedef itk::SignedDanielssonDistanceMapImageFilter<LabelImage, RealImage> SignedDistanceMapFilterType;
    typedef itk::DanielssonDistanceMapImageFilter<LabelImage, RealImage> DistanceMapFilterType;
    typedef DistanceMapFilterType::VectorImageType DistanceVectorImageType;
    typedef DistanceVectorImageType::PixelType DistancePixelType;
    typedef itk::VectorMagnitudeImageFilter<VectorImage, RealImage> GradientMagnitudeFilterType;
    typedef itk::StatisticsImageFilter<RealImage> RealImageStatisticsFilterType;

    class OffsetToVector {
    public:
        bool operator!=(const OffsetToVector& o) {
            return false;
        }
        bool operator==(const OffsetToVector& o) {
            return true;
        }
        inline VectorType operator()(SignedDistanceMapFilterType::VectorImageType::PixelType o) {
            VectorType v;
            fordim (k) {
                v[k] = o[k];
            }
            return v;
        }
    };

    LabelImage::Pointer ImageProcessing::SmoothLabelMap(LabelImage::Pointer img) {
        cout << "Computing label map smoothing ..." << flush;

        LabelImage::Pointer procImage;

        const int LABEL_VAL = 255;
        CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
        toReal->SetInput(img);
        toReal->Update();

        AntiAliasFilterType::Pointer antiAliasFilter = AntiAliasFilterType::New();
        antiAliasFilter->SetInput( toReal->GetOutput() );
        antiAliasFilter->SetMaximumRMSError(1);
        antiAliasFilter->SetNumberOfIterations(10);
        antiAliasFilter->SetNumberOfLayers(2);
        antiAliasFilter->Update();

//        itkcmds::itkImageIO<RealImage> iox;
//        iox.WriteImageT("/tmpfs/aadouble.nrrd", antiAliasFilter->GetOutput());

        RescaleFilter::Pointer rescale = RescaleFilter::New();
        rescale->SetInput( antiAliasFilter->GetOutput() );
        rescale->SetOutputMinimum(   0 );
        rescale->SetOutputMaximum( 255 );
        rescale->Update();
        procImage = rescale->GetOutput();

        BinaryThreshFilterType::Pointer binTreshFilter = BinaryThreshFilterType::New();
        binTreshFilter->SetInput(rescale->GetOutput());
        binTreshFilter->SetUpperThreshold(255);
        binTreshFilter->SetLowerThreshold(127);
        binTreshFilter->SetOutsideValue (0);
        binTreshFilter->SetInsideValue (1);
        binTreshFilter->Update();
        procImage = binTreshFilter->GetOutput();

        ConnectedComponentFilterType::Pointer concompFilter = ConnectedComponentFilterType::New();
        RelabelFilterType::Pointer relabelFilter = RelabelFilterType::New();
        BinaryThreshFilterType::Pointer binTreshFilter2 = BinaryThreshFilterType::New();
        concompFilter->SetInput(procImage);
        concompFilter->Update();
        procImage = concompFilter->GetOutput();

        relabelFilter->SetInput(concompFilter->GetOutput());
        relabelFilter->Update();
        procImage = relabelFilter->GetOutput();

        binTreshFilter2->SetInput(relabelFilter->GetOutput());
        binTreshFilter2->SetUpperThreshold(1);
        binTreshFilter2->SetLowerThreshold(1);// only largest object (labels are sorted largest to smallest)
        binTreshFilter2->SetOutsideValue (0);
        binTreshFilter2->SetInsideValue (LABEL_VAL);
        binTreshFilter2->Update();
        procImage   = binTreshFilter2->GetOutput();

        StructuringElementType structuringElement;
        structuringElement.SetRadius(1);  // 3x3x3 structuring element
        structuringElement.CreateStructuringElement();

        DilateFilterType::Pointer dilateFilter = DilateFilterType::New();
        ErodeFilterType::Pointer erodeFilter = ErodeFilterType::New();

        dilateFilter->SetDilateValue (LABEL_VAL);
        dilateFilter->SetKernel(structuringElement);
        dilateFilter->SetInput(procImage);
        dilateFilter->Update();

        erodeFilter->SetErodeValue(LABEL_VAL);
        erodeFilter->SetKernel(structuringElement);
        erodeFilter->SetInput(dilateFilter->GetOutput());
        erodeFilter->Update();

        ErodeFilterType::Pointer erodeFilter2 = ErodeFilterType::New();
        erodeFilter2->SetErodeValue(LABEL_VAL);
        erodeFilter2->SetKernel(structuringElement);
        erodeFilter2->SetInput(erodeFilter->GetOutput());
        erodeFilter2->Update();

        procImage = erodeFilter2->GetOutput();

        cout << "done" << endl;
        return procImage;
    }

    LabelImage::Pointer ImageProcessing::ErodedBorder(pi::LabelImage::Pointer img) {
        LabelImage::Pointer procImage;

        StructuringElementType structuringElement;
        structuringElement.SetRadius(1);  // 3x3x3 structuring element
        structuringElement.CreateStructuringElement();

        ErodeFilterType::Pointer erodeFilter = ErodeFilterType::New();
        erodeFilter->SetErodeValue(255);
        erodeFilter->SetKernel(structuringElement);
        erodeFilter->SetInput(img);
        erodeFilter->Update();
        procImage = erodeFilter->GetOutput();

        SubtractFilterType::Pointer subFilter = SubtractFilterType::New();
        subFilter->SetInput1(img);
        subFilter->SetInput2(procImage);
        subFilter->Update();

        return subFilter->GetOutput();
    }

    GradientImage::Pointer ImageProcessing::ComputeGaussianGradient(LabelImage::Pointer img, double sigma) {
        CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
        toReal->SetInput(img);
        toReal->Update();

        GaussianGradientFilterType::Pointer gradientFilter = GaussianGradientFilterType::New();
        gradientFilter->SetInput(toReal->GetOutput());
        if (sigma > -1) {
            gradientFilter->SetSigma(sigma);
        }
        gradientFilter->Update();
        return gradientFilter->GetOutput();
    }
    
    GradientImage::Pointer ImageProcessing::ComputeGradient(LabelImage::Pointer img) {
        CastToRealFilterType::Pointer toReal = CastToRealFilterType::New();
        toReal->SetInput(img);
        toReal->Update();

        GradientFilterType::Pointer gradientFilter = GradientFilterType::New();
        gradientFilter->SetInput(toReal->GetOutput());
        gradientFilter->Update();
        return gradientFilter->GetOutput();
    }

    GradientImage::Pointer ImageProcessing::ComputeGaussianGradient(RealImage::Pointer img, double sigma) {
        GaussianGradientFilterType::Pointer gradientFilter = GaussianGradientFilterType::New();
        gradientFilter->SetInput(img);
        if (sigma > -1) {
            gradientFilter->SetSigma(sigma);
        }
        gradientFilter->Update();
        return gradientFilter->GetOutput();
    }

    GradientImage::Pointer ImageProcessing::ComputeGradient(RealImage::Pointer img) {
        GradientFilterType::Pointer gradientFilter = GradientFilterType::New();
        gradientFilter->SetInput(img);
        gradientFilter->Update();
        return gradientFilter->GetOutput();
    }

    RealImage::Pointer ImageProcessing::ComputeGaussianGradientMagnitude(RealImage::Pointer img, double sigma) {
        typedef itk::GradientMagnitudeRecursiveGaussianImageFilter<RealImage> FilterType;
        FilterType::Pointer filter = FilterType::New();
        if (sigma > 0) {
            filter->SetSigma(sigma);
        }
        filter->SetInput(img);
        filter->Update();
        return filter->GetOutput();
    }

    LabelImage::Pointer ImageProcessing::Ellipse(int* outputSize, double *center, double *radius) {
#ifdef DIMENSION3
        SpatialObjectToImageFilterType::Pointer imageFilter = SpatialObjectToImageFilterType::New();
        RealImage::SizeType size;
        fordim (k) {
            size[k] = outputSize[k];
        }
        imageFilter->SetSize(size);

        EllipseType::Pointer ellipse = EllipseType::New();
        ellipse->SetDefaultInsideValue(255);
        ellipse->SetDefaultOutsideValue(0);

        EllipseType::ArrayType axes;
        fordim (k) {
            axes[k] = radius[k];
        }
        ellipse->SetRadius(axes);

        EllipseType::TransformType::Pointer transform = EllipseType::TransformType::New();
        transform->SetIdentity();
        TransformType::OutputVectorType translation;
        fordim (k) {
            translation[k] = center[k];
        }
        transform->Translate(translation, false);

        ellipse->SetObjectToParentTransform(transform);
        imageFilter->SetInput(ellipse);
        imageFilter->SetUseObjectValue(true);
        imageFilter->SetOutsideValue(0);
        imageFilter->Update();
        return imageFilter->GetOutput();
#else
        return LabelImage::Pointer();
#endif
    }


    VectorImage::Pointer ImageProcessing::ComputeDistanceMap(LabelImage::Pointer img) {
        cout << "Computing distance map ..." << flush;
        LabelImage::Pointer binaryMap = ThresholdToBinary(img);

        // construct signed distance filter
        SignedDistanceMapFilterType::Pointer distmapFilter = SignedDistanceMapFilterType::New();
        distmapFilter->SetInput(binaryMap);
        distmapFilter->Update();
        SignedDistanceMapFilterType::OutputImagePointer distmap = distmapFilter->GetOutput();

        typedef
        itk::UnaryFunctorImageFilter<SignedDistanceMapFilterType::VectorImageType, VectorImage, OffsetToVector> OffsetToVectorCastFilterType;

        OffsetToVectorCastFilterType::Pointer caster = OffsetToVectorCastFilterType::New();
        caster->SetInput(distmapFilter->GetVectorDistanceMap());
        caster->Update();
        cout << "done" << endl;
        return caster->GetOutput();
    }

    RealImage::Pointer ImageProcessing::ComputeMagnitudeMap(VectorImage::Pointer img) {
        GradientMagnitudeFilterType::Pointer magFilter = GradientMagnitudeFilterType::New();
        magFilter->SetInput(img);
        magFilter->Update();
        return magFilter->GetOutput();
    }

    RealImage::Pointer ImageProcessing::ComputeMagnitudeMap(GradientImage::Pointer img) {
        typedef itk::VectorMagnitudeImageFilter<GradientImage, RealImage> MagnitudeFilterType;
        MagnitudeFilterType::Pointer magFilter = MagnitudeFilterType::New();
        magFilter->SetInput(img);
        magFilter->Update();
        return magFilter->GetOutput();
    }

    vtkPolyData* ImageProcessing::ConvertToMesh(LabelImage::Pointer image) {
#ifdef DIMENSION3
        typedef itk::Mesh<double> MeshType;
        typedef itk::BinaryMask3DMeshSource<LabelImage, MeshType> MeshSourceType;

        MeshSourceType::Pointer meshSource = MeshSourceType::New();
        meshSource->SetInput(image);
        meshSource->SetObjectValue(255);
        meshSource->Update();
        MeshType::Pointer mesh = meshSource->GetOutput();
#endif
        return NULL;
    }


    LabelImage::Pointer ImageProcessing::ZeroCrossing(LabelImage::Pointer src) {
        typedef itk::ZeroCrossingImageFilter<LabelImage, LabelImage> ZeroFilter;
        ZeroFilter::Pointer zero = ZeroFilter::New();
        zero->SetInput(src);
        zero->Update();
        return zero->GetOutput();
    }
    
    LabelImage::Pointer ImageProcessing::NormalizeToIntegralType(RealImage::Pointer src, LabelPixel min, LabelPixel max, LabelImage::Pointer mask) {
        ImageIO<LabelImage> io;
        LabelImage::Pointer output = io.CastImageFromS<RealImage>(src);
        output->FillBuffer(0);

        LabelImageIteratorType itermask(mask, mask->GetBufferedRegion());
        LabelImageIteratorType iterout(output, output->GetBufferedRegion());
        RealImageIteratorType iterin(src, src->GetBufferedRegion());

        iterout.GoToBegin();
        iterin.GoToBegin();

        RealImageStatisticsFilterType::Pointer filter = RealImageStatisticsFilterType::New();
        filter->SetInput(src);
        filter->Update();
        DataReal inMax = filter->GetMaximum();
        DataReal inMin = filter->GetMinimum();

        while (!iterout.IsAtEnd()) {
            if (mask.IsNull() || itermask.Get() > 0) {
                DataReal in = iterin.Get();
                LabelPixel out = (max - min) * (in - inMin) / (inMax - inMin) + min;
                iterout.Set(out);
            }
            ++iterout;
            ++iterin;
            ++itermask;
        }

        return output;
    }

    RealImage::Pointer ImageProcessing::NormalizeIntensity(RealImage::Pointer image, LabelImage::Pointer label) {
        double sum = 0, sum2 = 0;
        ImageIO<RealImage> io;
        RealImage::Pointer output = io.CopyImage(image);
        output->FillBuffer(0);

        RealImageIteratorType iter(image, image->GetBufferedRegion());
        LabelImageIteratorType iter2(label, image->GetBufferedRegion());
        RealImageIteratorType oiter(output, image->GetBufferedRegion());
        int n = 0;
        iter.GoToBegin();
        iter2.GoToBegin();
        while (!iter.IsAtEnd()) {
            if (iter2.Get() > 0) {
                ImageReal v = iter.Get();
                sum += v;
                sum2 += (v*v);
                ++n;
            }
            ++iter;
            ++iter2;
        }
        double mean = sum/(n-1);
        double sigma  = sqrt(sum2/(n-1) - mean*mean);
        iter.GoToBegin();
        iter2.GoToBegin();
        while (!iter.IsAtEnd()) {
            if (iter2.Get() > 0) {
                oiter.Set((iter.Get() - mean)/sigma);
            }
            ++iter;
            ++iter2;
            ++oiter;
        }
        return output;
    }

    LabelImage::Pointer ImageProcessing::ThresholdToBinary(LabelImage::Pointer img) {
        BinaryThreshFilterType::Pointer binThreshFilter = BinaryThreshFilterType::New();
        binThreshFilter->SetInput(img);
        binThreshFilter->SetInsideValue(1);
        binThreshFilter->SetOutsideValue(0);
        binThreshFilter->SetLowerThreshold(1);
        binThreshFilter->SetUpperThreshold(255);
        binThreshFilter->Update();
        return binThreshFilter->GetOutput();
    }


    ImageHistogramFilterType::HistogramPointer ImageProcessing::ComputeHistogram(RealImage::Pointer real, int nbin, DataReal rmin, DataReal rmax) {
        return ImageHistogramFilterType::HistogramPointer();
    }

    string ImageProcessing::ComputeHistogramToString(RealImage::Pointer real, int nbin, DataReal rmin, DataReal rmax) {
        return "";
    }



    void AtlasSimilarityScore::Add(LabelPixel a, LabelPixel b) {
        if (labelMap.size() <= a || labelMap.size() <= b) {
            labelMap.resize(std::max(a,b)+1);
        }
        labelMap[a].L = a;
        labelMap[b].L = b;
        if (a == b) {
            labelMap[a].AB++;
        } else {
            labelMap[a].A++;
            labelMap[b].B++;
        }
    }

    void AtlasSimilarityScore::Compute(LabelImage::Pointer a, LabelImage::Pointer b) {
        int na = a->GetPixelContainer()->Size();
        int nb = b->GetPixelContainer()->Size();

        if (na != nb) {
            cout << "the size of image is different!" << endl;
        }

        const LabelPixel* pa = a->GetBufferPointer();
        const LabelPixel* pb = b->GetBufferPointer();
        for (int i = 0; i < na; i++) {
            Add(*pa, *pb);
            pa++;
            pb++;
        }
    }

    ostream& operator<<(ostream& os, const LabelSimilarityScore& score) {
        if (score.L > 0) {
            os << score.L << " " << score.A << " " << score.AB << " " << score.B << " " << score.total() << " " << score.dice();
        }
        return os;
    }

    ostream& operator<<(ostream& os, const AtlasSimilarityScore& score) {
        for (int i = 0; i < score.labelMap.size(); i++) {
            if (score.labelMap[i].L > 0) {
                os << score.labelMap[i] << endl;
            }
        }
        return os;
    }



}