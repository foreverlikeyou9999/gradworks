//
//  piImagePatch.h
//  ParticleGuidedRegistration
//
//  Created by Joohwi Lee on 4/11/13.
//
//

#ifndef __ParticleGuidedRegistration__piImagePatch__
#define __ParticleGuidedRegistration__piImagePatch__

#include <iostream>
#include <vector>
#include <cmath>

#include <itkLinearInterpolateImageFunction.h>
#include <itkTransform.h>

namespace pi {
    template <class T>
    class ImagePatch {
    public:
        typedef T* ImagePointer;
        typedef typename T::PixelType PixelType;
        typedef typename T::PointType PointType;
        typedef typename T::IndexType IndexType;
        typedef typename T::RegionType RegionType;

        typedef struct {
            IndexType i;
            PointType q;
        } IndexPointType;

        typedef itk::Transform<double,T::ImageDimension,T::ImageDimension> TransformType;
        typedef TransformType* TransformPointer;
        typedef std::vector<PointType> PointVector;
        typedef std::vector<IndexPointType> IndexPointVector;
        typedef itk::LinearInterpolateImageFunction<T> ImageInterpolator;

        ImagePatch();
        ~ImagePatch();

        PointVector& getOffsetPoints();
        IndexPointVector& getSamplingPoints();
        
        void setImage(ImagePointer image);
        void setRadiusInIndexUnit(int radius);
        void setTransform(TransformPointer transform);

        
        void addSamplingIndex(IndexType& idx);
        void addSamplingPoint(PointType& point);
        void samplePoints(int i, PixelType*);
        bool samplePoints(int i, ImagePointer outputImage);
        
    private:
        void updatePhysicalPoints(unsigned int, int&);
        bool evaluateAtPhysicalPoint(PointType& p, PixelType& pixelOut);
        bool evaluateAtIndex(IndexType& x, PixelType& pixelOut);
        bool transformAndEvaluatePoint(PointType& p, PointType& q, PixelType& pixelOut);

    private:
        int _radius;
        int _width;
        ImagePointer _image;
        IndexType _indexLoop;
        TransformPointer _transform;
        PointVector _offsetPoints;
        IndexPointVector _samplingPoints;
        typename ImageInterpolator::Pointer _interpolator;
    };

    template <class T>
    ImagePatch<T>::ImagePatch() {
        _image = NULL;
        _transform = NULL;
    }

    template <class T>
    ImagePatch<T>::~ImagePatch() {
    }

    template <class T>
    void ImagePatch<T>::setImage(ImagePointer image) {
        _image = image;
        _interpolator = ImageInterpolator::New();
        _interpolator->SetInputImage(image);
    }

    template <class T>
    typename ImagePatch<T>::PointVector& ImagePatch<T>::getOffsetPoints() {
        return _offsetPoints;
    }

    template <class T>
    typename ImagePatch<T>::IndexPointVector& ImagePatch<T>::getSamplingPoints() {
        return _samplingPoints;
    }

    template <class T>
    void ImagePatch<T>::setRadiusInIndexUnit(int radius) {
        _radius = radius;
        _width = 2 * _radius + 1;
        if (_image != NULL) {
            int dims = T::ImageDimension;
            int nElems = std::pow(float(_width), dims);
            _offsetPoints.resize(nElems);
            int n = 0;
            updatePhysicalPoints(dims - 1, n);
        }
    }

    template <class T>
    void ImagePatch<T>::updatePhysicalPoints(unsigned int dim, int& n) {
        _indexLoop[dim] = -_radius;
        for (unsigned int i = 0; i < _width; i++) {
            if (dim > 0) {
                updatePhysicalPoints(dim - 1, n);
            } else {
                _image->TransformIndexToPhysicalPoint(_indexLoop, _offsetPoints[n++]);
            }
            _indexLoop[dim]++;
        }
    }

    template <class T>
    inline bool ImagePatch<T>::evaluateAtPhysicalPoint(PointType& p, PixelType& pixelOut) {
        if (!_interpolator->IsInsideBuffer(p)) {
            return false;
        }
        pixelOut = _interpolator->Evaluate(p);
    }
    
    template <class T>
    inline bool ImagePatch<T>::evaluateAtIndex(IndexType& x, PixelType& pixelOut) {
        if (!_interpolator->IsInsideBuffer(x)) {
            return false;
        }
        pixelOut = _interpolator->EvaluateIndex(x);
    }

    template <class T>
    inline bool ImagePatch<T>::transformAndEvaluatePoint(PointType& p, PointType& q, PixelType& v) {
        // do not check transform
        _transform->TransformPoint(p, q);
        return evaluateAtPhysicalPoint(q, v);
    }

    template <class T>
    void ImagePatch<T>::samplePoints(int n, PixelType* pixelBuffer) {
        if (n >= _samplingPoints.size()) {
            return;
        }
        PointType& center = _samplingPoints[n];
        typename PointVector::iterator iter = _offsetPoints.begin();
        for (; iter != _offsetPoints.end(); iter++) {
            PointType p;
            PointType q;
            for (int j = 0; j < T::ImageDimension; j++) {
                p[j] = *iter[j] + center[j];
            }
            _transform->TransformPoint(p, q);
            evaluateAtPhysicalPoint(q, *pixelBuffer);
            pixelBuffer++;
        }
    }

    template <class T>
    bool ImagePatch<T>::samplePoints(int n, ImagePointer outputImage) {
        if (_offsetPoints.size() != outputImage->GetPixelContainer()->Size()) {
            return false;
        }
        samplePoints(n, outputImage->GetBufferPointer());
        return true;
    }

    template <class T>
    void ImagePatch<T>::addSamplingPoint(PointType& p) {
        IndexType ip;
        ip.p = p;
        _image->TransformPhysicalPointToIndex(p, ip.i);
        _samplingPoints.push_back(p);
    }

    template <class T>
    void ImagePatch<T>::addSamplingIndex(IndexType& x) {
        IndexPointType ip;
        ip.i = x;
        _image->TransformIndexToPhysicalPoint(ip.x, ip.p);
        _samplingPoints.push_back(ip);
    }

}
#endif /* defined(__ParticleGuidedRegistration__piImagePatch__) */