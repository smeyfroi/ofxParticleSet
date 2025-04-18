#include "ofxParticleSet.h"

constexpr float PARTICLE_VELOCITY_DAMPING = 0.99;

Particle::Particle(glm::vec2 position_, glm::vec2 velocity_, float spin_, float radius_, ofFloatColor color_, int lifetime_):
position { position_ },
velocity { velocity_ },
spin { spin_ },
radius { radius_ },
color { color_ },
lifetime { lifetime_ }
{}

const glm::vec2 Particle::createForce(const glm::vec2 target, float attraction, float influence) {
  glm::vec2 direction = target - position;
  float distance = direction.length();
  float forceMagnitude = attraction * (1.0 - distance/influence);
  return glm::step(distance, influence) * forceMagnitude * glm::normalize(direction);
}

constexpr float PARTICLE_REPULSION = -0.001;
void Particle::update(const std::vector<Particle>& particles, const SpatialIndexPtrT& spatialIndexPtr) {
  if (!isAlive()) return;
  
  ofx::KDTree<glm::vec2>::SearchResults searchResults(20);
  spatialIndexPtr->findPointsWithinRadius(position, radius, searchResults);
  int count = 0;
  glm::vec2 centroid;
  for (const auto& searchResult: searchResults) {
    const Particle& otherParticle = particles[searchResult.first];
    if (position == otherParticle.position) continue;
    centroid += otherParticle.position;
    ++count;
  }
  if (count != 0) velocity += createForce(centroid/count, PARTICLE_REPULSION, radius);
  
  velocity = glm::rotate(velocity, spin);
  velocity *= PARTICLE_VELOCITY_DAMPING;
  position += velocity;
  lifetime--;
}



ParticleSet::ParticleSet(int maxParticleAge_):
maxParticleAge { maxParticleAge_ }
{
  searchResults.resize(20); // number of search results in drawConnections
}

void ParticleSet::eraseDeadParticles() {
  auto it = std::remove_if(particles.begin(), particles.end(), [](Particle& p) { return !p.isAlive(); });
  particles.erase(it, particles.end());
}

// TODO: find a way to extend the KDTree templates to include Particle directly
// so that we can avoid copying the positions out like this
void ParticleSet::createSpatialIndex() {
  positions.clear();
  positions.reserve(particles.size());
  std::transform(particles.begin(), particles.end(), std::back_inserter(positions), [](const Particle& p) { return p.position; });
  spatialIndexPtr = make_shared<ofx::KDTree<glm::vec2>>(positions);
}

void ParticleSet::update() {
  for (auto& p : particles) {
    p.update(particles, spatialIndexPtr);
  }
  eraseDeadParticles();
  createSpatialIndex();
}

void ParticleSet::add(glm::vec2 position) {
  for (int i = 0; i < 10; i++) {
    particles.emplace_back(
                           position,
                           glm::vec2{ofRandom(-5.0, 5.0), ofRandom(-5.0, 5.0)},
                           ofRandom(0, glm::two_pi<float>()/256.0),
                           ofRandom(10.0, 200.0),
                           ofFloatColor { ofRandom(1.0), ofRandom(1.0), ofRandom(1.0), ofRandom(0.1) },
                           ofRandom(maxParticleAge));
  }
}

void ParticleSet::drawPoints() {
  ofBlendMode(OF_BLENDMODE_ALPHA);
  ofFill();
  for (auto& p : particles) {
    ofSetColor(p.color);
    ofDrawCircle(p.position, 1.0);
  }
}

void ParticleSet::drawConnections() {
  ofBlendMode(OF_BLENDMODE_ALPHA);
  searchResults.clear();
  for (int i = 0; i < particles.size(); i++) {
    Particle p = particles[i];
    spatialIndexPtr->findPointsWithinRadius(p.position, p.radius, searchResults);
    for (const auto& searchResult: searchResults) {
      if (searchResult.first <= i) continue; // don't double-count particles
      const Particle& otherParticle = particles[searchResult.first];
      if (p.position == otherParticle.position) continue;
      float distanceSquared = searchResult.second;
      float distanceScale = distanceSquared/(p.radius*p.radius); // 0 close, 1 far
      ofFloatColor c = p.color;
      c.a = p.color.a * (1.0 - distanceScale) * ((float)p.lifetime / maxParticleAge);
      ofSetColor(c);
      ofSetLineWidth(1); //Gui::getInstance().lineWidth);
      ofDrawLine(p.position, otherParticle.position);
    }
  }
}

//ParticleSet::ParticleSet(int width_, int height_, float initialLearningRate_, int numIterations_) :
//width { width_ },
//height { height_ }
//{
//  setThreadName("ParticleSet");
//
//
//  startThread();
//}
//
//SomPalette::~SomPalette() {
//  newInstanceData.close();
//  waitForThread(true);
//}
//
//void SomPalette::addInstanceData(SomInstanceDataT& instanceData) {
//  if (isIterating()) newInstanceData.send(instanceData);
//}
//
//// TODO: Make sure we can't be overwhelmed if producer fills queue faster than we consume (e.g. could just do the SOM not the pixels)
//void SomPalette::threadedFunction() {
//  while (newInstanceData.receive(instanceData)) {
//      }
//    }
//    newPalettePixels.send(std::move(p));
//  }
//}
//
//void SomPalette::update() {
//  isNewPalettePixelsReady = false;
//  while (newPalettePixels.tryReceive(pixels)) {
//    isNewPalettePixelsReady = true;
//  }
//  if (isNewPalettePixelsReady) {
//    if (!paletteTexture.isAllocated()) paletteTexture.allocate(pixels);
//    paletteTexture.loadData(pixels);
//  }
//}
