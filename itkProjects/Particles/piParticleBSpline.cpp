//
//  myParticleBSpline.cpp
//  ParticlesGUI
//
//  Created by Joohwi Lee on 1/25/13.
//
//

#include "piParticleBSpline.h"
#include "piImageDef.h"



namespace pi {
    LabelImage::Pointer ParticleBSpline::GetReferenceImage() {
        return m_RefImage;
    }

    void ParticleBSpline::SetReferenceImage(LabelImage::Pointer img) {
        m_RefImage = img;
    }

    void ParticleBSpline::EstimateTransform(const ParticleSubject& src, const ParticleSubject& dst) {
        this->EstimateTransform<ParticleXCaster, LabelImage, ParticleSubject>(src, dst, src.GetNumberOfPoints(), m_RefImage);
    }

    void ParticleBSpline::EstimateTransformY(const ParticleSubject& src, const ParticleSubject& dst) {
        this->EstimateTransform<ParticleYCaster, LabelImage, ParticleSubject>(src, dst, src.GetNumberOfPoints(), m_RefImage);
    }

    void ParticleBSpline::EstimateTransformYZ(const ParticleSubject& src, const ParticleSubject& dst) {
        this->EstimateTransform<ParticleYZCaster, LabelImage, ParticleSubject>(src, dst, src.GetNumberOfPoints(), m_RefImage);
    }

    void ParticleBSpline::EstimateTransformZ(const ParticleSubject& src, const ParticleSubject& dst) {
        this->EstimateTransform<ParticleZCaster, LabelImage, ParticleSubject>(src, dst, src.GetNumberOfPoints(), m_RefImage);
    }

    void ParticleBSpline::ApplyTransform(ParticleSubject& a) {

    }

    RealImage::Pointer ParticleBSpline::WarpImage(RealImage::Pointer srcImage) {
        if (m_DisplacementField.IsNull()) {
            return RealImage::Pointer(NULL);
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