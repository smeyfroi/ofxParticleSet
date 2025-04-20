#pragma once

#include "ofMain.h"
#include "ofxParticleSet.h"
#include "ofxMotionFromVideo.h"
#include "ofxGui.h"
#include "ofxAudioAnalysisClient.h"
#include "ofxAudioData.h"
#include "ofxSomPalette.h"

constexpr int SOM_WIDTH = 256;

class ofApp : public ofBaseApp{
  
public:
  void setup() override;
  void update() override;
  void draw() override;
  void exit() override;
  
  void keyPressed(int key) override;
  void keyReleased(int key) override;
  void mouseMoved(int x, int y ) override;
  void mouseDragged(int x, int y, int button) override;
  void mousePressed(int x, int y, int button) override;
  void mouseReleased(int x, int y, int button) override;
  void mouseScrolled(int x, int y, float scrollX, float scrollY) override;
  void mouseEntered(int x, int y) override;
  void mouseExited(int x, int y) override;
  void windowResized(int w, int h) override;
  void dragEvent(ofDragInfo dragInfo) override;
  void gotMessage(ofMessage msg) override;

private:
  ofFbo fbo;
  ParticleSet particleSet;
  MotionFromVideo motionFromVideo;
  std::shared_ptr<ofxAudioAnalysisClient::LocalGistClient> audioAnalysisClientPtr;
  std::shared_ptr<ofxAudioData::Processor> audioDataProcessorPtr;
  SomPalette somPalette { SOM_WIDTH, SOM_WIDTH, 0.015, 10000 };

  ofxPanel gui;
  ofParameterGroup parameters;
  ofParameter<float> particleSpin {"particleSpin", 0.0, -0.1, 0.1};
  ofParameter<float> velocityScale {"velocityScale", 5.0, 0.0, 50.0};
  ofParameter<int> potentialNewParticles {"potentialNewParticles", 100.0, 0.0, 1000.0};

  ofColor colorAt(float x, float y);
  void addParticles();
};
