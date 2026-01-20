#pragma once

#include "ofMain.h"
#include "ofxParticleSet.h"
#include "ofxMotionFromVideo.h"
#include "ofxGui.h"

class ofApp: public ofBaseApp {
public:
  void setup();
  void update();
  void draw();
  void exit();
  
  void keyPressed(int key);
  void keyReleased(int key);
  void mouseMoved(int x, int y);
  void mouseDragged(int x, int y, int button);
  void mousePressed(int x, int y, int button);
  void mouseReleased(int x, int y, int button);
  void windowResized(int w, int h);
  void dragEvent(ofDragInfo dragInfo);
  void gotMessage(ofMessage msg);

private:
  ofFbo fbo;
  ParticleSet particleSet;
  MotionFromVideo motionFromVideo;
 
  ofxPanel gui;
  ofParameterGroup parameters;
  ofParameter<float> particleSpin {"particleSpin", 0.0, -0.1, 0.1};
  ofParameter<float> velocityScale {"velocityScale", 5.0, 0.0, 50.0};
};
