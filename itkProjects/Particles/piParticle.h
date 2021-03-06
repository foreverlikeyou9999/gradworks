//
//  piParticle.h
//  ParticleGuidedRegistration
//
//  Created by Joohwi Lee on 4/5/13.
//
//

#ifndef ParticleGuidedRegistration_piParticle_h
#define ParticleGuidedRegistration_piParticle_h

#include <iostream>
#include <vector>
#include "piMacros.h"

#ifndef ATTR_SIZE
#define ATTR_SIZE 3
#endif

#if DIMENSIONS == 3
#define NATTRS ATTR_SIZE*ATTR_SIZE*ATTR_SIZE
#else
#define NATTRS ATTR_SIZE*ATTR_SIZE
#endif

namespace pi {

    class ParticleAttribute {
    public:
        DataReal f[DIMENSIONS];
        DataReal F[DIMENSIONS];
        DataReal o[DIMENSIONS];

        DataReal x[NATTRS];
        DataReal y[NATTRS];
        DataReal z[NATTRS];
        DataReal g[NATTRS][DIMENSIONS];
    };

    class Particle {
    public:
        int subj;
        int idx;
        int label;

        DataReal t;

        // the position x and the transformed point y
        DataReal x[4];
        DataReal y[4];
        DataReal z[4];
        DataReal w[4];


        // the current velocity v and the force f
        DataReal v[4];
        DataReal f[4];
        DataReal E[4];

        // the density and pressure of a particle
        DataReal density;
        DataReal pressure;

        bool enabled;
        bool collisionEvent;

        Particle();
        ~Particle();

        void Zero();
        void Disable();
        void Enable();
        void Sub(const Particle& p, DataReal* nx);
        void AddForce(DataReal* ff, DataReal alpha = 1);
        void UpdateStatus(DataReal dt);

        DataReal Dist2(const Particle& p);

        Particle& operator=(const Particle& other);
    };
    typedef std::vector<Particle> ParticleVector;


    class ParticlePatch {
    public:
        VNLVector values;
        VNLMatrix gradients;

        ParticlePatch();
        ~ParticlePatch();

        void setSize(int size, int dim);

    private:
        ParticlePatch(const ParticlePatch&);
        void operator=(const ParticlePatch&);

    private:
        int _size;
        int _dims;
    };
    typedef std::vector<ParticlePatch> PatchVector;
    

    // utility operator overloading
    std::ostream& operator<<(std::ostream& os, const Particle& par);
    std::istream& operator>>(std::istream& is, Particle& par);
}

#endif
