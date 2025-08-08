#include "ofxParticleSet.h"

ParticleSet::ParticleSet(float drawScale_)
: drawScale { drawScale_ }
{
  allocateParticles();
  createSpatialIndex();
  setThreadName("ParticleSet " + ofToString(this));
  startThread();
  loadShader();
}

ParticleSet::~ParticleSet() {
  updates.close();
  waitForThread(true);
}

void ParticleSet::allocateParticles() {
  static_assert(sizeof(Particle) == sizeof(float) * 13, "Particle struct is not tightly packed");
  positions.clear(); positions.reserve(maxParticles);
  particles.clear(); particles.reserve(maxParticles);
  activeCount = 1;
  activeCount = 2;
  particles.emplace_back(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f), ofFloatColor(1.0f), 0.0f, 1000.0f, 1000.0f);
  particles.emplace_back(glm::vec3(400.0f, 400.0f, 0.0f), glm::vec3(0.0f), ofFloatColor(1.0f), 0.0f, 1000.0f, 1000.0f);
//  particles.emplace_back(glm::vec3(0.0f), glm::vec3(0.0f), ofFloatColor(0.0f), 0.0f, 0.0f, 0.0f); // dummy to enable VBO creation
  particleVbo.setVertexData(&particles[0].position, activeCount, GL_DYNAMIC_DRAW);
  particleVbo.setAttributeData(1, glm::value_ptr(particles[0].velocity), 3, activeCount, GL_DYNAMIC_DRAW);//, sizeof(Particle));
  particleVbo.setAttributeData(2, &particles[0].color[0], 4, activeCount, GL_DYNAMIC_DRAW);//, sizeof(Particle));
  particleVbo.setAttributeData(3, &particles[0].spin, 1, activeCount, GL_DYNAMIC_DRAW);//, sizeof(Particle));
  particleVbo.setAttributeData(4, &particles[0].startLife, 1, activeCount, GL_DYNAMIC_DRAW);//, sizeof(Particle));
  particleVbo.setAttributeData(5, &particles[0].life, 1, activeCount, GL_DYNAMIC_DRAW);//, sizeof(Particle));
}

void ParticleSet::loadShader() {
  bool shaderLoaded = shader.setupShaderFromSource(GL_VERTEX_SHADER, getVertexShader())
    && shader.setupShaderFromSource(GL_FRAGMENT_SHADER, getFragmentShader())
    && shader.bindDefaults()
    && shader.linkProgram();
  if (!shaderLoaded) {
    ofLogError() << typeid(*this).name() << " not loaded";
    ofExit();
  }
}

// threaded
void ParticleSet::eraseDeadParticles() {
//  size_t s = particles.size();
//  auto it = std::remove_if(particles.begin(),
//                           particles.end(),
//                           [](Particle& p) { return (p.lifetime <= 0); });
////  size_t excessParticles = std::max(0L, (it - particles.begin()) - maxParticles);
//  size_t excessParticles = 0;
//  particles.erase(it, particles.end()); // the ones that are no longer alive
//  particles.erase(particles.begin(), particles.begin() + excessParticles); // the earliest excess ones
}

// threaded
// TODO: find a way to extend the KDTree templates to include Particle directly to avoid copying the positions out like this
void ParticleSet::createSpatialIndex() {
  positions.clear();
  std::transform(particles.begin(), particles.end(),
                 std::back_inserter(positions),
                 [](const Particle& p) { return p.position; });
  spatialIndexPtr = make_shared<ofx::KDTree<glm::vec3>>(positions);
}

// main thread
void ParticleSet::update() {
  updates.send({});
  updateVbo();
}

// main thread
void ParticleSet::add(glm::vec3 position, glm::vec3 velocity, ofFloatColor color, float spin) {
  NewParticleDatum datum { position, velocity, color, spin};
  ParticleSetUpdate update { {datum} };
  updates.send(update);
}

// threaded
const glm::vec3 createForce(const ParticleSet::Particle& p, const glm::vec3 target, float attraction, float attractionRadius) {
  glm::vec3 direction = target - p.position;
  float distance = glm::length(direction);
  float forceMagnitude = attraction * (1.0 - distance / attractionRadius);
  return glm::step(distance, attractionRadius) * forceMagnitude * glm::normalize(direction);
}

// threaded
void updateParticle(ParticleSet::Particle& p,
                    const std::vector<ParticleSet::Particle>& particles,
                    const SpatialIndexPtrT& spatialIndexPtr,
                    float particleVelocityDamping,
                    float particleAttraction,
                    float attractionRadius,
                    float forceScale) {
  if (p.life <= 0) return;
  if (particleAttraction != 0.0) {
    ofx::KDTree<glm::vec2>::SearchResults searchResults(20);
    spatialIndexPtr->findPointsWithinRadius(p.position, attractionRadius, searchResults);
    int count = 0;
    glm::vec3 centroid;
    for (const auto& searchResult: searchResults) {
      const ParticleSet::Particle& otherParticle = particles[searchResult.first];
      centroid += otherParticle.position;
      ++count;
    }
    if (count > 1) {
      centroid /= count;
      p.velocity += createForce(p, centroid, particleAttraction, attractionRadius) * forceScale;
    }
  }
  p.velocity = glm::rotate(p.velocity, p.spin, glm::vec3(0, 0, 1));
  p.velocity *= particleVelocityDamping;
  p.position += p.velocity * ofGetLastFrameTime();
  p.life--;
}

// threaded
void ParticleSet::threadedFunction() {
  ParticleSetUpdate update;
  while (updates.receive(update)) {
    lock();
    do {
      std::for_each(update.newParticleData.begin(), update.newParticleData.end(),
                    [&] (auto& d) {
        if (particles.size() < maxParticles) {
          float startLife = ofRandom(maxParticleAge);
          particles.emplace_back(d.position,
                                 d.velocity,
                                 d.color,
                                 d.spin,
                                 startLife,
                                 startLife);
        }
      });
    } while(updates.tryReceive(update));
    activeCount = particles.size();
    
    std::for_each(particles.begin(),
                  particles.end(),
                  [&] (auto& p) {
      updateParticle(p,
                     particles,
                     spatialIndexPtr,
                     particleVelocityDamping,
                     particleAttraction,
                     particleAttractionRadius * drawScale,
                     forceScale * drawScale);
    });
//    eraseDeadParticles();
    unlock();
    createSpatialIndex();
    ofLogNotice() << "ParticleSet::threadedFunction: updated " << particles.size() << " particles";
  }
}

void ParticleSet::updateVbo() {
  particleVbo.updateVertexData(&particles[0].position, particles.size());
  particleVbo.updateAttributeData(1, glm::value_ptr(particles[0].velocity), particles.size());
  particleVbo.updateAttributeData(2, &particles[0].color[0], particles.size());
  particleVbo.updateAttributeData(3, &particles[0].spin, particles.size());
  particleVbo.updateAttributeData(4, &particles[0].startLife, particles.size());
  particleVbo.updateAttributeData(5, &particles[0].life, particles.size());
  activeCount = particles.size();
}

//void ParticleSet::updateMeshes() {
//  float particleConnectionRadius2 = particleConnectionRadius * particleConnectionRadius * drawScale * drawScale;
//  ofx::KDTree<glm::vec2>::SearchResults searchResults;
//  searchResults.resize(10);
//  
//  ofMesh newPointMesh;
//  newPointMesh.setMode(OF_PRIMITIVE_POINTS);
//  newPointMesh.enableColors();
//  ofMesh newLineMesh;
//  newLineMesh.setMode(OF_PRIMITIVE_LINES);
//  newLineMesh.enableColors();
//
//  for (int i = 0; i < particles.size(); i++) {
//    Particle p = particles[i];
//    spatialIndexPtr->findPointsWithinRadius(p.position, particleConnectionRadius * drawScale, searchResults);
//    if (strategy != STRATEGY_POINTS) {
//      // draw connection if other particles close
//      for (const auto& searchResult: searchResults) {
//        if (searchResult.first <= i) continue; // don't double-count particles
//        const Particle& otherParticle = particles[searchResult.first];
//        if (p.position == otherParticle.position) continue;
//        float distanceSquared = searchResult.second;
//        float distanceScale = distanceSquared / particleConnectionRadius2; // 0 close, 1 far
//        newLineMesh.addVertex({ p.position, 0.0 });
//        ofFloatColor c = p.color;
//        c.a = p.color.a * (1.0 - distanceScale) * colourMultiplier;
//        newLineMesh.addColor(c);
//        newLineMesh.addVertex({ otherParticle.position, 0.0 });
//        c.r = otherParticle.color.r;
//        c.r = otherParticle.color.g;
//        c.r = otherParticle.color.b;
//        newLineMesh.addColor(c);
//      }
//    }
//    // draw point if point strategy, or no close particles for point-and-connection stategy
//    if (strategy == STRATEGY_POINTS || (strategy == STRATEGY_CONNECTIONS_AND_POINTS && searchResults.size() <= 1)) {
//      newPointMesh.addVertex({ p.position, 0.0 });
//      ofFloatColor c = p.color;
//      c.a = p.color.a * colourMultiplier;
//      newPointMesh.addColor(c);
//    }
//  }
//  
//  lock();
//  std::swap(newLineMesh, lineMesh);
//  std::swap(newPointMesh, pointMesh);
//  unlock();
//}

void checkGLError(const char* context) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        std::cerr << "OpenGL error in " << context << ": " << err << std::endl;
    }
}

void ParticleSet::draw() {
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(0, 255, 255, 255);
  ofDrawRectangle(300, 300, 50, 50);
  
  ofEnableBlendMode(OF_BLENDMODE_ALPHA); // or OF_BLENDMODE_ALPHA
//  shader.begin();
//  shader.setUniform1f("particleSize", 4.0);
  ofSetColor(255,0,0,255);
  ofSetPointSize(8.0);
  updateVbo();
  particleVbo.draw(GL_LINES, 0, activeCount);
  checkGLError("draw");
//  particleVbo.draw(GL_POINTS, 0, activeCount);
//  shader.end();
//  glPointSize(particleDrawRadius);
//  particleVbo.drawInstanced(GL_POINTS, 0, 1, activeCount);
//  pointMesh.draw(); // TODO: in a vert shader, can set gl_PointSize to p.drawRadius * drawScale
//  glPointSize(1.0);
//  lineMesh.draw();
}

ofParameterGroup& ParticleSet::getParameterGroup() {
  if (parameters.size() == 0) {
    parameters.setName(getParameterGroupName());
    parameters.add(strategy);
    parameters.add(maxParticles);
    parameters.add(maxParticleAge);
    parameters.add(particleVelocityDamping);
    parameters.add(particleAttraction);
    parameters.add(particleAttractionRadius);
    parameters.add(particleConnectionRadius);
    parameters.add(particleDrawRadius);
    parameters.add(colourMultiplier);
    parameters.add(forceScale);
  }
  return parameters;
}

std::string ParticleSet::getVertexShader() {
  return R"(
    #version 410

    // Vertex attributes matching ParticleSet::Particle struct
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 velocity;
    layout(location = 2) in vec4 color;
    layout(location = 3) in float spin;
    layout(location = 4) in float startLife;
    layout(location = 5) in float life;

    uniform mat4 modelViewProjectionMatrix;
    uniform float particleSize;

    out vec4 colorVarying;
    out float lifeRatioVarying;

    void main() {
      // 1.0 just born, <= 0.0 dead
      lifeRatioVarying = life / startLife;      
      colorVarying = vec4(color.rgb, color.a * smoothstep(0.0, 0.2, lifeRatioVarying)); // Fade out in last 20% of life
      gl_PointSize = particleSize * lifeRatioVarying;
      gl_Position = modelViewProjectionMatrix * vec4(position, 1.0);
  
  colorVarying = vec4(1.0);
  gl_PointSize = 8.0;
  gl_Position = vec4(100.0, 100.0, 0.0, 1.0);
    }
  )";
}

std::string ParticleSet::getFragmentShader() {
  return R"(
    #version 410

    in vec4 colorVarying;
    in float lifeRatioVarying;

    out vec4 fragColor;

    void main() {
  fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Default color for debugging
  return;
  
      if (lifeRatioVarying <= 0.0) discard;
      
      // Create circular falloff using gl_PointCoord
      // gl_PointCoord ranges from (0,0) to (1,1) across the point
      vec2 center = vec2(0.5, 0.5);
      float distance = length(gl_PointCoord - center);
      float radius = 0.5;
      float softness = 0.1; // How soft the edge is      
      float alpha = 1.0 - smoothstep(radius - softness, radius, distance);
      fragColor = vec4(colorVarying.rgb, colorVarying.a * alpha);
      
      // Optional: Add some sparkle/energy effect based on life
//      float sparkle = sin(lifeRatioVarying * 3.14159) * 0.3 + 0.7;
//      fragColor.rgb *= sparkle;
      
      // Discard fully transparent pixels for better performance
      if (fragColor.a < 0.01) discard;
    }
  )";
}
