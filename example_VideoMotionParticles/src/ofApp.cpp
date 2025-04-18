#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofDisableArbTex(); // required for texture2D to work in GLSL, makes texture coords normalized
  ofEnableAlphaBlending();

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGBA32F);
  fbo.begin();
  ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 0.0});
  ofFill();
  ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
  fbo.end();
  
  motionFromVideo.load(ofToDataPath("trimmed.mov"));
//  motionFromVideo.load(ofToDataPath("violin-trimmed.mov"));
}

//--------------------------------------------------------------
void ofApp::update(){
  motionFromVideo.update();
  
  ofFloatPixels loresPixels;
  motionFromVideo.getMotionFbo().readToPixels(loresPixels);
  loresPixels.resize(fbo.getWidth()/16.0, fbo.getHeight()/16.0);
  for (int i = 0; i < 15; i++) {
    float loresX = ofRandom(loresPixels.getWidth());
    float loresY = ofRandom(loresPixels.getHeight());
    auto c = loresPixels.getColor(loresX, loresY);
    if (c.r > 0.05 || c.r < -0.05) particleSet.add({loresX*16.0, loresY*16.0});
    if (c.g > 0.05 || c.g < -0.05) particleSet.add({loresX*16.0, loresY*16.0});
  }

//  particleSet.add({x, y});

  particleSet.update();

  fbo.begin();
  {
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 0.001});
    ofFill();
    ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
  }
  ofEnableBlendMode(OF_BLENDMODE_ADD);
//  particleSet.drawPoints();
  particleSet.drawConnections();

  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  ofFill();
  ofSetColor(ofFloatColor { 0.0, 0.0, 0.0, 1.0 });
  ofDrawRectangle(0, 0, ofGetWindowWidth(), ofGetWindowHeight());
  
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
