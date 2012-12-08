//
//  BSplineRegistration.cpp
//  ParticlesGUI
//
//  Created by Joohwi Lee on 12/6/12.
//
//

#include "myBSplineRegistration.h"
#include "QElapsedTimer"
#include "itkWarpImageFilter.h"
#include "itkVectorMagnitudeImageFilter.h"
#include "iostream"

// estimation of displacement field via particle correspondence

using namespace std;

namespace my {
    typedef itk::WarpImageFilter<SliceType, SliceType, DisplacementFieldType> WarpImageFilterType;

    BSplineRegistration::BSplineRegistration() {

    }

    BSplineRegistration::~BSplineRegistration() {

    }

    void BSplineRegistration::SetReferenceImage(SliceType::Pointer refImage) {
        m_RefImage = refImage;
    }

    void BSplineRegistration::SetLandmarks(int n, double *src, double *dst) {
        if (m_FieldPoints.IsNull()) {
            m_FieldPoints = DisplacementFieldPointSetType::New();
        }
        m_FieldPoints->Initialize();

        // create point structures
        PointSetType::Pointer srcPoints = PointSetType::New();
        PointSetType::Pointer dstPoints = PointSetType::New();

        srcPoints->Initialize();
        dstPoints->Initialize();

        double* pSrc = src;
        double* pDst = dst;
        for (int i = 0; i < n; i++) {
            PointSetType::PointType iPoint;
            iPoint[0] = pSrc[0];
            iPoint[1] = pSrc[1];

            VectorType vector;
            vector[0] = pDst[0] - pSrc[0];
            vector[1] = pDst[1] - pSrc[1];
            m_FieldPoints->SetPoint(i, iPoint);
            m_FieldPoints->SetPointData(i, vector);
            pSrc += 2;
            pDst += 2;
        }
    }

    void BSplineRegistration::Update() {
        int splineOrder = 3;
        int numOfLevels = 3;
        int nSize = 25;

        BSplineFilterType::Pointer bspliner = BSplineFilterType::New();
        BSplineFilterType::ArrayType ncps;
        ncps.Fill(nSize + splineOrder);

        try {
            // debug: reparameterized point component is outside
            cout << "Field Points: " << endl;
            for (int i = 0; i < m_FieldPoints->GetNumberOfPoints(); i++) {
                DisplacementFieldPointSetType::PointType iPoint = m_FieldPoints->GetPoint(i);
                cout << iPoint << endl;
            }
            QElapsedTimer timer;
            timer.start();
            bspliner->SetOrigin(m_RefImage->GetOrigin());
            bspliner->SetSpacing(m_RefImage->GetSpacing());
            bspliner->SetSize(m_RefImage->GetBufferedRegion().GetSize());
            bspliner->SetGenerateOutputImage(true);
            bspliner->SetNumberOfLevels(numOfLevels);
            bspliner->SetSplineOrder(splineOrder);
            bspliner->SetNumberOfControlPoints(ncps);
            bspliner->SetInput(m_FieldPoints);
            bspliner->Update();
            m_DisplacementField = bspliner->GetOutput();
            cout << "BSpline Update Time: " << timer.elapsed() << endl;
        } catch (itk::ExceptionObject& e) {
            e.Print(std::cout);
        }
    }

    SliceType::Pointer BSplineRegistration::WarpImage(SliceType::Pointer srcImage) {
        if (m_DisplacementField.IsNull()) {
            return SliceType::Pointer(NULL);
        }
        WarpImageFilterType::Pointer warpFilter = WarpImageFilterType::New();
        warpFilter->SetInput(srcImage);
        warpFilter->SetDisplacementField(m_DisplacementField);
        warpFilter->SetOutputSpacing(srcImage->GetSpacing());
        warpFilter->SetOutputOrigin(srcImage->GetOrigin());
        warpFilter->SetOutputSize(srcImage->GetBufferedRegion().GetSize());
        warpFilter->Update();
        return warpFilter->GetOutput();
    }

    DisplacementFieldType::Pointer BSplineRegistration::GetDisplacementField() {
        return m_DisplacementField;
    }
    
    SliceType::Pointer BSplineRegistration::GetDisplacementMagnitude() {
        if (m_DisplacementField.IsNull()) {
            return SliceType::Pointer(NULL);
        }

        typedef itk::VectorMagnitudeImageFilter<DisplacementFieldType, SliceType> VectorFilterType;
        VectorFilterType::Pointer filter = VectorFilterType::New();
        filter->SetInput(m_DisplacementField);
        filter->Update();
        return filter->GetOutput();
    }

}