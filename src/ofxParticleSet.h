#pragma once

#include "ofMain.h"
#include "ofxParticleSet.h"
#include "ofxSpatialHash.h"
#include "ofParameter.h"
#include <memory>
#include <vector>

using SpatialIndexT = ofx::KDTree<glm::vec2>;
using SpatialIndexPtrT = shared_ptr<SpatialIndexT>;



class Particle {
  
public:
  Particle(glm::vec2 position_, glm::vec2 velocity_, float spin_, float drawRadius_, ofFloatColor color_, int lifetime_);
  bool isAlive() const;
  const glm::vec2 createForce(const glm::vec2 target, float attraction, float attractionRadius) const;
  void update(const std::vector<Particle>& particles,
              const SpatialIndexPtrT& spatialIndexPtr,
              float particleVelocityDamping,
              float particleAttraction,
              float attractionRadius,
              float distanceScale);
  
  glm::vec2 position;
  glm::vec2 velocity;
  float spin;
  float drawRadius;
  ofFloatColor color;
  int lifetime;
};



struct NewParticleDatum {
  glm::vec2 position;
  glm::vec2 velocity;
  ofFloatColor color;
  float spin;
};

struct ParticleSetUpdate {
  std::vector<NewParticleDatum> newParticleData;
};

class ParticleSet: public ofThread {

public:
  ParticleSet(float drawScale_ = 1.0); // 1.0 assumes normalised coords
  ~ParticleSet();
  void update();
  void add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color, float spin);
  void draw();
  size_t size() const { return particles.size(); };
  
  const int STRATEGY_POINTS = 0;
  const int STRATEGY_CONNECTIONS = 1;
  const int STRATEGY_CONNECTIONS_AND_POINTS = 2;
  
  std::string getParameterGroupName() const { return "Particle Set"; }
  ofParameterGroup parameters;
  ofParameter<int> strategy { "strategy", 1, 0, 2 };
  ofParameter<int> maxParticles { "maxParticles", 250, 100, 2000 };
  ofParameter<int> maxParticleAge { "maxParticleAge", 500, 10, 1000 };
  ofParameter<float> particleVelocityDamping { "particleVelocityDamping", 0.995, 0.9, 1.0 };
  ofParameter<float> particleAttraction { "particleAttraction", -0.02, -0.2, 0.2 };
  ofParameter<float> particleAttractionRadius { "particleAttractionRadius", 0.1, 0.0, 1.0 }; // normalised
  ofParameter<float> particleConnectionRadius { "particleConnectionRadius", 0.05, 0.0, 1.0 }; // normalised
  ofParameter<float> particleDrawRadius { "particleDrawRadius", 0.001, 0.0, 0.05 }; // normalised
  ofParameter<float> colourMultiplier { "colourMultiplier", 0.1, 0.0, 1.0 };
  ofParameter<float> forceScale { "forceScale", 0.1, 0.0, 0.4 }; // normalised

  ofParameterGroup& getParameterGroup();

protected:
  void threadedFunction() override;

private:
  float drawScale;
  ofMesh pointMesh, lineMesh;
  
  std::vector<Particle> particles;
  std::vector<glm::vec2> positions; // particle positions used to create the spatial index. There must be a direct way but I can't make the templates work
  SpatialIndexPtrT spatialIndexPtr;

  void eraseDeadParticles();
  void createSpatialIndex();
  void updateMeshes();

  ofThreadChannel<ParticleSetUpdate> updates;
};
