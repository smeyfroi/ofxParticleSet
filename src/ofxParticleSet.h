#pragma once

#include "ofMain.h"
#include <array>
#include <deque>
#include <string>
#include <vector>

struct NewParticleDatum {
  glm::vec2 position;
  glm::vec2 velocity;
  ofFloatColor color;
  float spin;
  float drawRadius;
};

class ParticleSet {
public:
  enum Strategy {
    STRATEGY_POINTS = 0,
    STRATEGY_CONNECTIONS = 1,
    STRATEGY_CONNECTIONS_AND_POINTS = 2
  };

  ParticleSet(float drawScale_ = 1.0f);
  ~ParticleSet();

  void update();
  void draw();
  void add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color, float spin, float drawRadius = -1.0f);

  size_t size() const { return liveCount; }

  std::string getParameterGroupName() const { return "Particle Set"; }
  ofParameterGroup& getParameterGroup();

  ofParameterGroup parameters;
  ofParameter<int> strategy { "strategy", STRATEGY_CONNECTIONS_AND_POINTS, STRATEGY_POINTS, STRATEGY_CONNECTIONS_AND_POINTS };
  ofParameter<int> maxParticles { "maxParticles", 5000, 100, 20000 };
  ofParameter<int> maxParticleAge { "maxParticleAge", 500, 10, 5000 };
  ofParameter<float> timeStep { "timeStep", 0.016f, 0.001f, 0.1f };
  ofParameter<float> velocityDamping { "velocityDamping", 0.995f, 0.99f, 1.0f };
  ofParameter<float> attractionStrength { "attractionStrength", -0.02f, -0.5f, 0.5f };
  ofParameter<float> attractionRadius { "attractionRadius", 0.1f, 0.0f, 1.0f }; // normalised
  ofParameter<float> forceScale { "forceScale", 0.1f, 0.0f, 1.0f }; // normalised
  ofParameter<float> connectionRadius { "connectionRadius", 0.05f, 0.0f, 1.0f }; // normalised
  ofParameter<float> colourMultiplier { "colourMultiplier", 0.2f, 0.0f, 1.0f };
  ofParameter<float> particleDrawRadius { "particleDrawRadius", 3.0f, 0.5f, 16.0f };
  ofParameter<float> initialVelocityScale { "initialVelocityScale", 50.0f, 0.1f, 100.0f };
  ofParameter<float> maxSpeed { "maxSpeed", 50.0f, 1.0f, 100.0f };
  ofParameter<int> sortNeighborWindow { "sortNeighborWindow", 256, 16, 2048 };
  ofParameter<float> lineFadeExponent { "lineFadeExponent", 1.0f, 0.1f, 4.0f };

private:
  struct GpuParticle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 spinDraw;
    glm::vec4 color;
    glm::vec2 lifetime; // x = remaining, y = max
    glm::vec2 flags; // x = alive flag, y = padding
  };

  static constexpr size_t ParticleStride = sizeof(GpuParticle);

  void allocateResources(int count);
  void destroyResources();
  void compileShaders();
  void setupProxyVbo();

  void processPendingAdditions();
  void uploadParticleToBuffer(int particleIndex);
  void runTransformFeedback(float deltaTime);
  void readBackParticles();
  void rebuildSpatialSort();
  void uploadSortedData();
  void ensureCapacity();
  int computeNeighborWindow() const;

  size_t liveCount = 0;

  std::vector<GpuParticle> cpuParticles;
  std::vector<int> freeList;
  std::vector<int> inverseSortedIndices;
  std::vector<int> sortedIndices;
  std::vector<glm::vec2> sortedPositions;
  std::vector<glm::vec4> sortedColors;

  std::deque<NewParticleDatum> pendingParticles;

  ofShader updateShader;
  ofShader pointShader;
  ofShader lineShader;
  ofVbo proxyVbo;

  std::array<GLuint, 2> vaos { 0, 0 };
  std::array<GLuint, 2> vbos { 0, 0 };
  GLuint particlePositionTBO = 0;
  GLuint particlePositionTexture = 0;
  GLuint sortedIndexTBO = 0;
  GLuint sortedIndexTexture = 0;
  GLuint inverseIndexTBO = 0;
  GLuint inverseIndexTexture = 0;
  GLuint sortedColorTBO = 0;
  GLuint sortedColorTexture = 0;

  int currentBuffer = 0;
  bool resourcesReady = false;
  bool shadersReady = false;
};
