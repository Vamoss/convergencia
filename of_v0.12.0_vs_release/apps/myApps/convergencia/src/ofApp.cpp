#include "ofApp.h"



//--------------------------------------------------------------
void ofApp::setup() {
	ofLog::setChannel(std::make_shared<ofCustomLoggerChannel>("log.txt", true));
	ofSetLogLevel(OF_LOG_VERBOSE);

	drawUI = true;

	laserWidth = 800;
	laserHeight = 800;
	laserManager.setCanvasSize(laserWidth, laserHeight);

	serial.listDevices();
	vector <ofSerialDeviceInfo> usbDevices = serial.getDeviceList();


	// if you don't want to manage your own GUI for your
	// app you can add extra params to the laser GUI
	laserManager.addCustomParameter(renderProfileLabel);
	laserManager.addCustomParameter(renderProfileIndex.set("Render Profile", 0, 0, 2));
	laserManager.addCustomParameter(dotIntensity.set("Dot Intensity", 1, 0, 1));
	laserManager.addCustomParameter(dotProbability.set("Dot Probability", 1, 0, 1));	

	laserManager.addCustomParameter(convergence.set("Convergence", 0, 0, 1));
	laserManager.addCustomParameter(amountOfEmissors.set("Emissors", 1, 0, MAX_EMISSORS));
	laserManager.addCustomParameter(timeSpeed.set("Animation speed", 1, 0, 4));

	laserManager.addCustomParameter(colour.set("Colour", ofColor(0, 255, 0), ofColor(0), ofColor(255)));
	laserManager.addCustomParameter(drawAttractors.set("Draw Attractors", false));
	laserManager.addCustomParameter(useNearestNeighbor.set("Nearest Neighbor", false));
	laserManager.addCustomParameter(arduinoIndex.set("Arduino Index", 0, 0, usbDevices.size()-1));	

	ofParameter<string> description;
	description.setName("description");
	description.set("INSTRUCTIONS : \nTAB to toggle output editor\nSPACE to hide the UI\nC to clear the particles");
	laserManager.addCustomParameter(description);

	arduinoHistory.setName("Arduino Values");
	laserManager.addCustomParameter(arduinoHistory);

	for (int i = 0; i < MAX_ATTRACTORS; i++)
	{
		Attractor attractor;
		attractor.x = laserWidth / 2.f;
		attractor.y = laserHeight;
		for (int j = 0; j < MAX_POINTS; j++)
		{
			float x = 0;//tbd
			float y = ofMap(j, 0, MAX_POINTS, laserHeight, 0);
			attractor.points.push_back(ofVec2f(x, y));
		}
		attractors.push_back(attractor);
	}

	int baud = 115200;
	ofLogNotice("Attempting to connect to serial device: ") << arduinoIndex;
	serial.setup(arduinoIndex, baud);
	timeLastTryConnect = ofGetElapsedTimef();
}

//--------------------------------------------------------------
void ofApp::update() {
	// prepares laser manager to receive new points
	laserManager.update();

	updateArduino();
}


void ofApp::draw() {
	ofSetWindowTitle(ofToString(ofGetFrameRate()) + " " + ofToString(particles.size()));

	ofBackground(15, 15, 20);

	drawDots();

	float time = ofGetElapsedTimef();
	for (int i = particles.size(); i < 200; i++) {
		float angle = ofNoise(time * 0.28f, (float)i * 100.0f) * TWO_PI * 2;
		float radius = sqrt(ofNoise(time * 1.85f, (float)i * 954.0f)) * 30;
		int x = laserWidth / 2 + sin(angle) * radius;
		int y = laserHeight - 40 + cos(angle) * radius;
		laserManager.drawDot(x, y, ofColor(100, 100, 100), dotIntensity, OFXLASER_PROFILE_FAST);
	}

	//float moveSpeed = ofMap(particles.size(), 10, 80, 0.5, 50, true);
	//laserManager.getLaser(0).scannerSettings.moveSpeed = moveSpeed;
	// cout << laserManager.getLaser(0).scannerSettings.moveSpeed << endl;

	// sends points to the DAC
	laserManager.send();

	if(drawUI)
		laserManager.drawUI();


}

void ofApp::updateArduino() {
	if (serial.isInitialized()) {
		int numBytesToRead = serial.available();
		if (numBytesToRead > 512) {
			numBytesToRead = 512;
		}
		if (numBytesToRead > 0) {
			serialReadBuffer.clear();
			serial.readBytes(serialReadBuffer, numBytesToRead);
			serialReadString = serialReadBuffer.getText();

			string fullMessage = "";
			for (int i = 0; i < serialReadString.length(); i++) {
				unsigned char character = serialReadString[i];
				if (character == '\n' || character == '\r' || character == '\t' || character == 13) {
					if (fullMessage.length() > 0) {
						processSerialData(ofToInt(fullMessage));
					}
					fullMessage = "";
				}
				else {
					fullMessage += character;
				}

				if (fullMessage.length() > 512) {
					fullMessage = "";
				}
			}
		}
	}
	else {
		float etimef = ofGetElapsedTimef();
		if (etimef - timeLastTryConnect > 10.0) {
			serial.getDeviceList();
			timeLastTryConnect = etimef;
			int baud = 115200;
			ofLogNotice("Attempting to connect to serial device: ") << arduinoIndex;
			serial.setup(arduinoIndex, baud);
		}
	}
}

void ofApp::processSerialData(int data) {
	float pct;
	if (arduinoValues.size() > 0) {
		int maxValue = *max_element(arduinoValues.begin(), arduinoValues.end());
		pct = ofClamp((float)data / (float)maxValue, 0, 1);
		convergence.set(pct * pct * pct * pct * pct * pct * pct * pct * pct);
		dotProbability.set(sqrt(pct));
	}
	else {
		pct = 1;
	}

	string lastHistory = ofToString(floor(pct * 100)) + "% " + ofToString(data);
	for (int i = 0; i < 12; i++) {
		if (i < arduinoValues.size()) {
			lastHistory += ", " + ofToString(arduinoValues[arduinoValues.size() - 1 - i]);
		}
	}
	arduinoHistory.set(lastHistory);

	arduinoValues.push_back(data);
	if (arduinoValues.size() > 30) {
		arduinoValues.erase(arduinoValues.begin());
	}
}


void ofApp::drawDots() {

	string renderProfile;
	switch (renderProfileIndex) {
	case 0:
		renderProfile = OFXLASER_PROFILE_DEFAULT;
		break;
	case 1:
		renderProfile = OFXLASER_PROFILE_DETAIL;
		break;
	case 2:
		renderProfile = OFXLASER_PROFILE_FAST;
		break;
	}
	renderProfileLabel = "Render Profile : OFXLASER_PROFILE_" + renderProfile;

	float originX = laserWidth / 2.f;
	float originY = laserHeight;


	float time = (float)ofGetFrameNum() / 100.f;

	//emissors
	for (int i = 0; i < amountOfEmissors; i++)
	{
		if (ofRandom(1) <= dotProbability) {
			Particle particle;
			particle.pos.x = laserWidth / 2;
			particle.pos.y = laserHeight - 30;
			particle.angle = ofRandom(-0.2, 0.2) + PI + HALF_PI;
			particle.attractor = floor(ofRandom(MAX_ATTRACTORS));
			particles.push_back(particle);
		}
	}

	//attractors
	for (int i = 0; i < MAX_ATTRACTORS; i++)
	{
		float convergency = ((float)i / (float)MAX_ATTRACTORS + 1.f) * 10.f;
		float phase = -time * convergency;
		float frequency = (sin(time) + 1.f) / 2.f * 10.f;
		Attractor& attractor = attractors[i];
		for (int j = 0; j < MAX_POINTS; j++)
		{
			float pct = (float)j / (float)MAX_POINTS;
			float radius = (sin(time + pct * PI) + 1.f) / 2.f * 300.f;
			attractor.points[j].x = originX + cos(pct * TWO_PI * frequency + phase) * pct * radius;
		}
	}


	const float MIN_VELOCITY = 1.f * timeSpeed;
	const float MAX_VELOCITY = 10.0f * timeSpeed;

	//*
	// particles
	for (int index = particles.size() - 1; index >= 0; index--) {
		Particle& particle = particles[index];
		particle.angle += (ofNoise(particle.pos.x / 10.f + ofRandom(-1, 1), particle.pos.y / 10.f, time) - 0.5f) * 0.2f;
		float vel = ofMap(particle.pos.y, originY, 0, MIN_VELOCITY, MAX_VELOCITY);
		particle.pos.x += cos(particle.angle) * vel;
		particle.pos.y += sin(particle.angle) * vel;

		if (particle.pos.x < 0 || particle.pos.x > laserWidth || particle.pos.y < 0 || particle.pos.y > laserHeight) {
			particles.erase(particles.begin() + index);
		}
		else {
			ofVec2f attractorPoint = findClosest(particle);
			if (attractorPoint.x != -1 && attractorPoint.y != -1) {
				float angle = findAngle(particle, attractorPoint);
				particle.angle += (custom_mod(angle, TWO_PI) - custom_mod(particle.angle, TWO_PI)) * convergence;
			}
		}
	}

	/**/

	if (drawAttractors) {
		for (auto& attractor : attractors) {
			for (auto& point : attractor.points) {
				laserManager.drawDot(point.x, point.y, ofColor(255, 0, 0), 1, renderProfile);
			}
		}
	}

	if (useNearestNeighbor) {
		vector<int> route = nearestNeighbor();
		for (int i = 0; i < route.size(); i++)
		{
			laserManager.drawDot(particles[route[i]].pos.x, particles[route[i]].pos.y, colour, dotIntensity, renderProfile);
		}
	}
	else {
		for (auto& particle : particles) {
			laserManager.drawDot(particle.pos.x, particle.pos.y, colour, dotIntensity, renderProfile);
		}
	}
}


//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs& e) {
	if (e.key == OF_KEY_TAB) {
		laserManager.selectNextLaser();
	}
	else if (e.key == ' ') {
		drawUI = !drawUI;
	}
	else if (e.key == 'c') {
		particles.clear();
	}
}

//--------------------------------------------------------------
void ofApp::mouseDragged(ofMouseEventArgs& e) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(ofMouseEventArgs& e) {

}

void ofApp::mouseReleased(ofMouseEventArgs& e) {

}

ofVec2f ofApp::findClosest(Particle& particle) {
	for (auto& point : attractors[particle.attractor].points) {
		if (point.y < particle.pos.y) {
			return point;
		}
	}
	return { -1, -1 };
}

float ofApp::findAngle(Particle& p1, ofVec2f& p2) {
	return atan2(p2.y - p1.pos.y, p2.x - p1.pos.x);
}

float ofApp::custom_mod(float x, float m) {
	return fmod((std::fmod(x, m) + m), m);
}

vector<int> ofApp::nearestNeighbor() {
	int n = particles.size();
	vector<int> path;

	if (n == 0)
		return path;

	vector<bool> visited(n, false);
	visited[0] = true;
	int current = 0;
	int next = 0;
	int minDist;
	path.push_back(0);

	for (int i = 0; i < n - 1; ++i) {
		minDist = numeric_limits<int>::max();
		for (int j = 1; j < n; ++j) {
			if (!visited[j] && j != current) {
				int dist = particles[current].pos.squareDistance(particles[j].pos);
				if (dist < minDist) {
					minDist = dist;
					next = j;
				}
			}
		}
		visited[next] = true;
		current = next;
		path.push_back(next);
	}

	return path;
}