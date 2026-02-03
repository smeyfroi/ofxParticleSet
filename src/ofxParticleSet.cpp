#include "ofxParticleSet.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

constexpr uint32_t MortonBits = 13u; // 8,192 buckets per axis (~0.85px at 7k)
static_assert(MortonBits <= 21u, "MortonBits must remain <= 21 to fit masks");
constexpr uint32_t MortonScale = (1u << MortonBits) - 1u;
constexpr uint32_t MortonJitterBits = 10u; // hashed tie-breaker to decorrelate Z-order bands
constexpr uint32_t MortonJitterMask = (1u << MortonJitterBits) - 1u;
constexpr float MortonNeighborWindowMultiplier = 2.0f;

uint64_t expandBits(uint32_t v) {

  uint64_t x = static_cast<uint64_t>(v & 0x1FFFFFu);
  x = (x | (x << 32)) & 0x1F00000000FFFFull;
  x = (x | (x << 16)) & 0x1F0000FF0000FFull;
  x = (x | (x << 8)) & 0x100F00F00F00F00Full;
  x = (x | (x << 4)) & 0x10C30C30C30C30C3ull;
  x = (x | (x << 2)) & 0x1249249249249249ull;
  return x;
}

uint64_t calculateMortonCode(const glm::vec2& position) {
  constexpr float upperBound = 0.999999f;
  glm::vec2 normalised = glm::clamp(position, glm::vec2(0.0f), glm::vec2(upperBound));
  glm::vec2 scaled = normalised * static_cast<float>(MortonScale);
  uint32_t xx = static_cast<uint32_t>(scaled.x);
  uint32_t yy = static_cast<uint32_t>(scaled.y);
  xx = std::min<uint32_t>(xx, MortonScale);
  yy = std::min<uint32_t>(yy, MortonScale);
  return expandBits(xx) | (expandBits(yy) << 1);
}

uint32_t hashIndex(uint32_t value) {
  value ^= value >> 17;
  value *= 0xed5ad4bbU;
  value ^= value >> 11;
  value *= 0xac4c1b51U;
  value ^= value >> 15;
  value *= 0x31848babU;
  value ^= value >> 14;
  return value;
}

} // namespace

ParticleSet::ParticleSet() {
  getParameterGroup();
  compileShaders();
  allocateResources(maxParticles.get());
  setupProxyVbo();
}

ParticleSet::~ParticleSet() {
  destroyResources();
}

void ParticleSet::setParameterOverrides(const ParameterOverrides& overrides) {
  if (parameterOverrides.timeStep == overrides.timeStep &&
      parameterOverrides.velocityDamping == overrides.velocityDamping &&
      parameterOverrides.attractionStrength == overrides.attractionStrength &&
      parameterOverrides.attractionRadius == overrides.attractionRadius &&
      parameterOverrides.forceScale == overrides.forceScale &&
      parameterOverrides.connectionRadius == overrides.connectionRadius &&
      parameterOverrides.colourMultiplier == overrides.colourMultiplier &&
      parameterOverrides.maxSpeed == overrides.maxSpeed) {
    return;
  }

  parameterOverrides = overrides;
}

void ParticleSet::clearParameterOverrides() {
  setParameterOverrides(ParameterOverrides {});
}

float ParticleSet::getTimeStepEffective() const {
  return parameterOverrides.timeStep.value_or(timeStep.get());
}

float ParticleSet::getVelocityDampingEffective() const {
  return parameterOverrides.velocityDamping.value_or(velocityDamping.get());
}

float ParticleSet::getAttractionStrengthEffective() const {
  return parameterOverrides.attractionStrength.value_or(attractionStrength.get());
}

float ParticleSet::getAttractionRadiusEffective() const {
  return parameterOverrides.attractionRadius.value_or(attractionRadius.get());
}

float ParticleSet::getForceScaleEffective() const {
  return parameterOverrides.forceScale.value_or(forceScale.get());
}

float ParticleSet::getConnectionRadiusEffective() const {
  return parameterOverrides.connectionRadius.value_or(connectionRadius.get());
}

float ParticleSet::getColourMultiplierEffective() const {
  return parameterOverrides.colourMultiplier.value_or(colourMultiplier.get());
}

float ParticleSet::getMaxSpeedEffective() const {
  return parameterOverrides.maxSpeed.value_or(maxSpeed.get());
}

ofParameterGroup& ParticleSet::getParameterGroup() {
  if (parameters.size() == 0) {
    parameters.setName(getParameterGroupName());
    parameters.add(strategy);
    parameters.add(maxParticles);
    parameters.add(maxParticleAge);
    parameters.add(timeStep);
    parameters.add(velocityDamping);
    parameters.add(attractionStrength);
    parameters.add(attractionRadius);
    parameters.add(forceScale);
    parameters.add(connectionRadius);
    parameters.add(colourMultiplier);
    parameters.add(particleDrawRadius);
    parameters.add(initialVelocityScale);
    parameters.add(maxSpeed);
    parameters.add(sortNeighborWindow);
    parameters.add(lineFadeExponent);
  }
  return parameters;
}

void ParticleSet::allocateResources(int count) {
  destroyResources();

  cpuParticles.assign(count, {});
  inverseSortedIndices.assign(count, -1);
  sortedIndices.assign(count, -1);
  sortedPositions.assign(count, glm::vec2(0.0f));
  sortedColors.assign(count, glm::vec4(0.0f));

  freeList.clear();
  for (int i = count - 1; i >= 0; --i) {
    freeList.push_back(i);
  }
  liveCount = 0;
  currentBuffer = 0;

  glGenVertexArrays(2, vaos.data());
  glGenBuffers(2, vbos.data());

  for (int i = 0; i < 2; ++i) {
    glBindVertexArray(vaos[i]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos[i]);
    glBufferData(GL_ARRAY_BUFFER, count * ParticleStride, cpuParticles.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, velocity)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, spinDraw)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, color)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, lifetime)));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(ParticleStride), reinterpret_cast<void*>(offsetof(GpuParticle, flags)));
  }
  glBindVertexArray(0);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  glGenBuffers(1, &particlePositionTBO);
  glBindBuffer(GL_TEXTURE_BUFFER, particlePositionTBO);
  glBufferData(GL_TEXTURE_BUFFER, count * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
  glGenTextures(1, &particlePositionTexture);
  glBindTexture(GL_TEXTURE_BUFFER, particlePositionTexture);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RG32F, particlePositionTBO);

  glGenBuffers(1, &sortedIndexTBO);
  glBindBuffer(GL_TEXTURE_BUFFER, sortedIndexTBO);
  glBufferData(GL_TEXTURE_BUFFER, count * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
  glGenTextures(1, &sortedIndexTexture);
  glBindTexture(GL_TEXTURE_BUFFER, sortedIndexTexture);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, sortedIndexTBO);

  glGenBuffers(1, &inverseIndexTBO);
  glBindBuffer(GL_TEXTURE_BUFFER, inverseIndexTBO);
  glBufferData(GL_TEXTURE_BUFFER, count * sizeof(int), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
  glGenTextures(1, &inverseIndexTexture);
  glBindTexture(GL_TEXTURE_BUFFER, inverseIndexTexture);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_R32I, inverseIndexTBO);

  glGenBuffers(1, &sortedColorTBO);
  glBindBuffer(GL_TEXTURE_BUFFER, sortedColorTBO);
  glBufferData(GL_TEXTURE_BUFFER, count * sizeof(glm::vec4), nullptr, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_TEXTURE_BUFFER, 0);
  glGenTextures(1, &sortedColorTexture);
  glBindTexture(GL_TEXTURE_BUFFER, sortedColorTexture);
  glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, sortedColorTBO);

  resourcesReady = true;
}

void ParticleSet::setupProxyVbo() {
  if (sortedPositions.empty()) {
    sortedPositions.assign(maxParticles.get(), glm::vec2(0.0f));
  }
  proxyVbo.setVertexData(sortedPositions.data(), maxParticles.get(), GL_DYNAMIC_DRAW);
}

void ParticleSet::destroyResources() {
  resourcesReady = false;
  liveCount = 0;
  freeList.clear();
  if (proxyVbo.getIsAllocated()) {
    proxyVbo.clear();
  }
  if (vaos[0] != 0) {
    glDeleteVertexArrays(2, vaos.data());
    vaos = { 0, 0 };
  }
  if (vbos[0] != 0) {
    glDeleteBuffers(2, vbos.data());
    vbos = { 0, 0 };
  }
  auto deleteBufferAndTexture = [](GLuint& buffer, GLuint& texture) {
    if (buffer != 0) {
      glDeleteBuffers(1, &buffer);
      buffer = 0;
    }
    if (texture != 0) {
      glDeleteTextures(1, &texture);
      texture = 0;
    }
  };
  deleteBufferAndTexture(particlePositionTBO, particlePositionTexture);
  deleteBufferAndTexture(sortedIndexTBO, sortedIndexTexture);
  deleteBufferAndTexture(inverseIndexTBO, inverseIndexTexture);
  deleteBufferAndTexture(sortedColorTBO, sortedColorTexture);
}

void ParticleSet::compileShaders() {
  const std::string updateVert = R"GLSL(
    #version 330

    layout(location = 0) in vec2 inPosition;
    layout(location = 1) in vec2 inVelocity;
    layout(location = 2) in vec2 inSpinDraw;
    layout(location = 3) in vec4 inColor;
    layout(location = 4) in vec2 inLifetime;
    layout(location = 5) in vec2 inFlags;

    uniform float deltaTime;
    uniform float velocityDamping;
    uniform float attractionStrength;
    uniform float attractionRadius;
    uniform float forceScale;
    uniform float maxSpeed;
    uniform int liveCount;
    uniform int sortNeighborWindow;

    uniform samplerBuffer sortedPositions;
    uniform isamplerBuffer inverseSortedIndices;

    out vec2 outPosition;
    out vec2 outVelocity;
    out vec2 outSpinDraw;
    out vec4 outColor;
    out vec2 outLifetime;
    out vec2 outFlags;

    void main() {
      vec2 position = inPosition;
      vec2 velocity = inVelocity;
      vec2 spinDraw = inSpinDraw;
      vec4 color = inColor;
      vec2 lifetime = inLifetime;
      vec2 flags = inFlags;

      if (flags.x < 0.5) {
        outPosition = position;
        outVelocity = vec2(0.0, 0.0);
        outSpinDraw = spinDraw;
        outColor = color;
        outLifetime = vec2(0.0, lifetime.y);
        outFlags = vec2(0.0, 0.0);
        return;
      }

      int sortedIndex = texelFetch(inverseSortedIndices, gl_VertexID).r;
      if (sortedIndex >= 0 && liveCount > 1 && abs(attractionStrength) > 0.0001) {
        int searchRadius = sortNeighborWindow;
        int startIdx = max(0, sortedIndex - searchRadius);
        int endIdx = min(liveCount - 1, sortedIndex + searchRadius);
        vec2 neighborCentroid = vec2(0.0, 0.0);
        int neighborCount = 0;
        for (int idx = startIdx; idx <= endIdx; ++idx) {
          if (idx == sortedIndex) {
            continue;
          }
          vec2 otherPosition = texelFetch(sortedPositions, idx).xy;
          vec2 delta = otherPosition - position;
          float distance = length(delta);
          if (distance > 0.0001 && distance <= attractionRadius) {
            neighborCentroid += otherPosition;
            neighborCount++;
          }
        }
        if (neighborCount > 0) {
          neighborCentroid /= float(neighborCount);
          vec2 dir = neighborCentroid - position;
          float distance = length(dir);
          if (distance > 0.0001) {
            vec2 normalizedDir = dir / distance;
            float forceMagnitude = attractionStrength * (1.0 - distance / max(attractionRadius, 0.0001));
            velocity += normalizedDir * forceMagnitude * forceScale;
          }
        }
      }

      if (spinDraw.x != 0.0) {
        float cs = cos(spinDraw.x);
        float sn = sin(spinDraw.x);
        velocity = vec2(cs * velocity.x - sn * velocity.y,
                        sn * velocity.x + cs * velocity.y);
      }

      velocity *= velocityDamping;
      float speed = length(velocity);
      if (speed > maxSpeed) {
        velocity = (velocity / speed) * maxSpeed;
      }

      position += velocity * deltaTime;
      position = fract(position);
      position = clamp(position, vec2(0.0), vec2(0.999));

      lifetime.x -= deltaTime;
      if (lifetime.x <= 0.0) {
        lifetime.x = 0.0;
        flags.x = 0.0;
      }

      outPosition = position;
      outVelocity = velocity;
      outSpinDraw = spinDraw;
      outColor = color;
      outLifetime = lifetime;
      outFlags = flags;
    }
  )GLSL";

  updateShader.setupShaderFromSource(GL_VERTEX_SHADER, updateVert);
  const GLchar* varyings[] = { "outPosition", "outVelocity", "outSpinDraw", "outColor", "outLifetime", "outFlags" };
  GLuint program = updateShader.getProgram();
  glTransformFeedbackVaryings(program, 6, varyings, GL_INTERLEAVED_ATTRIBS);
  updateShader.linkProgram();

  const std::string pointVert = R"GLSL(
    #version 330

    layout(location = 0) in vec2 inPosition;
    layout(location = 3) in vec4 inColor;
    layout(location = 2) in vec2 inSpinDraw;
    layout(location = 5) in vec2 inFlags;

    uniform mat4 modelViewProjectionMatrix;
    uniform float colourMultiplier;
    uniform vec2 viewportScale;

    out vec4 vColor;
    out float vAlive;

    void main() {
      vAlive = inFlags.x;
      float a = inColor.a * colourMultiplier;
      vColor = vec4(inColor.rgb * a, a);
      vec2 worldPos = inPosition * viewportScale;
      gl_Position = modelViewProjectionMatrix * vec4(worldPos, 0.0, 1.0);
      gl_PointSize = inSpinDraw.y;
    }
  )GLSL";

  const std::string pointFrag = R"GLSL(
    #version 330

    in vec4 vColor;
    in float vAlive;
    out vec4 fragColor;

    void main() {
      if (vAlive < 0.5) discard;
      vec2 coord = gl_PointCoord - vec2(0.5);
      if (length(coord) > 0.5) discard;
      fragColor = vColor;
    }
  )GLSL";

  pointShader.setupShaderFromSource(GL_VERTEX_SHADER, pointVert);
  pointShader.setupShaderFromSource(GL_FRAGMENT_SHADER, pointFrag);
  pointShader.linkProgram();

  const std::string lineVert = R"GLSL(
    #version 330

    layout(location = 0) in vec2 position;

    void main() {
      gl_Position = vec4(position, 0.0, 1.0);
    }
  )GLSL";

  const std::string lineGeom = R"GLSL(
    #version 330

    layout(points) in;
    layout(line_strip, max_vertices = 256) out;

    uniform mat4 modelViewProjectionMatrix;
    uniform float connectionRadius;
    uniform float colourMultiplier;
    uniform float lineFadeExponent;
    uniform int sortNeighborWindow;
    uniform int liveCount;
    uniform vec2 viewportScale;

    uniform samplerBuffer particlePositions;
    uniform samplerBuffer particleColors;
    uniform isamplerBuffer sortedIndices;

    out vec4 gColor;

    void emitConnection(vec2 a, vec2 b, vec4 colorA, vec4 colorB, float alpha) {
      vec2 scaledA = a * viewportScale;
      vec2 scaledB = b * viewportScale;
      float aA = colorA.a * colourMultiplier * alpha;
      float aB = colorB.a * colourMultiplier * alpha;
      vec4 ca = vec4(colorA.rgb * aA, aA);
      vec4 cb = vec4(colorB.rgb * aB, aB);
      gColor = ca;
      gl_Position = modelViewProjectionMatrix * vec4(scaledA, 0.0, 1.0);
      EmitVertex();
      gColor = cb;
      gl_Position = modelViewProjectionMatrix * vec4(scaledB, 0.0, 1.0);
      EmitVertex();
      EndPrimitive();
    }

    void main() {
      if (liveCount <= 1) {
        return;
      }
      int mySortedIndex = gl_PrimitiveIDIn;
      if (mySortedIndex >= liveCount) {
        return;
      }
      vec2 myPosition = texelFetch(particlePositions, mySortedIndex).xy;
      vec4 myColor = texelFetch(particleColors, mySortedIndex);
      int myOriginalIndex = texelFetch(sortedIndices, mySortedIndex).r;

      float radius = connectionRadius;
      float radiusSq = radius * radius;

      int searchRadius = sortNeighborWindow;
      int startIdx = max(0, mySortedIndex - searchRadius);
      int endIdx = min(liveCount - 1, mySortedIndex + searchRadius);
      int emitted = 0;
      const int maxLines = 128;

      for (int idx = startIdx; idx <= endIdx; ++idx) {
        if (idx <= mySortedIndex) continue;
        if (emitted >= maxLines) break;
        int otherOriginalIndex = texelFetch(sortedIndices, idx).r;
        if (otherOriginalIndex == myOriginalIndex) continue;
        vec2 otherPos = texelFetch(particlePositions, idx).xy;
        vec2 delta = otherPos - myPosition;
        float distanceSq = dot(delta, delta);
        if (distanceSq <= radiusSq && distanceSq > 0.0001) {
          float distance = sqrt(distanceSq);
          float alpha = pow(1.0 - (distance / radius), lineFadeExponent);
          vec4 otherColor = texelFetch(particleColors, idx);
          emitConnection(myPosition, otherPos, myColor, otherColor, alpha);
          emitted++;
        }
      }
    }
  )GLSL";

  const std::string lineFrag = R"GLSL(
    #version 330

    in vec4 gColor;
    out vec4 fragColor;

    void main() {
      fragColor = gColor;
    }
  )GLSL";

  lineShader.setupShaderFromSource(GL_VERTEX_SHADER, lineVert);
  lineShader.setupShaderFromSource(GL_GEOMETRY_SHADER, lineGeom);
  lineShader.setupShaderFromSource(GL_FRAGMENT_SHADER, lineFrag);
  lineShader.linkProgram();

  shadersReady = true;
}

void ParticleSet::ensureCapacity() {
  if (static_cast<int>(cpuParticles.size()) != maxParticles.get()) {
    allocateResources(maxParticles.get());
    setupProxyVbo();
  }
}

void ParticleSet::add(glm::vec2 position, glm::vec2 velocity, ofFloatColor color, float spin, float drawRadiusOverride) {
  NewParticleDatum datum;
  datum.position = position;
  datum.velocity = velocity;
  datum.color = color;
  datum.spin = spin;
  datum.drawRadius = drawRadiusOverride > 0.0f ? drawRadiusOverride : particleDrawRadius.get();
  pendingParticles.push_back(datum);
}

void ParticleSet::processPendingAdditions() {
  if (pendingParticles.empty() || freeList.empty()) {
    return;
  }
  float baseTimeStep = std::max(getTimeStepEffective(), 0.0001f);
  while (!pendingParticles.empty() && !freeList.empty()) {
    int index = freeList.back();
    freeList.pop_back();
    const auto datum = pendingParticles.front();
    pendingParticles.pop_front();

    GpuParticle& particle = cpuParticles[index];
    particle.position = glm::clamp(datum.position, glm::vec2(0.0f), glm::vec2(0.999f));
    particle.velocity = datum.velocity * initialVelocityScale.get();
    particle.spinDraw = glm::vec2(datum.spin, datum.drawRadius);
    particle.color = glm::vec4(datum.color.r, datum.color.g, datum.color.b, datum.color.a);
    float lifespanSteps = ofRandom(maxParticleAge.get());
    float lifetimeSeconds = std::max(lifespanSteps * baseTimeStep, baseTimeStep);
    particle.lifetime = glm::vec2(lifetimeSeconds, lifetimeSeconds);
    particle.flags = glm::vec2(1.0f, 0.0f);

    uploadParticleToBuffer(index);
  }
}

void ParticleSet::uploadParticleToBuffer(int particleIndex) {
  if (!resourcesReady) return;
  glBindBuffer(GL_ARRAY_BUFFER, vbos[currentBuffer]);
  glBufferSubData(GL_ARRAY_BUFFER,
                  particleIndex * ParticleStride,
                  ParticleStride,
                  &cpuParticles[particleIndex]);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleSet::runTransformFeedback(float deltaTime) {
  if (!resourcesReady || !shadersReady) return;

  int readBuffer = currentBuffer;
  int writeBuffer = 1 - currentBuffer;
  int neighborWindow = computeNeighborWindow();

  updateShader.begin();
  updateShader.setUniform1f("deltaTime", deltaTime);
  updateShader.setUniform1f("velocityDamping", getVelocityDampingEffective());
  updateShader.setUniform1f("attractionStrength", getAttractionStrengthEffective());
  updateShader.setUniform1f("attractionRadius", getAttractionRadiusEffective());
  updateShader.setUniform1f("forceScale", getForceScaleEffective());
  updateShader.setUniform1f("maxSpeed", getMaxSpeedEffective());
  updateShader.setUniform1i("liveCount", static_cast<int>(liveCount));
  updateShader.setUniform1i("sortNeighborWindow", neighborWindow);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, particlePositionTexture);
  updateShader.setUniform1i("sortedPositions", 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_BUFFER, inverseIndexTexture);
  updateShader.setUniform1i("inverseSortedIndices", 1);

  glBindVertexArray(vaos[readBuffer]);
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, vbos[writeBuffer]);

  glEnable(GL_RASTERIZER_DISCARD);
  glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0, maxParticles.get());
  glEndTransformFeedback();
  glDisable(GL_RASTERIZER_DISCARD);

  glBindVertexArray(0);
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_BUFFER, 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_BUFFER, 0);

  updateShader.end();

  currentBuffer = writeBuffer;
}

void ParticleSet::readBackParticles() {
  if (!resourcesReady) return;

  glBindBuffer(GL_ARRAY_BUFFER, vbos[currentBuffer]);
  void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_READ_ONLY);
  if (!ptr) {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return;
  }
  memcpy(cpuParticles.data(), ptr, cpuParticles.size() * ParticleStride);
  glUnmapBuffer(GL_ARRAY_BUFFER);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  freeList.clear();
  liveCount = 0;
  for (int i = 0; i < static_cast<int>(cpuParticles.size()); ++i) {
    if (cpuParticles[i].flags.x > 0.5f) {
      liveCount++;
    } else {
      freeList.push_back(i);
    }
  }
}

void ParticleSet::rebuildSpatialSort() {
  struct Entry {
    uint64_t morton;
    int index;
  };

  std::vector<Entry> entries;
  entries.reserve(liveCount);
  inverseSortedIndices.assign(cpuParticles.size(), -1);

  size_t sortedCount = 0;
  for (int i = 0; i < static_cast<int>(cpuParticles.size()); ++i) {
    if (cpuParticles[i].flags.x <= 0.5f) continue;
    Entry entry;
    entry.index = i;
    uint64_t mortonBase = calculateMortonCode(cpuParticles[i].position);
    uint64_t jitter = static_cast<uint64_t>(hashIndex(static_cast<uint32_t>(entry.index)) & MortonJitterMask);
    entry.morton = (mortonBase << MortonJitterBits) | jitter;
    entries.push_back(entry);
  }

  std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
    return a.morton < b.morton;
  });

  sortedCount = entries.size();
  for (size_t i = 0; i < sortedCount; ++i) {
    int originalIndex = entries[i].index;
    inverseSortedIndices[originalIndex] = static_cast<int>(i);
    sortedIndices[i] = originalIndex;
    sortedPositions[i] = cpuParticles[originalIndex].position;
    sortedColors[i] = cpuParticles[originalIndex].color;
  }
  liveCount = sortedCount;
}

void ParticleSet::uploadSortedData() {
  if (!resourcesReady) return;

  int live = static_cast<int>(liveCount);

  glBindBuffer(GL_TEXTURE_BUFFER, particlePositionTBO);
  glBufferSubData(GL_TEXTURE_BUFFER, 0, live * sizeof(glm::vec2), sortedPositions.data());
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  glBindBuffer(GL_TEXTURE_BUFFER, sortedIndexTBO);
  glBufferSubData(GL_TEXTURE_BUFFER, 0, live * sizeof(int), sortedIndices.data());
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  glBindBuffer(GL_TEXTURE_BUFFER, inverseIndexTBO);
  glBufferSubData(GL_TEXTURE_BUFFER, 0, inverseSortedIndices.size() * sizeof(int), inverseSortedIndices.data());
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  glBindBuffer(GL_TEXTURE_BUFFER, sortedColorTBO);
  glBufferSubData(GL_TEXTURE_BUFFER, 0, live * sizeof(glm::vec4), sortedColors.data());
  glBindBuffer(GL_TEXTURE_BUFFER, 0);

  if (proxyVbo.getIsAllocated() && live > 0) {
    proxyVbo.updateVertexData(sortedPositions.data(), live);
  }
}

int ParticleSet::computeNeighborWindow() const {
  if (liveCount <= 1) {
    return 0;
  }
  float normalizedRadius = std::max({ getConnectionRadiusEffective(), getAttractionRadiusEffective(), 0.0005f });
  float approxCells = normalizedRadius * static_cast<float>(MortonScale);
  int autoWindow = static_cast<int>(std::ceil(approxCells * MortonNeighborWindowMultiplier));
  autoWindow = std::max(autoWindow, 1);
  int userWindow = std::max(sortNeighborWindow.get(), 1);
  int desired = std::max(autoWindow, userWindow);
  int maxAllowed = static_cast<int>(liveCount) - 1;
  return std::max(1, std::min(desired, maxAllowed));
}

void ParticleSet::update() {
  ensureCapacity();
  if (!resourcesReady || !shadersReady) return;

  processPendingAdditions();

  float effectiveTimeStep = getTimeStepEffective();
  float delta = effectiveTimeStep > 0.0f ? effectiveTimeStep : ofGetLastFrameTime();
  delta = std::clamp(delta, 0.0001f, 0.1f);
  runTransformFeedback(delta);
  readBackParticles();
  rebuildSpatialSort();
  uploadSortedData();
}

void ParticleSet::draw(glm::vec2 viewportScale) {
  if (!resourcesReady || !shadersReady || liveCount == 0) return;

  ofPushStyle();

  int neighborWindow = computeNeighborWindow();

  bool drawPoints = strategy.get() != STRATEGY_CONNECTIONS;
  bool drawLines = strategy.get() != STRATEGY_POINTS;

  if (drawPoints) {
    glEnable(GL_PROGRAM_POINT_SIZE);
    pointShader.begin();
    pointShader.setUniformMatrix4f("modelViewProjectionMatrix", ofGetCurrentMatrix(OF_MATRIX_PROJECTION) * ofGetCurrentMatrix(OF_MATRIX_MODELVIEW));
    pointShader.setUniform1f("colourMultiplier", getColourMultiplierEffective());
    pointShader.setUniform2f("viewportScale", viewportScale);

    glBindVertexArray(vaos[currentBuffer]);
    glDrawArrays(GL_POINTS, 0, maxParticles.get());
    glBindVertexArray(0);

    pointShader.end();
    glDisable(GL_PROGRAM_POINT_SIZE);
  }

  if (drawLines && proxyVbo.getIsAllocated() && liveCount > 1) {
    lineShader.begin();
    lineShader.setUniformMatrix4f("modelViewProjectionMatrix", ofGetCurrentMatrix(OF_MATRIX_PROJECTION) * ofGetCurrentMatrix(OF_MATRIX_MODELVIEW));
    lineShader.setUniform1f("connectionRadius", getConnectionRadiusEffective());
    lineShader.setUniform1f("colourMultiplier", getColourMultiplierEffective());
    lineShader.setUniform1f("lineFadeExponent", lineFadeExponent.get());
    lineShader.setUniform1i("sortNeighborWindow", neighborWindow);
    lineShader.setUniform1i("liveCount", static_cast<int>(liveCount));
    lineShader.setUniform2f("viewportScale", viewportScale);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, particlePositionTexture);
    lineShader.setUniform1i("particlePositions", 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, sortedIndexTexture);
    lineShader.setUniform1i("sortedIndices", 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, sortedColorTexture);
    lineShader.setUniform1i("particleColors", 2);

    proxyVbo.draw(GL_POINTS, 0, static_cast<int>(liveCount));

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    lineShader.end();
  }

  ofPopStyle();
}
