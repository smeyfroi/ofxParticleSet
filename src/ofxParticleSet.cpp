#include "ofxParticleSet.h"

Particle::Particle(glm::vec2 position_, glm::vec2 velocity_, float spin_, float drawRadius_, ofFloatColor color_, int lifetime_):
position { position_ },
velocity { velocity_ },
spin { spin_ },
drawRadius { drawRadius_ },
color { color_ },
lifetime { lifetime_ }
{}

bool Particle::isAlive() const {
  return lifetime > 0 && glm::length2(velocity) > (0.0005 * 0.0005);
}

const glm::vec2 Particle::createForce(const glm::vec2 target, float attraction, float attractionRadius) const {
  glm::vec2 direction = target - position;
  float distance = glm::length(direction);
  float forceMagnitude = attraction * (1.0 - distance / attractionRadius);
  return glm::step(distance, attractionRadius) * forceMagnitude * glm::normalize(direction);
}

void Particle::update(const std::vector<Particle>& particles,
                      const SpatialIndexPtrT& spatialIndexPtr,
                      float particleVelocityDamping,
                      float particleAttraction,
                      float attractionRadius) {
  if (!isAlive()) return;
  
  if (particleAttraction != 0.0) {
    ofx::KDTree<glm::vec2>::SearchResults searchResults(20);
    spatialIndexPtr->findPointsWithinRadius(position, attractionRadius, searchResults);
    int count = 0;
    glm::vec2 centroid;
    for (const auto& searchResult: searchResults) {
      const Particle& p = particles[searchResult.first];
      centroid += p.position;
      ++count;
    }
    if (count > 1) {
      centroid /= count;
      velocity += createForce(centroid, particleAttraction, attractionRadius);
    }
  }
  
  velocity = glm::rotate(velocity, spin);
  velocity *= particleVelocityDamping;
  position += velocity;
  lifetime--;
}



ParticleSet::ParticleSet(float drawScale_)
: drawScale { drawScale_ }
{
  createSpatialIndex();
  setThreadName("ParticleSet " + ofToString(this));
  startThread();
}

ParticleSet::~ParticleSet() {
  updates.close();
  waitForThread(true);
}

void ParticleSet::eraseDeadParticles() {
  size_t s = particles.size();
  auto it = std::remove_if(particles.begin(),
                           particles.end(),
                           [](Particle& p) { return (!p.isAlive()); });
  size_t excessParticles = std::max(0L, (it - particles.begin()) - maxParticles);
  particles.erase(it, particles.end()); // the ones that are no longer alive
  particles.erase(particles.begin(), particles.begin() + excessParticles); // the earliest excess ones
}

// TODO: find a way to extend the KDTree templates to include Particle directly to avoid copying the positions out like this
void ParticleSet::createSpatialIndex() {
  positions.clear();
  positions.reserve(particles.size());
  std::transform(particles.begin(),
                 particles.end(),
                 std::back_inserter(positions),
                 [](const Particle& p) { return p.position; });
  spatialIndexPtr = make_shared<ofx::KDTree<glm::vec2>>(positions);
}

void ParticleSet::update() {
  updates.send({});
}

void ParticleSet::add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color, float spin) {
  NewParticleDatum datum { position, velocity, color, spin};
  ParticleSetUpdate update { {datum} };
  updates.send(update);
}

void ParticleSet::threadedFunction() {
  ParticleSetUpdate update;
  while (updates.receive(update)) {
    lock();
    do {
      std::for_each(update.newParticleData.begin(),
                    update.newParticleData.end(),
                    [&] (auto& d) {
        particles.emplace_back(d.position,
                               d.velocity,
                               d.spin,
                               particleDrawRadius,
                               d.color,
                               ofRandom(maxParticleAge));
      });
    } while(updates.tryReceive(update));
    std::for_each(particles.begin(),
                  particles.end(),
                  [&] (auto& p) {
      p.update(particles,
               spatialIndexPtr,
               particleVelocityDamping,
               particleAttraction,
               particleAttractionRadius * drawScale);
    });
    eraseDeadParticles();
    createSpatialIndex();
    unlock();
  }
}

void ParticleSet::draw() {
  float particleConnectionRadius2 = particleConnectionRadius * particleConnectionRadius * drawScale * drawScale;
  ofx::KDTree<glm::vec2>::SearchResults searchResults;
  searchResults.resize(10);
  
  lock();
  {
    ofFill();
    for (int i = 0; i < particles.size(); i++) {
      Particle p = particles[i];
      ofFloatColor c = p.color;
      spatialIndexPtr->findPointsWithinRadius(p.position, particleConnectionRadius * drawScale, searchResults);
      for (const auto& searchResult: searchResults) {
        if (searchResult.first <= i) continue; // don't double-count particles
        const Particle& otherParticle = particles[searchResult.first];
        if (p.position == otherParticle.position) continue;
        float distanceSquared = searchResult.second;
        float distanceScale = distanceSquared / particleConnectionRadius2; // 0 close, 1 far
        c.a = p.color.a * (1.0 - distanceScale) * ((float)p.lifetime / maxParticleAge);
        ofSetColor(c);
        ofSetLineWidth(p.drawRadius * drawScale);
        ofDrawLine(p.position, otherParticle.position);
      }
      if (searchResults.size() <= 1) {
        //      c.a *= std::sqrt((float)p.lifetime / maxParticleAge);// * (glm::length2(p.velocity) / 5.0);
        ofSetColor(c);
        ofDrawCircle(p.position, p.drawRadius / 2.0 * drawScale);
      }
    }
  }
  unlock();
}

ofParameterGroup& ParticleSet::getParameterGroup() {
  if (parameters.size() == 0) {
    parameters.setName(getParameterGroupName());
    parameters.add(maxParticles);
    parameters.add(maxParticleAge);
    parameters.add(particleVelocityDamping);
    parameters.add(particleAttraction);
    parameters.add(particleAttractionRadius);
    parameters.add(particleConnectionRadius);
    parameters.add(particleDrawRadius);
  }
  return parameters;
}
