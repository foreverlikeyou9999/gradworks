//
//  myParticleDynamics.cpp
//  ParticlesGUI
//
//  Created by Joohwi Lee on 11/29/12.
//
//

#include "myParticleDynamics.h"
#include "boost/numeric/odeint.hpp"
#include "vnl/algo/vnl_symmetric_eigensystem.h"
#include "iostream"
#include "myImageParticlesAlgorithm.h"


// experiment options
const static bool applySurfaceEntropyToFirstOnly = false;
const static bool applyBoundaryConditionToFirstOnly = false;
const static bool useEnsembleForce = false;
const static bool useParticlePhysics = true;


using namespace std;
const static int nDim = 2;

// VNL-Boost compatibility functions
namespace boost {
    namespace numeric {
        namespace odeint {
            typedef VNLVector state_type;

            template <>
            struct is_resizeable<state_type> {
                typedef boost::true_type type;
                static const bool value = type::value;
            };

            template<>
            struct same_size_impl<state_type, state_type> { // define how to check size
                static bool same_size(const state_type &v1, const state_type &v2) {
                    return v1.size() == v2.size();
                }
            };

            template<>
            struct resize_impl<state_type , state_type>
            { // define how to resize
                static void resize( state_type &v1 ,
                                   const state_type &v2) {
                    v1.set_size(v2.size());
                }
            };
        }
    }
}

#define __dist2(x1,y1,x2,y2) ((x1-y1)*(x1-y1)+(x2-y2)*(x2-y2))

static inline double dist2(VNLMatrix& m, int r, int p1, int p2, int d = 2) {
    return __dist2(m[r][d*p1], m[r][d*p1+1], m[r][d*p2], m[r][d*p2+1]);
}

ParticleSystem::ParticleSystem(const int nSubj, const int nParticles): m_nDim(2), m_nSubjects(nSubj), m_nParticles(nParticles), m_nParams(nParticles*m_nDim) {
    m_Cutoff = 15;
    m_Sigma2 = 3*3;
    m_Mu = 1;
    m_COR = .2

    		;
    m_Force.set_size(m_nSubjects, m_nParams);
    m_Constraint = NULL;
    m_StatusHistory = NULL;
    m_Callback = NULL;
    m_Context = NULL;
}

void ParticleSystem::SetPositions(OptimizerParametersType* params) {
    m_Status.set_size(m_nSubjects*m_nParams*2);
    m_Status.fill(0);
    params->copy_out(m_Status.data_block());

//    VNLMatrixRef gPos(m_nSubject, m_nParams, m_Status.data_block() + m_nSubject * m_nParams);
//    gPos.fill(1);

}

void ParticleSystem::GetPositions(OptimizerParametersType* params) {
    VNLMatrixRef pos(m_nSubjects, m_nParams, m_Status.data_block());
    pos.copy_out(params->data_block());
}

void ParticleSystem::SetHistoryVector(VNLVectorArray* statusHistory) {
    m_StatusHistory = statusHistory;
}

void ParticleSystem::SetCostHistoryVector(STDDoubleArray* costHistory) {
    m_CostHistory = costHistory;
}

void ParticleSystem::SetConstraint(myImplicitSurfaceConstraint* constraint) {
    m_Constraint = constraint;
}

void ParticleSystem::SetEventCallback(EventCallback* callback) {
    m_Callback = callback;
}

void ParticleSystem::SetContext(ImageParticlesAlgorithm* context) {
    m_Context = context;

    m_Cutoff = context->GetProperty().GetDouble("cutoffDistance", 15.0);
    m_Sigma2  = context->GetProperty().GetDouble("sigma", 3.0);
    m_Sigma2 *= m_Sigma2;
}

void ParticleSystem::UpdateSurfaceForce(VNLMatrixRef& gPos, VNLMatrixRef& gVel, VNLMatrix& gForce) {
    const int nDim = 2;//vec::SIZE;

    int nSubj = (applySurfaceEntropyToFirstOnly ? 1 : m_nSubjects);

    // compute forces between particles
    VNLVector weights(m_nParticles);
    for (int n = 0; n < nSubj; n++) {
        SliceInterpolatorType::Pointer kappaIntp = m_Context->GetAttributeInterpolators()->at(n);
        for (int i = 0; i < m_nParticles; i++) {
            // reference data
            VNLVectorRef iForce(nDim, &gForce[n][nDim*i]);
            VNLVectorRef iVel(nDim, &gVel[n][nDim*i]);
            VNLVectorRef iPos(nDim, &gPos[n][nDim*i]);
            
            ContinuousIndexType iIdx;
            iIdx[0] = iPos[0];
            iIdx[1] = iPos[1];

  
            // iteration over particles
            // may reduce use symmetric properties
            for (int j = 0; j < m_nParticles; j++) {
                if (i == j) {
                    // there's no self interaction
                    weights[j] = 0;
                } else {
                    // kappa should use jPos
                    VNLVectorRef jPos(2, &gPos[n][nDim*j]);
                    ContinuousIndexType jIdx;
                    jIdx[0] = jPos[0];
                    jIdx[1] = jPos[1];
                    double kappa = kappaIntp->EvaluateAtContinuousIndex(jIdx);
                    kappa *= kappa;
                    double dij = (iPos-jPos).two_norm();
                    if (dij > m_Cutoff) {
                        weights[j] = 0;
                    } else {
                        weights[j] = exp(-dij*dij*kappa/(m_Sigma2));
                        // debug: check kappa is different between neighbors
//                        if (i == 10) {
//                            cout << "Distance: " << dij << "; Kappa: " << kappa << endl;
//                        }
                    }
                }
            }
            double sumForce = weights.sum();
            if (sumForce > 0) {
                weights /= sumForce;
            }
            
            // actual force update
            VNLVec2 xixj;
            // update force for neighboring particles
            for (int j = 0; j < m_nParticles; j++) {
                if (i == j || weights[j] == 0) {
                    continue;
                }
                VNLVectorRef jPos(nDim, &gPos[n][nDim*j]);
                VNLCVector::subtract(iPos.data_block(), jPos.data_block(), xixj.data_block(), nDim);
                xixj.normalize();
                iForce += (weights[j] * xixj);
            }

            // dragging force
            iForce -= (m_Mu * iVel);
        }
    }
}



void ParticleSystem::UpdateGravityForce(VNLMatrixRef& pos, VNLMatrixRef& vel, VNLMatrix& force) {
    const int nDim = 2;
    for (int n = 0; n < m_nSubjects; n++) {
        for (int i = 0; i < m_nParticles; i++) {
            force[n][nDim*i+1] = 0.98;
        }
    }
}

void ParticleSystem::EstimateRigidTransform(VNLMatrixRef& gPos, VNLMatrixArray& transforms, VNLMatrixArray& jacobians) {
    if (m_nParticles < 5) {
        cout << "Not enough particles for procrustes estimation" << endl;
        return;
    }


    // move target points to origin center
    VNLMatrix targetPoints(gPos[0], m_nParticles, nDim);
    VNLVec2 targetCentroid;
    vnl_row_mean(targetPoints, targetCentroid);
    vnl_row_subtract(targetPoints, targetCentroid, targetPoints);

    for (int n = 1; n < gPos.rows(); n++) {
        // move source points to origin center
        VNLMatrix sourcePoints(gPos[n], m_nParticles, nDim);
        VNLVec2 sourceCentroid;
        vnl_row_mean(sourcePoints, sourceCentroid);
        vnl_row_subtract(sourcePoints, sourceCentroid, sourcePoints);


        double rotationParam;
        VNLVec2 translationParam = targetCentroid - sourceCentroid;
        VNLMatrix cov = sourcePoints.transpose() * targetPoints;

        // debug: if the covariance is computed correctly
        //    std::cout << "COV: " << cov << std::endl;
        vnl_svd<double> svd(cov);
        VNLMatrix rotationMatrix = svd.V() * svd.U().transpose();

        //    std::cout << rotationMatrix << std::endl;

        // debug: atan2 is more stable than acos
        rotationParam = atan2(rotationMatrix[1][0], rotationMatrix[0][0]);
        //    std::cout << rotationParam << std::endl;
        //
        //    std::cout << "acos(1) = " << acos(1) << std::endl;

        VNLVector offset = rotationMatrix * translationParam;

        VNLMatrix transformMatrix(2,3);
        transformMatrix[0][2] = offset[0];
        transformMatrix[1][2] = offset[1];
        transformMatrix.update(rotationMatrix);

        VNLMatrix jacobian(2,2);
        jacobian.update(rotationMatrix);

//        vnl_identity(transformMatrix);
//        vnl_identity(jacobian);

        transforms.push_back(transformMatrix);
        jacobians.push_back(jacobian);

        cout << "Estimated Rigid Transform: " << transformMatrix << endl;
    }
}

void ParticleSystem::ApplyMatrixOperation(const double* posIn, const VNLMatrix& matrix, double* posOut) {
    double *pIn = (double*) posIn;
    double *pOut = posOut;
    for (int i = 0;i < m_nParticles; i++) {
        VNLVec3 posi;
        posi.copy_in(pIn);
        posi[2] = 1;
        VNLVectorRef tposi(nDim, pOut);
        vnl_matrix_x_vector(matrix, posi, tposi);
        pIn += nDim;
        pOut += nDim;
    }
}

void ParticleSystem::UpdateEnsembleForce(VNLMatrixRef& gPos, VNLMatrixRef& gVel, VNLMatrix& gForce)
 {

    // estimate transform from subjN to subj1
    VNLMatrixArray transforms;
    VNLMatrixArray jacobians;
//    cout << "Position before estimation: " << gPos << endl;
    EstimateRigidTransform(gPos, transforms, jacobians);
//    cout << "Position after estimation: " << gPos << endl;
    // transform particles onto subj1 space
    VNLMatrix tPos(gPos);
    for (int n = 1; n < m_nSubjects; n++) {
        ApplyMatrixOperation(gPos.begin(), transforms[n-1], tPos.begin());
    }

    // debug: check if transform is correct
//    cout << "Before: " << gPos << endl;
//    cout << "After: " << tPos << endl;

    // aggregate attribute data and compute mean
    VNLMatrix data(tPos.rows(), m_nParams);
    data.update(tPos);
    
    VNLVector dataMean(data.cols());
    vnl_row_mean(data, dataMean);

//    cout << "Data Mean: " << dataMean << endl;

    // move data to center
    for (int i = 0; i < m_nSubjects; i++) {
        VNLCVector::subtract(data[i], dataMean.begin(), data[i], data.cols());
    }

    // debug: positional entropy should work without image params
    //    cout << "nPaarms: " << nParams << endl;
//    cout << "Data: " << data << endl;
    VNLMatrix cov = data * data.transpose();

    // relaxation parameter for singular matrix
    // this produce pseudo-inverse matrix,
    // if alpha is zero, the inverse matrix will have really high numbers
    double alpha = 1;
    for (int i = 0; i < cov.rows(); i++) {
        cov[i][i] += alpha;
    }

    // debug: check covariance matrix
    cout << "Data COV: " << cov << endl;

    // cost function is the sum of log of eigenvalues
    vnl_symmetric_eigensystem<double> eigen(cov);

    // debug: eigenvalues for singular matrix; still producing eigenvalue with zero
    //    cout << "Eigenvalues: " << eigen.D << endl;
    //    cout << "Eigenvectors: " << eigen.V << endl;
    double cost = 0;
    for (int i = 0; i < eigen.D.size(); i++) {
        if (eigen.D[i] > 1) {
            cost += log(eigen.D[i]);
        }
    }

    // debug: let's see difference between image gradient and position gradient
    //    cout << "Covariance:" << cov << endl;
    VNLMatrix covInverse = vnl_matrix_inverse<double>(cov);
    VNLMatrix grad = covInverse * data;
    // debug: gradient is the direction minimizing covariance matrix
    //    cout << "Ensemble COV:" << cov << endl;
    //    cout << "Ensemble InverseCOV: " << covInverse << endl;
    //    cout << "gradient: " << grad << endl;
    if (grad.has_nans()) {
        cout << "Gradient has Nans: " << grad << endl;
    }

    VNLAlgebra alg(data.rows(), m_nParams);
    // multiply jacobian of the function to gradient
    for (int n = 1; n < m_nSubjects; n++) {
        ApplyMatrixOperation(grad[n], jacobians[n-1], grad[n]);
        grad *= -10;
//        cout << "Applying gradient: " << grad << endl;
        VNLCVector::add(gPos[n], grad[n], gPos[n], m_nParams);
    }
}


// Apply boundary constraint
void ParticleSystem::ApplyBoundaryConditions(const VNLVector &x, const VNLMatrix& gForce, VNLMatrixRef& dpdt, VNLMatrixRef& dvdt) {

    // boundary constraint
    const int nDim = 2;
    VNLMatrixRef gPos(m_nSubjects, m_nParams, (double*) x.data_block());
    VNLMatrixRef gVel(m_nSubjects, m_nParams, (double*) x.data_block()+m_nSubjects*m_nParams);

    int nSubj = applyBoundaryConditionToFirstOnly ? 1 : m_nSubjects;
    nSubj = m_nSubjects;
    for (int n = 0; n < nSubj; n++) {
        for (int i = 0; i < m_nParticles; i++) {

            // input data
            VNLVectorRef posi(nDim, &gPos[n][nDim*i]);
            VNLVectorRef forcei(nDim, (double*) &gForce[n][nDim*i]);
            VNLVectorRef veli(nDim, &gVel[n][nDim*i]);

            // output data
            VNLVectorRef dpdti(nDim, &dpdt[n][nDim*i]);
            VNLVectorRef dvdti(nDim, &dvdt[n][nDim*i]);

            // constraint should be available
            if (m_Constraint != NULL) {
                SliceInterpolatorType::ContinuousIndexType idx;
                SliceInterpolatorType::IndexType nidx;
                nidx[0] = idx[0] = posi[0];
                nidx[1] = idx[1] = posi[1];

                if (!m_Constraint->IsInsideRegion(n, nidx)) {
                    veli.fill(0);
                    continue;
                }

                double dist = m_Constraint->GetDistance(n, idx);
                if (dist >= 0) {
                    myImplicitSurfaceConstraint::OffsetType offset = m_Constraint->GetOutsideOffset(n, nidx);
                    for (int k = 0; k < nDim; k++) {
                        posi[k] += offset[k];
                    }
                }

                // compute on new position
                nidx[0] = idx[0] = posi[0];
                nidx[1] = idx[1] = posi[1];

                //                if (m_Constraint->GetOutsideOffset(n, nidx))

                GradientType g = m_Constraint->GetGradient(n, idx);
                VNLVectorRef normal(nDim, g.GetDataPointer());
                double normalMagnitude = normal.two_norm();

                // normalize to compute direction
                //                normal.normalize();

                // boundary conditions
                // velocity set to zero
                // the normal component of the force set to zero
                //                double velMag = veli.two_norm();

                // distance and gradient sometimes doesn't coincide.
                if (normalMagnitude > 0.4) {
                    normal.normalize();

                    // velocity should be zero toward normal direction
                    double normalSpeed = dot_product(veli, normal);
                    if (normalSpeed < 0) {
                        VNLVector newVelocity = veli - 2 * normalSpeed * normal;
                        newVelocity *= m_COR;
                        // how to know current timestep?
                        //                        newVelocity /= 0.1;
                        dvdti.fill(0);
                        veli.copy_in(newVelocity.data_block());
                        //                        cout << "Velocity: " << veli << " => " << newVelocity << "; Speed: " << normalSpeed << "; Normal: " << normal << endl;
                    }

                    // remove normal term of the current force
                    double normalForce = dot_product(normal, forcei);
                    if (normalForce < 0) {
                        VNLVector newForce = forcei + normalForce * normal;
                        dvdti.copy_in(newForce.data_block());
                        //                        cout << "Force: " << forcei << ", " << newForce << "; grad: " << normal << endl;
                    }
                }
            }
        }
    }
    
}



// integration function
void ParticleSystem::operator()(const VNLVector &x, VNLVector& dxdt, const double t) {
    VNLMatrixRef pos(m_nSubjects, m_nParams, (double*) x.data_block());
    VNLMatrixRef vel(m_nSubjects, m_nParams, (double*) x.data_block()+m_nSubjects*m_nParams);

    VNLMatrixRef dpdt(m_nSubjects, m_nParams, dxdt.data_block());
    VNLMatrixRef dvdt(m_nSubjects, m_nParams, dxdt.data_block()+m_nSubjects*m_nParams);

    m_Force.fill(0);

    // update forces at time t
    if (useEnsembleForce) {
        UpdateEnsembleForce(pos, vel, m_Force);
    }
    UpdateSurfaceForce(pos, vel, m_Force);
    // UpdateGravityForce(pos, vel, m_Force);

    if (useParticlePhysics) {
        // dP/dt = V
        dpdt.copy_in(vel.data_block());
        // dV/dt = F/m
        dvdt.copy_in(m_Force.data_block());
    } else {
        // gradient descent
        dpdt.copy_in(m_Force.data_block());
        dvdt.fill(0);
    }

    bool useImplicitBoundary = true;
    if (useImplicitBoundary) {
        ApplyBoundaryConditions(x, m_Force, dpdt, dvdt);
    }
}


// observer function
void ParticleSystem::operator()(const VNLVector &x, const double t) {
    const int nDim = 2;
    VNLMatrixRef gPos(m_nSubjects, m_nParams, (double*) x.data_block());
    VNLMatrixRef gVel(m_nSubjects, m_nParams, (double*) x.data_block()+m_nSubjects*m_nParams);

    double cost = 0;
    // compute forces between particles
    VNLVector weights(m_nParticles);
    for (int n = 0; n < m_nSubjects; n++) {
        for (int i = 0; i < m_nParticles; i++) {
            // reference data
            VNLVectorRef vel(nDim, &gVel[n][nDim*i]);
            VNLVectorRef pos(nDim, &gPos[n][nDim*i]);

            for (int j = 0; j < m_nParticles; j++) {
                if (i == j) {
                    // there's no self interaction
                    weights[j] = 0;
                    continue;
                }
                VNLVectorRef posj(2, &gPos[n][nDim*j]);

                double dij = (pos-posj).two_norm();
                if (dij > m_Cutoff) {
                    weights[j] = 0;
                    continue;
                }
                weights[j] = exp(-dij*dij/(m_Sigma2));
            }
            cost += weights.sum();
        }
    }

    double xy[2];
    xy[0] = t;
    xy[1] = cost;
    m_Callback->EventRaised(0xADDCEC, 0, NULL, xy);

    
    if (m_StatusHistory != NULL) {
        m_StatusHistory->push_back(x);
        m_CostHistory->push_back(cost);
    }
    cout << "Time: " << t << endl;
}

void ParticleSystem::Integrate() {
//    ParticleSystemObserver observer(this);

    VNLVector status(m_Status);
    // use constant time step
    const int RK4 = 2;
    const int EULER = 1;
    const int RKF45 = 0;

    double dt = 0.1;
    double t0 = 0;
    double t1 = t0 + m_Context->GetProperty().GetInt("numberOfIterations", 100) * dt;
    
    int odeMethod = EULER;
    boost::numeric::odeint::euler<VNLVector> eulerStepper;
    boost::numeric::odeint::runge_kutta4<VNLVector> rk4Stepper;
    switch (odeMethod) {
        case RKF45:
            boost::numeric::odeint::integrate((*this), m_Status, t0, t1, dt, (*this));
            break;
        case EULER:
            boost::numeric::odeint::integrate_const(eulerStepper, (*this), m_Status, t0, t1, dt, (*this));
            break;
        case RK4:
            boost::numeric::odeint::integrate_const(rk4Stepper, (*this), m_Status, t0, t1, dt, (*this));
            break;
        default:
            break;
    }

    cout << "History size: " << m_StatusHistory->size() << endl;

}