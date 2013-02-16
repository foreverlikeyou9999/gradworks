#include "iostream"
#include "sstream"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer.hpp>

#include "piParticleCore.h"
#include "piParticleConstraint.h"
#include "piParticleBSpline.h"
#include "piParticleForces.h"
#include "piParticleCollision.h"

namespace pi {
    ostream& operator<<(ostream& os, const Particle& par) {
        for4(k) { os << par.x[k] << " "; }
        for4(k) { os << par.y[k] << " "; }
        for4(k) { os << par.v[k] << " "; }
        for4(k) { os << par.f[k] << " "; }
        os << par.density << " ";
        os << par.pressure << " ";
        return os;
    }

    istream& operator>>(istream& is, Particle& par) {
        for4(k) { is >> par.x[k]; }
        for4(k) { is >> par.y[k]; }
        for4(k) { is >> par.v[k]; }
        for4(k) { is >> par.f[k]; }
        is >> par.density;
        is >> par.pressure;
        return is;
    }

    // constructor
    // set every member variable as zero
    Particle::Particle() {
        Zero();
    }

    Particle::~Particle() {

    }

    void Particle::Zero() {
        t = 0;
        for4(j) {
            x[j] = y[j] = v[j] = f[j] = w[j] = 0;
        }
        density = pressure = 0;
    }

    void Particle::Sub(const Particle& p, double* d) {
        fordim(i) {
            d[i] = x[i] - p.x[i];
        }
    }

    void Particle::AddForce(const double* ff, double alpha) {
        fordim(i) {
            f[i] += (alpha * ff[i]);
        }
    }

    void Particle::SubForce(const double* ff, double alpha) {
        fordim(i) {
            f[i] -= (alpha * ff[i]);
        }
    }

    double Particle::Dist2(const Particle& p) {
        double d[__Dim];
        fordim(i) {
            d[i] = x[i] - p.x[i];
        }
        double dist2 = 0;
        fordim(k) {
            dist2 += (d[k]*d[k]);
        }
        return dist2;
    }

    void Particle::UpdateForce(double *ff) {
        fordim(i) {
            f[i] = ff[i];
        }
    }

    void Particle::UpdateVelocity(double *vv) {
        fordim(i) {
            v[i] = vv[i];
        }
    }

    void Particle::UpdateSystem(double dt) {
        fordim(i) {
            x[i] += dt * v[i];
            v[i] += dt * f[i];
        }
    }

    Particle& Particle::operator=(const Particle& other) {
        for4(i) {
            x[i] = other.x[i];
            y[i] = other.y[i];
            v[i] = other.v[i];
            f[i] = other.f[i];
            density = other.density;
            pressure = other.pressure;
        }
        return (*this);
    }

    ParticleSubject::ParticleSubject(int subjid, int npoints) : m_SubjId(subjid) {
        NewParticles(npoints);
    }

    ParticleSubject::~ParticleSubject() {

    }

    void ParticleSubject::Clear() {
        m_Particles.clear();
        m_SubjId = -1;
        m_InverseDeformableTransform = m_DeformableTransform = FieldTransformType::Pointer(NULL);
    }

    void ParticleSubject::Zero() {
        const int nPoints = GetNumberOfPoints();
        for (int i = 0; i < nPoints; i++) {
            m_Particles[i].idx = i;
            m_Particles[i].Zero();
        }
    }

    void ParticleSubject::NewParticles(int n) {
        m_Particles.resize(n);
        Zero();
    }

    void ParticleSubject::InitializeRandomPoints(LabelImage::Pointer intersection) {
        // resize particles array
        const int nPoints = GetNumberOfPoints();

        // compute intersection by looping over region
        std::vector<LabelImage::IndexType> indexes;
        LabelImageIteratorType iter(intersection, intersection->GetBufferedRegion());
        for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
            LabelImage::IndexType idx = iter.GetIndex();
            LabelImage::PixelType pixel = iter.Value();
            if (pixel > 0) {
                indexes.push_back(idx);
            }
        }
        if (indexes.size() > 0) {
            std::random_shuffle(indexes.begin(), indexes.end());
        }


        for (int i = 0; i < nPoints; i++) {
            LabelImage::IndexType idx = indexes[i];
            m_Particles[i].idx = i;
            m_Particles[i].Set(idx);
        }
    }

    void ParticleSubject::Initialize(int subj, std::string name, int nPoints) {
        m_SubjId = subj;
        if (name != "") {
            m_Name = name;
        }
        m_Particles.resize(nPoints);
        Zero();
    }
    
    void ParticleSubject::Initialize(int subj, std::string name, const ParticleSubject& shape) {
        m_SubjId = subj;
        if (name != "") {
            m_Name = name;
        }
        const int nPoints = shape.GetNumberOfPoints();
        m_Particles.resize(nPoints);
        for (int i = 0; i < nPoints; i++) {
            m_Particles[i] = shape.m_Particles[i];
            m_Particles[i].idx = i;
        }
    }

    void ParticleSubject::Initialize(const ParticleArray& array) {
        const int nPoints = array.size();
        if (GetNumberOfPoints() != nPoints) {
            m_Particles.resize(nPoints);
        }

        for (int i = 0; i < nPoints; i++) {
            m_Particles[i] = array[i];
            m_Particles[i].idx = i;
            m_Particles[i].subj = m_SubjId;
        }
    }

    void ParticleSubject::ApplyMatrix(VNLMatrix &mat) {

    }

    void ParticleSubject::TransformX2Y(TransformType* transform) {
        const int nPoints = GetNumberOfPoints();

        if (transform != NULL) {
            for (int i = 0; i < nPoints; i++) {
                TransformType::InputPointType inputPoint;
                fordim (j) {
                    inputPoint[j] = m_Particles[i].x[j];
                }
                TransformType::OutputPointType outputPoint = transform->TransformPoint(inputPoint);
                fordim (j) {
                    m_Particles[i].y[j] = outputPoint[j];
                }
            }
        } else {
            for (int i = 0; i < nPoints; i++) {
                for4(j) {
                    m_Particles[i].y[j] = m_Particles[i].x[j];
                }
            }
        }
    }

    void ParticleSubject::TransformY2X(TransformType* transform) {
        const int nPoints = GetNumberOfPoints();

        if (transform != NULL) {
            for (int i = 0; i < nPoints; i++) {
                TransformType::InputPointType inputPoint;
                fordim (j) {
                    inputPoint[j] = m_Particles[i].y[j];
                }
                TransformType::OutputPointType outputPoint = transform->TransformPoint(inputPoint);
                fordim (j) {
                    m_Particles[i].x[j] = outputPoint[j];
                }
            }
        } else {
            for (int i = 0; i < nPoints; i++) {
                for4(j) {
                    m_Particles[i].x[j] = m_Particles[i].y[j];
                }
            }
        }
    }

    void ParticleSubject::TransformX2X(TransformType* transform) {
        const int nPoints = GetNumberOfPoints();
        if (transform != NULL) {
            TransformType::InputPointType inputPoint;
            for (int i = 0; i < nPoints; i++) {
                fordim (j) {
                    inputPoint[j] = m_Particles[i].x[j];
                }
                TransformType::OutputPointType outputPoint = transform->TransformPoint(inputPoint);
                fordim (j) {
                    m_Particles[i].x[j] = outputPoint[j];
                }
            }
        }
    }

    void ParticleSubject::TransformY2Y(TransformType* transform) {
        const int nPoints = GetNumberOfPoints();
        if (transform != NULL) {
            for (int i = 0; i < nPoints; i++) {
                TransformType::InputPointType inputPoint;
                fordim (j) {
                    inputPoint[j] = m_Particles[i].y[j];
                }
                TransformType::OutputPointType outputPoint = transform->TransformPoint(inputPoint);
                fordim (j) {
                    m_Particles[i].y[j] = outputPoint[j];
                }
            }
        }
    }

    void ParticleSubject::ReadParticlePositions(std::istream& is, int nPoints) {
        if (m_Particles.size() != nPoints) {
            m_Particles.resize(nPoints);
        }
        for (int i = 0; i < nPoints; i++) {
            m_Particles[i].idx = i;
            m_Particles[i].t = 0;
            m_Particles[i].subj = m_SubjId;
            for4(k) {
                is >> m_Particles[i].x[k];
            }
        }
    }
    
    void ParticleSubject::WriteParticlePositions(std::ostream& os) {
        for (int i = 0; i < m_Particles.size(); i++) {
            for4(k) {
                os << m_Particles[i].x[k] << "  ";
            }
            os << endl;
        }
    }
    
    void ParticleSubject::ReadParticles(std::istream& is, int nPoints) {
        if (m_Particles.size() != nPoints) {
            m_Particles.resize(nPoints);
        }
        for (int i = 0; i < nPoints; i++) {
            m_Particles[i].idx = i;
            m_Particles[i].t = 0;
            m_Particles[i].subj = m_SubjId;
            is >> m_Particles[i];
        }
    }
    
    void ParticleSubject::WriteParticles(std::ostream& os) {
        for (int i = 0; i < m_Particles.size(); i++) {
            os << m_Particles[i] << endl;
        }
    }

    void ImageContext::Clear() {
        m_FileNames.clear();
        m_LabelImages.clear();
        m_DistanceMaps.clear();
    }

    void ImageContext::LoadLabel(std::string filename) {
        itkcmds::itkImageIO<LabelImage> io;
        LabelImage::Pointer image = io.ReadImageT(filename.c_str());
        m_LabelImages.push_back(image);

        // set default spacing to 1 to match index and physical coordinate space
        LabelImage::SpacingType defaultSpacing;
        defaultSpacing.Fill(1);
        m_LabelImages.back()->SetSpacing(defaultSpacing);

        m_FileNames.push_back(filename);
    }

    void ImageContext::LoadDoubleImage(std::string filename) {
        itkcmds::itkImageIO<DoubleImage> io;
        DoubleImage::Pointer image = io.ReadImageT(filename.c_str());
        m_Images.push_back(image);

        // set default spacing to 1 to match index and physical coordinate space
        DoubleImage::SpacingType defaultSpacing;
        defaultSpacing.Fill(1);
        m_Images.back()->SetSpacing(defaultSpacing);
        
        m_DoubleImageFileNames.push_back(filename);
    }

    LabelImage::Pointer ImageContext::GetLabel(int j) {
        return m_LabelImages[j];
    }

    DoubleImage::Pointer ImageContext::GetDoubleImage(int j) {
        return m_Images[j];
    }

    LabelImage::Pointer ImageContext::GetIntersection() {
        return m_Intersection;
    }
    
    void ImageContext::SetIntersection(LabelImage::Pointer intersection) {
        m_Intersection = intersection;
    }

    void ImageContext::ComputeIntersection() {
        itkcmds::itkImageIO<LabelImage> io;
        LabelImage::Pointer intersection = io.NewImageT(m_LabelImages[0]);
        LabelImage::RegionType region = intersection->GetBufferedRegion();

        // set as member variable to reuse
        m_Intersection = intersection;

        // compute intersection by looping over region
        std::vector<LabelImage::IndexType> indexes;
        LabelImageIteratorType iter(intersection, region);
        for (iter.GoToBegin(); !iter.IsAtEnd(); ++iter) {
            LabelImage::IndexType idx = iter.GetIndex();
            LabelImage::PixelType pixel = 255;
            const int pixelThreshold = 1;
            for (int i = 0; i < m_LabelImages.size(); i++) {
                if (m_LabelImages[i]->GetPixel(idx) < pixelThreshold) {
                    pixel = 0;
                    break;
                }
            }
            if (pixel > 0) {
                intersection->SetPixel(idx, 255);
            }
        }
        if (m_IntersectionOutput != "") {
            io.WriteImageT(m_IntersectionOutput.c_str(), intersection);
        }
    }
    
    StringVector& ImageContext::GetDoubleImageFileNames() {
        return m_DoubleImageFileNames;
    }
    
    StringVector& ImageContext::GetFileNames() {
        return m_FileNames;
    }
    
    LabelVector& ImageContext::GetLabelVector() {
        return m_LabelImages;
    }
    
    DoubleImageVector& ImageContext::GetDoubleImageVector() {
        return m_Images;
    }

    ParticleSystem::ParticleSystem()  {
    }

    int ParticleSystem::GetNumberOfSubjects() {
        return m_Subjects.size();
    }

    int ParticleSystem::GetNumberOfParticles() {
        if (m_Subjects.size() > 0) {
            return m_Subjects[0].GetNumberOfPoints();
        }
        return 0;
    }

    
    void ParticleSystem::InitializeSystem(Options& options) {
        m_Options = options;
        m_Subjects.resize(options.GetStringVector("Subjects:").size());
        for (int i = 0; i < m_Subjects.size(); i++) {
            m_Subjects[i].Initialize(i, options.GetStringVectorValue("Subjects:", i), options.GetInt("NumberOfParticles:", 0));
        }
    }

    ParticleSubject& ParticleSystem::GetInitialSubject() {
        return m_InitialSubject;
    }

    void ParticleSystem::ComputeMeanSubject() {
        const int nPoints = m_Subjects[0].GetNumberOfPoints();
        const int nSubjects = m_Subjects.size();

        m_MeanSubject.m_SubjId = -1;
        m_MeanSubject.NewParticles(nPoints);

        // for every dimension k
        fordim(k) {
            // for every point i
            for (int i = 0; i < nPoints; i++) {
                // sum over all subject j
                for (int j = 0; j < nSubjects; j++) {
                    m_MeanSubject[i].x[k] += m_Subjects[j][i].x[k];
                }
                m_MeanSubject[i].x[k] /= nSubjects;
            }
        }
    }

    const ParticleSubject& ParticleSystem::GetMeanSubject() const {
        return m_MeanSubject;
    }

    ParticleSubjectArray& ParticleSystem::GetSubjects() {
        return m_Subjects;
    }

/*
 static bool LoadSystem(std::string filename) {
        using namespace std;
        int nSubjects = 0;

        m_ImageContext.Clear();
        m_TrackingOutputPattern = "";

        m_Subjects.clear();
        m_Initial.clear();

        char cbuf[256];
        ifstream in(filename.c_str());
        if (in.is_open()) {
            string name;
            while (in.good()) {
                in.getline(cbuf, sizeof(cbuf));
                cout << cbuf << endl;
                if (in.good()) {
                    stringstream ss(cbuf);
                    ss >> name;
                    if (name[0] == '#') {
                        continue;
                    } else if (name == "TrackingOutputPattern:") {
                        ss >> m_TrackingOutputPattern;
                        m_Options.Set(name, m_TrackingOutputPattern);
                        cout << "TrackingOutputPattern => " << m_TrackingOutputPattern << endl;
                    } else if (name == "ParticleDimension:") {
                        int value;
                        ss >> value;
                        if (value != __Dim) {
                            cout << "Particle Domain Mismatch!" << endl;
                            throw "Particle Dimension Mismatch!";
                        }
                    } else if (name == "Forces:") {
                        string option;
                        m_InternalForceFlag = m_EnsembleForceFlag = m_IntensityForceFlag = false;
                        while (ss.good()) {
                            ss >> option;
                            if (option == "+internal") {
                                m_InternalForceFlag = true;
                            } else if (option == "+ensemble") {
                                m_EnsembleForceFlag = true;
                            } else if (option == "+intensity") {
                                m_IntensityForceFlag = true;
                            }
                        }
                    } else if (name == "ForceCoefficients:") {
                        ss >> m_EnsembleCoeff >> m_IntensityCoeff;
                        m_Options.AppendDouble(name, m_EnsembleCoeff);
                        m_Options.AppendDouble(name, m_IntensityCoeff);
                    } else if (name == "IntersectionImage:") {
                        ss >> m_ImageContext.m_IntersectionOutput;
                        m_Options.Set(name, m_ImageContext.m_IntersectionOutput);
                    } else if (name == "NumParticlesPerSubject:") {
                        ss >> m_NumParticlesPerSubject;
                        m_Options.Set(name, m_NumParticlesPerSubject);
                    } else if (name == "TimeRange:") {
                        ss >> m_times[0] >> m_times[1] >> m_times[2];
                        m_Options.AppendDouble(name, m_times[0]);
                        m_Options.AppendDouble(name, m_times[1]);
                        m_Options.AppendDouble(name, m_times[2]);
                    } else if (name == "PreprocessingTimeRange:") {
                        ss >> m_timesPreprocessing[0] >> m_timesPreprocessing[1] >> m_timesPreprocessing[2];
                        m_Options.AppendDouble(name, m_timesPreprocessing[0]);
                        m_Options.AppendDouble(name, m_timesPreprocessing[1]);
                        m_Options.AppendDouble(name, m_timesPreprocessing[2]);
                    } else if (name == "LabelImages:") {
                        int value;
                        ss >> value;
                        for (int i = 0; i < value; i++) {
                            char buf[128];
                            in.getline(buf, sizeof(buf));
                            if (in.good()) {
                                try {
                                    m_ImageContext.LoadLabel(buf);
                                } catch (itk::ExceptionObject& ex) {
                                    ex.Print(cout);
                                    exit(0);
                                }
                            }
                            //cout << in.exceptions() << endl;
                        }
                    } else if (name == "IntensityImages:") {
                        int value;
                        ss >> value;
                        for (int i = 0; i < value; i++) {
                            char buf[128];
                            in.getline(buf, sizeof(buf));
                            if (in.good()) {
                                m_ImageContext.LoadDoubleImage(buf);
                                m_Options.AppendString(name, buf);
                            }
                        }
                    } else if (name == "Subjects:") {
                        int value;
                        ss >> value;
                        nSubjects = value;

                        m_Subjects.clear();
                        m_Subjects.resize(nSubjects);
                        for (int i = 0; i < nSubjects; i++) {
                            char buf[128];
                            in.getline(buf, sizeof(buf));
                            m_Subjects[i].m_Name = buf;
                            m_Options.AppendString(name, buf);
                        }
                    } else if (name == "InitialParticles:") {
                        m_Initial.resize(1);
                        for (int i = 0; i < m_Initial.size(); i++) {
                            m_Initial[i].NewParticles(m_NumParticlesPerSubject);
                            for (int j = 0; j < m_NumParticlesPerSubject; j++) {
                                Particle& p = m_Initial[i][j];
                                char buf[128];
                                in.getline(buf, sizeof(buf));
                                if (in.good())
                                {
                                    stringstream ss(buf);
                                    for4(k) { ss >> p.x[k]; }
                                }
                            }
                        }

                    } else if (name == "Particles:") {
                        int subjId;
                        ss >> subjId;
                        int nParticles;
                        ss >> nParticles;
                        if (nParticles > 0) {
                            cout << "Reading " << nParticles << " particles ..." << endl;
                            if (m_Subjects[subjId].GetNumberOfPoints() != nParticles) {
                                m_Subjects[subjId].NewParticles(m_NumParticlesPerSubject);
                            }
                            for (int j = 0; j < nParticles; j++) {
                                Particle& p = m_Subjects[subjId][j];
                                char buf[128];
                                in.getline(buf, sizeof(buf));
                                if (in.good()) {
                                    stringstream ss(buf);
                                    ss >> p;
                                }
                            }
                        } else if (nParticles == 0) {
                            cout << "Using " << m_Initial[0].GetNumberOfPoints() << " initial particles ..." << endl;
                            m_Subjects[subjId].Initialize(subjId, "", m_Initial[0]);
                        }
                    } else {
                        string kind;
                        ss >> kind;
                        if (kind == "strings") {
                            int nValues;
                            ss >> nValues;
                            for (int i = 0; i < nValues; i++) {
                                char buf[128];
                                in.getline(buf, sizeof(buf));
                                if (in.good()) {
                                    m_Options.AppendString(name, buf);
                                } else {
                                    break;
                                }
                            }
                        } else if (kind == "doubles") {
                            while (ss.good()) {
                                double d;
                                ss >> d;
                                m_Options.AppendDouble(name, d);
                            }
                        }
                    }
                }
            }
        } else {
            return false;
        }
        return true;
    }

    static void SaveSystem(std::string filename) {
        using namespace std;
        ofstream out(filename.c_str());

        out << "ParticleDimension: " << __Dim << endl;
        out << "Forces: ";
        if (m_InternalForceFlag) {
            out << "+internal ";
        } else {
            out << "-internal ";
        }
        if (m_EnsembleForceFlag) {
            out << "+ensemble ";
        } else {
            out << "-ensemble ";
        }
        if (m_IntensityForceFlag) {
            out << "+intensity ";
        } else {
            out << "-intensity ";
        }
        out << endl;

        out << "ForceCoefficients: " << m_EnsembleCoeff << " " << m_IntensityCoeff << endl;

        out << "NumParticlesPerSubject: " << m_NumParticlesPerSubject << endl;

        // Time Range
        out << "PreprocessingTimeRange: " << m_timesPreprocessing[0] << " " << m_timesPreprocessing[1] << " " << m_timesPreprocessing[2] << endl;

        out << "TimeRange: " << m_times[0] << " " << m_times[1] << " " << m_times[2] << endl;

        // Tracking output pattern
        if (m_TrackingOutputPattern != "") {
            out << "TrackingOutputPattern: " << m_TrackingOutputPattern << endl;
        }

        // Intersection file name
        if (m_ImageContext.m_IntersectionOutput != "") {
            out << "IntersectionImage: " << m_ImageContext.m_IntersectionOutput << endl;
        }

        const int nSubjects = m_Subjects.size();
        out << "Subjects: " << nSubjects << endl;
        for (int i = 0; i < nSubjects; i++) {
            out << m_Subjects[i].m_Name << endl;
        }

        // Label Images
        StringVector& labels = m_ImageContext.GetFileNames();
        if (labels.size() > 0) {
            out << "LabelImages: " << labels.size() << endl;
            for (int i = 0; i < labels.size(); i++) {
                out << labels[i] << endl;
            }
        }

        // Intensity Images
        StringVector& images = m_ImageContext.GetDoubleImageFileNames();
        if (images.size() > 0) {
            out << "IntensityImages: " << images.size() << endl;
            for (int i = 0; i < images.size(); i++) {
                out << images[i] << endl;
            }
        }

        // Initial Particles
        out << "InitialParticles: " << m_NumParticlesPerSubject << endl;
        for (int i = 0; i < m_Initial.size(); i++) {
            for (int j = 0; j < m_NumParticlesPerSubject; j++) {
                Particle& p = m_Initial[i][j];
                for4(k) { out << p.x[k] << " "; }
                out << endl;
            }
        }

        // Particles
        //        cout << "Subjects: " << m_Subjects.size() << endl;
        for (int i = 0; i < m_Subjects.size(); i++) {
            int nParticles = m_Subjects[i].GetNumberOfPoints();
            out << "Particles: " << i << " " << nParticles << endl;
            for (int j = 0; j < nParticles; j++) {
                Particle& p = m_Subjects[i][j];
                out << p << endl;
            }
        }
        out.close();
    }
*/
}
