#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup() {
  ofEnableAlphaBlending();
  ofSetFrameRate(30);

  fbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB32F);
  fbo.begin();
  ofClear(ofFloatColor { 0.0, 0.0, 0.0, 1.0});
  fbo.end();
  
  parameters.add(particleSet.getParameterGroup());
  gui.setup(parameters);
}

//--------------------------------------------------------------
void ofApp::update(){
  particleSet.update();
  fbo.begin();
  ofClear(ofFloatColor { 0.0, 0.0, 0.0, 1.0});
  auto viewportScale = glm::max(glm::vec2(ofGetWidth(), ofGetHeight()), glm::vec2(1.0f));
  ofEnableBlendMode(OF_BLENDMODE_ADD);
  //  ofBlendMode(OF_BLENDMODE_SCREEN);
//    ofBlendMode(OF_BLENDMODE_ALPHA);
  particleSet.draw(viewportScale);
  fbo.end();
}

//--------------------------------------------------------------
void ofApp::draw(){
  ofEnableBlendMode(OF_BLENDMODE_DISABLED);
  fbo.draw(0, 0);
  if (guiVisible) gui.draw();
  ofSetWindowTitle(ofToString(ofGetFrameRate()) + " / " + ofToString(particleSet.size()));
}

//--------------------------------------------------------------
void ofApp::exit(){
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
  if (key == OF_KEY_TAB) { guiVisible = not guiVisible; return; }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
  glm::vec2 viewport = glm::max(glm::vec2(ofGetWidth(), ofGetHeight()), glm::vec2(1.0f));
  glm::vec2 normalisedPosition = glm::clamp(glm::vec2(x, y) / viewport, glm::vec2(0.0f), glm::vec2(0.999f));
  for (int i = 0; i < 100; i++) {
    glm::vec2 pixelVelocity { ofRandom(6.0f) - 3.0f, ofRandom(6.0f) - 3.0f };
    glm::vec2 normalisedVelocity = pixelVelocity / viewport;
    particleSet.add(normalisedPosition,
                    normalisedVelocity,
                    ofFloatColor{ofRandom(1.0f), ofRandom(1.0f), ofRandom(1.0f), ofRandom(1.0f)},
                    ofRandom(0.1f) - 0.05f);
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
