//
//  myParticleBSpline.cpp
//  ParticlesGUI
//
//  Created by Joohwi Lee on 1/25/13.
//
//

#include "myParticleBSpline.h"
#include "myImageDef.h"
#include "itkBSplineScatteredDataPointSetToImageFilter.h"
#include "itkBSplineTransform.h"
#include "itkWarpImageFilter.h"


namespace pi {

    typedef itk::BSplineScatteredDataPointSetToImageFilter
    <DisplacementFieldPointSetType, DisplacementFieldType> BSplineFilterType;
    typedef BSplineFilterType::WeightsContainerType WeightsContainerType;
    typedef itk::BSplineTransform<double,__Dim,3> BSplineTransform;
    typedef itk::WarpImageFilter<DoubleImage, DoubleImage, DisplacementFieldType> WarpImageFilterType;


    LabelImage::Pointer ParticleBSpline::GetReferenceImage() {
        return m_RefImage;
    }

    void ParticleBSpline::SetReferenceImage(LabelImage::Pointer img) {
        m_RefImage = img;
    }

    void ParticleBSpline::EstimateTransform(const ParticleSubject& src, const ParticleSubject& dst) {
        int nPoints = src.GetNumberOfPoints();

        if (m_FieldPoints.IsNull()) {
            m_FieldPoints = DisplacementFieldPointSetType::New();
        }
        m_FieldPoints->Initialize();

        // create point structures
        IntPointSetType::Pointer srcPoints = IntPointSetType::New();
        IntPointSetType::Pointer dstPoints = IntPointSetType::New();

        srcPoints->Initialize();
        dstPoints->Initialize();

        int n = nPoints;
        for (int i = 0; i < n; i++) {
            IntPointSetType::PointType iPoint;
            fordim(j) {
                iPoint[j] = src[i].x[j];
            }
            VectorType vector;
            fordim(j) {
                vector[j] = dst[i].x[j] - src[i].x[j];
            }
            m_FieldPoints->SetPoint(i, iPoint);
            m_FieldPoints->SetPointData(i, vector);
        }

        int splineOrder = m_SplineOrder;
        int numOfLevels = m_SplineLevel;
        int nSize = m_ControlPoints;

        BSplineFilterType::Pointer bspliner = BSplineFilterType::New();
        BSplineFilterType::ArrayType numControlPoints;
        numControlPoints.Fill(nSize + splineOrder);

        DoubleImage::SizeType imageSize = m_RefImage->GetBufferedRegion().GetSize();
        DoubleImage::SpacingType imageSpacing = m_RefImage->GetSpacing();
        DoubleImage::PointType imageOrigin = m_RefImage->GetOrigin();

        try {
            // debug: reparameterized point component is outside
            bspliner->SetOrigin(imageOrigin);
            bspliner->SetSpacing(imageSpacing);
            bspliner->SetSize(imageSize);
            bspliner->SetGenerateOutputImage(true);
            bspliner->SetNumberOfLevels(numOfLevels);
            bspliner->SetSplineOrder(splineOrder);
            bspliner->SetNumberOfControlPoints(numControlPoints);
            bspliner->SetInput(m_FieldPoints);
            bspliner->Update();
            m_DisplacementField = bspliner->GetOutput();
        } catch (itk::ExceptionObject& e) {
            e.Print(std::cout);
        }
    }

    void ParticleBSpline::ApplyTransform(ParticleSubject& a) {

    }

    DoubleImage::Pointer ParticleBSpline::WarpImage(DoubleImage::Pointer srcImage) {
        if (m_DisplacementField.IsNull()) {
            return DoubleImage::Pointer(NULL);
        }
        WarpImageFilterType::Pointer warpFilter = WarpImageFilterType::New();
        warpFilter->SetInput(srcImage);
        warpFilter->SetDisplacementField(m_DisplacementField);
        warpFilter->SetOutputParametersFromImage(srcImage);
        warpFilter->Update();
        return warpFilter->GetOutput();
    }

    LabelImage::Pointer ParticleBSpline::WarpLabel(LabelImage::Pointer srcImage) {
        if (m_DisplacementField.IsNull()) {
            return LabelImage::Pointer(NULL);
        }
        typedef itk::WarpImageFilter<LabelImage, LabelImage, DisplacementFieldType> WarpLabelFilterType;
        WarpLabelFilterType::Pointer warpFilter = WarpLabelFilterType::New();
        warpFilter->SetInput(srcImage);
        warpFilter->SetInterpolator(NNLabelInterpolatorType::New());
        warpFilter->SetDisplacementField(m_DisplacementField);
        warpFilter->SetOutputParametersFromImage(srcImage);
        warpFilter->Update();
        return warpFilter->GetOutput();
    }

    FieldTransformType::Pointer ParticleBSpline::GetTransform() {
        FieldTransformType::Pointer txf = FieldTransformType::New();
        if (m_DisplacementField.IsNotNull()) {
            txf->SetDisplacementField(m_DisplacementField);
            return txf;
        }
        return FieldTransformType::Pointer(NULL);
    }
}