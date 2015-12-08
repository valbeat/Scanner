#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ofBackground(0, 0, 0);
    #ifdef _USE_LIVE_VIDEO
    setupCamera();
    #else
    setupVideo();
    #endif
    
    setupGui();
    setupCam3d();
    
    image.allocate(camWidth, camHeight, GL_RGB);
    image.begin();
    ofClear(0,255);
    image.end();
    
    laserScan.allocate(camWidth, camHeight, GL_RGB);
    laserScan.begin();
    ofClear(0,255);
    laserScan.end();
    
    preview.allocate(camWidth, camHeight, GL_RGB);
    preview.begin();
    ofClear(0,255);
    preview.end();
    
    pointCloud.clearVertices();
    pointCloud.setMode(OF_PRIMITIVE_POINTS);
    
    // マウスカーソル非表示バグ回避
    ofHideCursor();
    
}
//--------------------------------------------------------------
void ofApp::update(){
    bool isNewFrame = false;
    #ifdef _USE_LIVE_VIDEO
    camera.update();
    isNewFrame = camera.isFrameNew();
    #else
    video.update();
    isNewFrame = video.isFrameNew();
    #endif
    if (isNewFrame) {
        if (isStart) {
            updateRotate();
        }
        #ifdef _USE_LIVE_VIDEO
        image.begin();
        camera.draw(0,0, camWidth, camHeight);
        image.end();
        #else
        image.begin();
        video.draw(0, 0, camWidth, camHeight);
        image.end();
        #endif

        ofPixels pixels;
        image.readToPixels(pixels);
        readLaserPixels(pixels);
        
        
        calc();
        
        createPointCloud();
        
        
        preview.begin();
        ofClear(0);
//        ofTranslate(camWidth / 2, 0);
        cam3d.begin();
        pointCloud.draw();
        cam3d.end();
        preview.end();
        
        laserPos.clear();

    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    
    ofSetColor(255);
    image.draw(0, 0, camWidth, camHeight);

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(255,100);
    laserScan.draw(0,0,camWidth,camHeight);
    ofDisableBlendMode();
    
    glEnable(GL_POINT_SMOOTH);
    glPointSize(3);
    ofEnableDepthTest();
    ofSetColor(255);
    preview.draw(camWidth,0,camWidth,camHeight);
    ofDisableDepthTest();
    glDisable(GL_POINT_SMOOTH);
    
    ofSetColor(255, 0, 0);
    ofRect(camWidth / 2 - x0, 0, 1, camHeight);
    ofSetColor(255);
    
    
    if (guiFlag) {
        ofSetColor(255);
        gui.draw();
    }
    
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    switch (key) {
        case 'g' :
            (guiFlag == false) ? guiFlag = true : guiFlag = false;
            break;
        case 'f':
            ofToggleFullscreen();
            break;
    }

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){
    //マウスカーソル非表示バグ回避
    ofShowCursor();
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
//--------------------------------------------------------------
void ofApp::setupCamera() {
    camWidth = 640;
    camHeight = 480;
    camera.setDesiredFrameRate(30);
    ofSetLogLevel(OF_LOG_VERBOSE);
    camera.setVerbose(true);
    camera.listDevices();
    camera.setDeviceID(0);
    camera.initGrabber(camWidth,camHeight);
}
//--------------------------------------------------------------
void ofApp::setupVideo() {
    camWidth = 640;
    camHeight = 480;
    video.loadMovie(VIDEO_NAME);
    video.play();
}

void ofApp::setupCam3d() {
    cam3d.setNearClip(1e-4);
    cam3d.setFarClip(1000);
    cam3d.setDistance(0);
    cam3d.setPosition(0,0,-100);
    cam3d.lookAt(ofVec3f(0,0,1));
}

//--------------------------------------------------------------
void ofApp::setupGui() {
    //ボタンの動作設定
    updateRotateButton.addListener(this, &ofApp::updateRotateButtonPressed);
    resetPointsButton.addListener(this, &ofApp::resetPointsButtonPressed);
    startScanButton.addListener(this, &ofApp::startScanButtonPressed);
    
    guiFlag = true;
    gui.setup();
    gui.add(laserBright.setup("laserBright",250,0,255));
    gui.add(d.setup("d(mm)", 60, 0, 200));
    gui.add(L.setup("L(mm)", 260, 0, 1000));
    gui.add(rotate.setup("theta",0,0,360));
//    gui.add(rotateInterval.setup("+theta",1,0,360));
    gui.add(updateRotateButton.setup("theta++"));
    gui.add(startScanButton.setup("start"));
    gui.add(resetPointsButton.setup("reset"));
    gui.loadFromFile("settings.xml");
    
}
//--------------------------------------------------------------
void ofApp::updateRotate() {
    int r = rotate;
    if (r > 360) return;
    r++;
    rotate.operator=(r);
}
//--------------------------------------------------------------
void ofApp::updateRotateButtonPressed() {
    updateRotate();
}
//--------------------------------------------------------------
void ofApp::startScanButtonPressed() {
    isStart ? isStart = false : isStart = true;
}
//--------------------------------------------------------------
void ofApp::resetPointsButtonPressed() {
    pointCloud.clear();
    isStart = false;
    rotate.operator=(0);
}
//--------------------------------------------------------------
void ofApp::readLaserPixels(ofPixels pixels) {
    laserScan.begin();
    ofClear(0,255);
    laserScan.end();
    int w = pixels.getWidth();
    int h = pixels.getHeight();

    for (int y = 0; y < h; y+= 10) {
        vector<int> v;
        for (int x = 0; x < w ; x++) {
            ofColor c = pixels.getColor(x, y);
            if(c.g > laserBright) {
                v.push_back(x);
            }
        }
        
        if (!v.empty()) {
            int mX = (int)median(v);
            laserPos.push_back(ofPoint(mX,y));
            
            // レーザーの中心を表示させる
            laserScan.begin();
            // ブレンドモードをONにした時はスムージングを切る
            ofDisableSmoothing();
            ofEnableBlendMode(OF_BLENDMODE_ADD);
            ofSetColor(0,255,0);
            ofRect(mX, y, 1, 1);
            ofSetColor(255, 255, 255);
            ofEnableAlphaBlending();
            ofEnableSmoothing();
            laserScan.end();
        }
    }
}
//--------------------------------------------------------------
// ポイントクラウド生成
void ofApp::createPointCloud() {
    if (!pos3Ds.empty()) {
        for (int i = 0; i < pos3Ds.size(); i++) {
            ofPoint pos = pos3Ds[i];
//            cout <<  "i:" << i << " x:" << pos.x << " y:" << pos.y << " z:" << pos.z << endl;
            pointCloud.addVertex(pos);
        }
    }
    pos3Ds.clear();
}
//--------------------------------------------------------------
// 計算部分
void ofApp::calc() {

    int Nx = camWidth; // スクリーン幅[pixel]
    float Lw; // スクリーン幅[mm]
    int lookPoint = (int)(camWidth / 2); // 注視点（カメラの中心）
    
    // dの[mm]→[pixel]変換
    x0 = (int)(d * RESOLUSION_WIDTH / 25.4); // [pixel]
    Lw = Nx / RESOLUSION_WIDTH * 25.4; // [mm]
    for (int i = 0; i < laserPos.size(); i++) {
        ofPoint pos = laserPos[i];
        float diff = abs(abs(pos.x - lookPoint) - x0);
//        float Xs,Ys,Zs;
        ofPoint p;
        float rad = rotate * DEG_TO_RAD;
        p.z = cos(rad) * diff * Nx * L * d;
        p.x = sin(rad) * diff;
        p.y = -pos.y + camHeight / 2;
//        p.z = (Nx * L * d) / (Nx * d + Lw * diff);
//        p.z = d *  / ;
//        point3d.x = L * sin(rad) * (1 - Nx * d + Lw * diff);
//        point3d.y = (-pos.y + 240) / RESOLUSION_HEIGHT;
        pos3Ds.push_back(p);
    }
}
//--------------------------------------------------------------
// 配列の平均値を返す
float ofApp::mean(vector<int> v) {
    int size = v.size();
    int sum = 0;
    for (int i = 0; i < size; i++){
        sum += v[i];
    }
    return sum / size;
}
//--------------------------------------------------------------
// 配列の中央値を返す
float ofApp::median(vector<int> v) {
    int size = v.size();
    vector<int> _v;
    copy(v.begin(), v.end(), back_inserter(_v));
    int tmp;
    for (int i = 0; i < size - 1; i++){
        for (int j = i + 1; j < size; j++) {
            if (_v[i] > _v[j]){
                tmp = _v[i];
                _v[i] = _v[j];
                _v[j] = tmp;
            }
        }
    }
    if (size % 2 == 1) {
        return _v[(size - 1) / 2];
    } else {
        return (_v[(size / 2) - 1] + _v[size / 2]) / 2;
    }
}
//--------------------------------------------------------------