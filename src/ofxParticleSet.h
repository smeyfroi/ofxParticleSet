#pragma once

#include "ofMain.h"
#include "ofxParticleSet.h"
#include "ofxSpatialHash.h"
#include <memory>
#include <vector>

using SpatialIndexT = ofx::KDTree<glm::vec2>;
using SpatialIndexPtrT = shared_ptr<SpatialIndexT>;



class Particle {
  
public:
  Particle(glm::vec2 position_, glm::vec2 velocity_, float spin_, float radius_, ofFloatColor color_, int lifetime_);
  bool isAlive() const { return lifetime > 0; };
  const glm::vec2 createForce(const glm::vec2 target, float attraction, float influence);
  void update(const std::vector<Particle>& particles, const SpatialIndexPtrT& spatialIndexPtr);
  
  glm::vec2 position;
  glm::vec2 velocity;
  float spin;
  float radius; // determines the radius for connections
  ofFloatColor color;
  int lifetime;
};



class ParticleSet {
//class ParticleSet: public ofThread {

public:
  ParticleSet(int maxParticleAge_);
  void eraseDeadParticles();
  void createSpatialIndex();
  void update();
  void add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color);
  void drawPoints();
  void drawConnections();
  size_t size() const { return particles.size(); };
  
//protected:
//  void threadedFunction() override;

private:
  int maxParticleAge;
  std::vector<Particle> particles;
  std::vector<glm::vec2> positions; // particle positions used to create the spatial index. There must be direct way but I can't make the templates work
  SpatialIndexPtrT spatialIndexPtr;
  ofx::KDTree<glm::vec2>::SearchResults searchResults;

//  ofThreadChannel<SomInstanceDataT> newInstanceData;
//  ofThreadChannel<ofPixels> newPalettePixels;
//  bool isNewPalettePixelsReady;
//  ofPixels pixels; // non-GL pixels for the palette
};
