#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofEnableAlphaBlending();
  ofBackground(ofFloatColor{0.0, 0.0, 0.0, 1.0});

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fbo.begin();
  ofClear(ofFloatColor {0.0, 0.0, 0.0, 0.0});
  fbo.end();
  
//  motionFromVideo.load(ofToDataPath("trimmed.mov"));
  motionFromVideo.load(ofToDataPath("violin-trimmed.mov"));
}

//--------------------------------------------------------------
constexpr float VELOCITY_SCALE = 3.0;
void ofApp::update(){
  motionFromVideo.update();
  
  ofFloatPixels pixels;
  motionFromVideo.getMotionFbo().readToPixels(pixels);
  for (int i = 0; i < 100; i++) {
    float x = ofRandom(pixels.getWidth());
    float y = ofRandom(pixels.getHeight());
    auto c = pixels.getColor(x, y);
    if (c.r > 0.05 || c.r < -0.05 || c.g > 0.05 || c.g < -0.05) {
      particleSet.add({x, y},
                      {c.r*VELOCITY_SCALE, c.g*VELOCITY_SCALE},
                      ofFloatColor{ofRandom(1.0), ofRandom(1.0), ofRandom(1.0), ofRandom(0.2)});
    }
  }

  particleSet.update();

  fbo.begin();
  {
    // fade
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 0.0005});
    ofFill();
    ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
    // particles
    ofEnableBlendMode(OF_BLENDMODE_ADD);
//  particleSet.drawPoints();
    particleSet.drawConnections();
  }
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 1.0 });
  fbo.draw(0, 0);
  
//  ofEnableBlendMode(OF_BLENDMODE_ALPHA);
//  ofSetColor(ofFloatColor { 1.0, 1.0, 1.0, 0.05 });
//  motionFromVideo.getVideoFbo().draw(0, 0, ofGetWindowWidth(), ofGetWindowHeight());

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
