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
  const glm::vec2 createForce(const glm::vec2 target, float attraction, float attractionRadius);
  void update(const std::vector<Particle>& particles,
              const SpatialIndexPtrT& spatialIndexPtr,
              float particleVelocityDamping,
              float particleAttraction,
              float attractionRadius);
  
  glm::vec2 position;
  glm::vec2 velocity;
  float spin;
  float drawRadius;
  ofFloatColor color;
  int lifetime;
};



class ParticleSet {
//class ParticleSet: public ofThread {

public:
  void eraseDeadParticles();
  void createSpatialIndex();
  void update();
  void add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color, float spin);
  void draw();
  size_t size() const { return particles.size(); };
  
  
  std::string getParameterGroupName() const { return "Particle Set"; }
  ofParameterGroup parameters;
  ofParameter<int> maxParticles {"maxParticles", 300, 100, 2000 };
  ofParameter<int> maxParticleAge {"maxParticleAge", 500, 10, 1000 };
  ofParameter<float> particleVelocityDamping {"particleVelocityDamping", 0.995, 0.9, 1.0 };
  ofParameter<float> particleAttraction {"particleAttraction", 0.0, -0.2, 0.2 };
  ofParameter<float> particleAttractionRadius {"particleAttractionRadius", 150.0, 0.0, 1000.0 };
  ofParameter<float> particleConnectionRadius {"particleConnectionRadius", 0.0, 0.0, 1000.0 };
  ofParameter<float> particleDrawRadius {"particleDrawRadius", 1.7, 1.0, 20.0 };

  ofParameterGroup& getParameterGroup();

//protected:
//  void threadedFunction() override;

private:
  std::vector<Particle> particles;
  std::vector<glm::vec2> positions; // particle positions used to create the spatial index. There must be direct way but I can't make the templates work
  SpatialIndexPtrT spatialIndexPtr;

//  ofThreadChannel<SomInstanceDataT> newInstanceData;
//  ofThreadChannel<ofPixels> newPalettePixels;
//  bool isNewPalettePixelsReady;
//  ofPixels pixels; // non-GL pixels for the palette
};
