#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofEnableAlphaBlending();
  ofSetFrameRate(30);
  ofBackground(ofFloatColor{0.0, 0.0, 0.0, 1.0});

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fbo.begin();
  ofClear(ofFloatColor {0.0, 0.0, 0.0, 0.0});
  fbo.end();
  
  motionFromVideo.load("/Users/steve/Documents/music-source-material/belfast/trombone-trimmed.mov", true); // mute
//  motionFromVideo.load(ofToDataPath("violin-trimmed.mov"));
  
  parameters.add(particleSet.getParameterGroup());
  parameters.add(motionFromVideo.getParameterGroup());
  parameters.add(particleSpin);
  parameters.add(velocityScale);
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  motionFromVideo.update();
  auto scale = 1.0f / motionFromVideo.getSize();
  for (int i = 0; i < 100; i++) {
    if (auto vec = motionFromVideo.trySampleMotion()) {
      particleSet.add({vec->x * scale.x, vec->y * scale.y},
                      {vec->z * velocityScale * scale.x, vec->w * velocityScale * scale.y},
                      ofFloatColor{ofRandom(1.0), ofRandom(1.0), ofRandom(1.0), ofRandom(0.2)},
                      particleSpin);
    }
  }

  particleSet.update();

  fbo.begin();
  {
    // fade
    ofEnableBlendMode(OF_BLENDMODE_MULTIPLY);
    ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 0.001});
    ofFill();
    ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
    // particles
    ofEnableBlendMode(OF_BLENDMODE_ADD);
    auto viewportScale = glm::max(glm::vec2(ofGetWidth(), ofGetHeight()), glm::vec2(1.0f));
    particleSet.draw(viewportScale);
  }
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0 });
  fbo.draw(0, 0);
  
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 0.05 });
  motionFromVideo.getVideoFbo().draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
//  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 0.2 });
//  motionFromVideo.getMotionFbo().draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());

  gui.draw();
  ofSetWindowTitle(ofToString(ofGetFrameRate()) + " / " + ofToString(particleSet.size()));
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
