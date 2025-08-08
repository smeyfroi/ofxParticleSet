#pragma once

#include "ofMain.h"
#include "ofxParticleSet.h"
#include "ofxSpatialHash.h"
#include "ofParameter.h"
#include <memory>
#include <vector>

using SpatialIndexT = ofx::KDTree<glm::vec3>;
using SpatialIndexPtrT = shared_ptr<SpatialIndexT>;

struct NewParticleDatum {
  glm::vec3 position;
  glm::vec3 velocity;
  ofFloatColor color;
  float spin;
};

struct ParticleSetUpdate {
  std::vector<NewParticleDatum> newParticleData;
};

class ParticleSet: public ofThread {

public:
  struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    ofFloatColor color;
    float spin;
    float startLife;
    float life;
  };

  ParticleSet(float drawScale_ = 1.0); // 1.0 assumes normalised coords
  ~ParticleSet();
  void update();
  void add(glm::vec3 position, glm::vec3 velocity, ofFloatColor color, float spin);
  void draw();
  void allocateParticles();
  size_t size() const { return particles.size(); };
  
  const int STRATEGY_POINTS = 0;
  const int STRATEGY_CONNECTIONS = 1;
  const int STRATEGY_CONNECTIONS_AND_POINTS = 2;
  
  std::string getParameterGroupName() const { return "Particle Set"; }
  ofParameterGroup parameters;
  ofParameter<int> strategy { "strategy", 1, 0, 2 };
  ofParameter<int> maxParticles { "maxParticles", 10000, 100, 20000 }; // TODO: event listener to reallocate everything
  ofParameter<int> maxParticleAge { "maxParticleAge", 500, 10, 1000 };
  ofParameter<float> particleVelocityDamping { "particleVelocityDamping", 0.995, 0.9, 1.0 };
  ofParameter<float> particleAttraction { "particleAttraction", -0.02, -0.2, 0.2 };
  ofParameter<float> particleAttractionRadius { "particleAttractionRadius", 0.1, 0.0, 1.0 }; // normalised
  ofParameter<float> particleConnectionRadius { "particleConnectionRadius", 0.05, 0.0, 1.0 }; // normalised
  ofParameter<float> particleDrawRadius { "particleDrawRadius", 1.5, 0.0, 16.0 };
  ofParameter<float> colourMultiplier { "colourMultiplier", 0.1, 0.0, 1.0 };
  ofParameter<float> forceScale { "forceScale", 0.1, 0.0, 0.4 }; // normalised

  ofParameterGroup& getParameterGroup();

protected:
  void threadedFunction() override;

private:
  float drawScale;
  int activeCount;
  std::vector<Particle> particles;
  ofVbo particleVbo;
  ofShader shader;
  void loadShader();
  std::string getVertexShader();
  std::string getFragmentShader();

  std::vector<glm::vec3> positions; // particle positions used to create the spatial index. There must be a direct way but I can't make the templates work. This happens in parallel in any case.
  SpatialIndexPtrT spatialIndexPtr;

  void eraseDeadParticles();
  void createSpatialIndex();
  void updateVbo();

  ofThreadChannel<ParticleSetUpdate> updates;
};
