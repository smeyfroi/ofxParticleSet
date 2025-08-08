#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofEnableAlphaBlending();
  ofSetFrameRate(60);

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB32F);
  fbo.begin();
  ofSetColor(ofFloatColor { 0.3, 0.3, 0.3, 1.0});
  ofFill();
  ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::update(){
  particleSet.update();
  fbo.begin();
  ofBlendMode(OF_BLENDMODE_ALPHA);
  ofSetColor(ofFloatColor { 0.3, 0.3, 0.3, 0.005});
  ofFill();
  ofDrawRectangle(0, 0, fbo.getWidth(), fbo.getHeight());
  particleSet.draw();
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofBlendMode(OF_BLENDMODE_NONE);
  fbo.draw(0, 0);
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
void ofApp::mouseDragged(int x, int y, int button) {
  for (int i = 0; i < 10; i++) {
    particleSet.add({x, y, 0.0}, {ofRandom(5.0)-2.5, ofRandom(5.0)-2.5, 0.0}, ofFloatColor{ofRandom(1.0),ofRandom(1.0),ofRandom(1.0),ofRandom(1.0)}, ofRandom(0.1)-0.05);
  }
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
