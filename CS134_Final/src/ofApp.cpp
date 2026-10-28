
//--------------------------------------------------------------
//
// CS 134 Final Project  - 3D Lander Game
// Helper code provided by Professor Kevin M. Smith
// 
//  Student Names: Nathaniel Fadrigon, Minh Phong Do
//  Date: <date of last version>
//



#include "ofApp.h"
#include "Util.h"


//--------------------------------------------------------------
// setup scene, lighting, state and load geometry
//
void ofApp::setup(){  // by Nathaniel Fadrigon & Minh Phong Do
	ofDisableArbTex();  // Required for shaders to work
	gameStarted = false;
	font24.load("final/gunship.ttf", 24);
	font16.load("final/gunship.ttf", 16);
	fuelTimeLeft = MAXFUELTIME;

	backgroundImg.load("final/dustySky.jpg");
	skySphere.set(2000, 50);  // radius: 2000, resolution: 50
    skySphere.mapTexCoordsFromTexture(backgroundImg.getTexture());
	bWireframe = false;
	bDisplayPoints = false;
	bAltKeyDown = false;
	bCtrlKeyDown = false;
	bLanderLoaded = true; //lander is loaded on program start
	bTerrainSelected = true;
//	ofSetWindowShape(1024, 768);
	cam.setDistance(50);
	cam.setNearClip(.1);
	cam.setFov(65.5);   // approx equivalent to 28mm in 35mm format
	ofSetVerticalSync(true);
	cam.enableMouseInput(); // mouse input enabled for easycam by default
	ofEnableSmoothing();
	ofEnableDepthTest();

	glEnable(GL_CULL_FACE);      // turn on face culling
    glCullFace(GL_BACK);         // don’t draw back faces
    glFrontFace(GL_CCW);

	// setup rudimentary lighting 
	//
	initLightingAndMaterials();

	// cam setup
	initCamSetup();

	//load terrain
	terrain.load("final/snowFinal10.obj");
	terrain.setScaleNormalization(false);
	
	// initialize lander physics object
	lander = Lander();
	lander.thrusterSound.load("final/thrustersfx.mp3");
	lander.thrusterSound.setLoop(true);
	lander.thrusterSound.setVolume(0.5);
	lander.explosionSound.load("final/explosion.mp3");
	lander.explosionSound.setVolume(0.6);

	// Load game sound effects
	achievementSound.load("final/achieve.mp3");
	achievementSound.setVolume(0.7);
	winningSound.load("final/winning.mp3");
	winningSound.setVolume(0.5);

	bgm.load("final/nobonoko - PLNR - 06 Generator.mp3");
	bgm.setVolume(0.0);
	bgm.setLoop(true);
	bgm.play();

	// load lander model (should not need to be dragged in)
	landerModel.load("final/eveFinal3.obj");
	landerModel.setScaleNormalization(false);
	landerModel.setPosition(lander.position.x, lander.position.y, lander.position.z);
	lander.model = &landerModel;

	for (int i = 0; i < landerModel.getMeshCount(); i++) {
		bboxList.push_back(Octree::meshBounds(landerModel.getMesh(i)));
	}
	glm::vec3 min = landerModel.getSceneMin();
	glm::vec3 max = landerModel.getSceneMax();
	landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
	
	maxOctreeLevel = 12;

	// create GUI
	pauseSettings.setup("Settings", "settings.xml", (float)(ofGetWidth() / 2.2), (float)(ofGetHeight() / 2.5));

	pauseSettings.add(numLevels.setup("Octree Level Display: ", 1, 1, maxOctreeLevel));
	//pauseSettings.add(bTimingInfo.setup("Toggle Timing Info in Console ", false));
	pauseSettings.add(camSelect.setup("Cam Select", 1, 1, 7));
	bHide = true;

	//  create octree
	//
	ofMesh combinedMesh;
	for (int i = 0; i < terrain.getMeshCount(); i++) {
		ofMesh meshCopy = terrain.getMesh(i);
		combinedMesh.append(meshCopy);
	}

	// Create octree from combined mesh
	octree.create(combinedMesh, maxOctreeLevel);

	groundNode = octree.root;
	aboveGround = true;

	selectedPoint = octree.mesh.getVertex(octree.root.points[0]);

	// create color array for drawing octree
	//
	octree.colors = { ofColor::red, ofColor::orangeRed, ofColor::orange, ofColor::yellow, ofColor::yellowGreen,
		ofColor::greenYellow, ofColor::green, ofColor::turquoise, ofColor::blue, ofColor::indigo, ofColor::purple};


	// landing zone initialization
	landingZone1 = Box(Vector3(-29, 7, -92), Vector3(8, 12, -55));
	landingZone2 = Box(Vector3(116, 10, -55), Vector3(154, 15, -18));
	landingZone3 = Box(Vector3(72, -20, 87), Vector3(110, -15, 124));

	// initialize particle systems/forces
	gravForce = new GravityForce(glm::vec3(0.0, -8.0, 0.0));
	turbForce = new TurbulenceForce(glm::vec3(-10.0, -10.0, -10.0), glm::vec3(10.0, 10.0, 10.0));

	lander.thruster = &thruster;
	lander.explosion = &explosion;  // Connect explosion emitter to lander
	lander.initThruster();

	thruster.sys->addForce(gravForce);
	thruster.sys->addForce(turbForce);

	// Setup explosion particle system
	explosion.setEmitterType(RadialEmitter);
	explosion.oneShot = true;              // Fire once, then stop
	explosion.groupSize = 1000;             // 500 particles for big explosion
	explosion.setLifespan(2.0);              // Particles last 2 seconds
	explosion.setParticleRadius(5);        // Medium-sized particles
	explosion.setVelocity(ofVec3f(0, 50, 0)); // Initial upward velocity

	// Create explosive radial force (shoots particles outward)
	explosionForce = new ImpulseRadialForce(15000);  // Strong outward force
	explosion.sys->addForce(gravForce);             // Gravity pulls down
	explosion.sys->addForce(explosionForce);        // Explosive burst

	// Load particle texture (create programmatically since you may not have dot.png)
	particleTex.load("final/dot.png");

	// Setup shader (inline - no files needed)
	shader.setupShaderFromSource(GL_VERTEX_SHADER, R"(
		#version 120
		void main() {
			gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
			gl_PointSize = gl_Normal.x;
			gl_FrontColor = gl_Color;
		}
	)");

	shader.setupShaderFromSource(GL_FRAGMENT_SHADER, R"(
		#version 120
		uniform sampler2D tex;
		void main() {
			gl_FragColor = texture2D(tex, gl_PointCoord) * gl_Color;
		}
	)");
	shader.linkProgram();

	//initialize game state
	startPosition = glm::vec3(0, 20, 0);
	resetGame();
	
	// Set initial target light to zone 1
	updateTargetLight();
}
 
//--------------------------------------------------------------
//
void ofApp::update() {  // by Nathaniel Fadrigon & Minh Phong Do
	glm::mat4 R = glm::rotate(glm::mat4(1.0), glm::radians(lander.rotation), glm::vec3(0, -1, 0));
	// Update achievement message fade effect
	if (achievementMessageAlpha > 0) {
		float elapsed = ofGetElapsedTimef() - achievementMessageTimer;
		if (elapsed < 1.0) {
			// Fade in for 1 second
			achievementMessageAlpha = 255 * (elapsed / 1.0);
		} else if (elapsed < 3.0) {
			// Hold at full opacity for 2 seconds
			achievementMessageAlpha = 255;
		} else if (elapsed < 4.0) {
			// Fade out for 1 second
			achievementMessageAlpha = 255 * (1.0 - (elapsed - 3.0) / 1.0);
		} else {
			// Fully faded out
			achievementMessageAlpha = 0;
		}
	}

	if (gameStarted) {
		glm::mat4 R = glm::rotate(glm::mat4(1.0), glm::radians(lander.rotation), glm::vec3(0, -1, 0));
		if (!gamePaused && !gameWon) {
			// collision resolution (based on lander.model)
			// each update, get list of colliding octree points and store in colPointList
			updateColPointList();
			// if number of colliding points is great enough, consider the lander grounded
			if (colPointList.size() >= 1) {
				glm::vec3 avgNormal = glm::vec3(0, 0, 0);
				glm::vec3 impactForce = glm::vec3(0, 0, 0);

				//iterate through all the points in the boxes and find the average of all mesh normals that correspond to each point
				for (auto & point : colPointList) {
					avgNormal += octree.mesh.getNormal(point);
				}
				avgNormal = glm::normalize(avgNormal);

				// Check which landing zone we're currently targeting
				Box* targetZone = nullptr;
				if (currentLandingZone == 0) targetZone = &landingZone1;
				else if (currentLandingZone == 1) targetZone = &landingZone2;
				else if (currentLandingZone == 2) targetZone = &landingZone3;

				bool inZoneX = false;
				bool inZoneZ = false;
				
				if (targetZone != nullptr) {
					inZoneX = (lander.position.x >= targetZone->min().x() && 
								lander.position.x <= targetZone->max().x());
					inZoneZ = (lander.position.z >= targetZone->min().z() && 
								lander.position.z <= targetZone->max().z());
				}

				// move lander out of the ground based on mesh normal
				while (colPointList.size() >= 1) {
					lander.position += avgNormal * 0.01;
					lander.updateModel();
					updateColPointList();
				}

				if (inZoneX && inZoneZ) {
					// Successful landing in current zone!
					lander.velocity = glm::vec3(0, 0, 0);
					lander.angVelocity = 0;
					
					currentLandingZone++;
					
					if (currentLandingZone == 1) {
						// Just landed in zone 1
						achievementMessage = "ZONE 1 COMPLETE!";
						achievementMessageAlpha = 255;
						achievementMessageTimer = ofGetElapsedTimef();
						achievementSound.play();  // Play achievement sound
						updateTargetLight();  // Move spotlight to zone 2
					} else if (currentLandingZone == 2) {
						// Just landed in zone 2
						achievementMessage = "ZONE 2 COMPLETE!";
						achievementMessageAlpha = 255;
						achievementMessageTimer = ofGetElapsedTimef();
						achievementSound.play();  // Play achievement sound
						updateTargetLight();  // Move spotlight to zone 3
					} else if (currentLandingZone >= 3) {
						// Just landed in zone 3 - final victory!
						winGame();
					}
				} else {
					// check if velocity is above a certain limit, if it is, then lander explodes with high impact force
					if (glm::length(lander.velocity) > 3.0) {
						impactForce = avgNormal * glm::length(lander.velocity) * lander.mass * 1000;
						lander.die();  // Lander handles explosion internally now
					} else {
						// use a calmer, bouncy impact force
						impactForce = avgNormal * glm::length(lander.velocity) * lander.mass * 150;
					}
					lander.force += impactForce;
				}
			}

			if (lander.alive) {
				// apply lander forces
				gravForce->updateForce(&lander);
				turbForce->updateForce(&lander);

				if (fuelTimeLeft > 0) {
					if (lander.thrustForce != glm::vec3(0.0, 0.0, 0.0)) { // engine is on
						glm::vec4 thrust = R * glm::vec4(lander.thrustForce.x, lander.thrustForce.y, -lander.thrustForce.z, 0.0);
						lander.force += thrust;
						fuelTimeLeft -= ofGetLastFrameTime();
						lander.thrusterOn();
					} else {
						lander.thrusterOff();
					}
					lander.torque += lander.turnTorque;
				}
			}

			// update lander
			lander.update();

			// AGL ray detection
			aboveGround = octree.intersect(lander.aglRay, octree.root, groundNode);
			if (aboveGround) lander.agl = lander.position.y - octree.mesh.getVertex(groundNode.points[0]).y;
		} else {
			if (lander.thrusterSound.isPlaying()) {
				lander.thrusterSound.stop();
			}
		}
	} else if (diagnosticMode){ // diagnostic mode active (only reachable from game start screen)
		lander.position = lander.model->getPosition();
		startPosition = glm::vec3(lander.position);
		updateColPointList();
	}
	// camera updates
	currentCamera = static_cast<CameraView>((int)camSelect - 1);
	// Update tracking camera to always look at lander
	trackingCam.lookAt(lander.position);

	// Bottom camera (onboard)
	onboardCam.setPosition(lander.position.x, lander.position.y + 2, lander.position.z);
	onboardCam.lookAt(lander.position + glm::vec3(0, -1, 0));

	topCam.setPosition(lander.position.x, lander.position.y + topCamHeight, lander.position.z);
	topCam.lookAt(lander.position);

	glm::vec3 forward = glm::vec3(R * glm::vec4(0, 0, 1, 0));
	landerFrontDirection = glm::normalize(forward);

	glm::vec3 rightDirection = glm::normalize(glm::cross(landerFrontDirection, glm::vec3(0, 1, 0)));
	glm::vec3 sidePos = lander.position + (rightDirection * sideCamDistance);
	sidePos.y = lander.position.y + 5;
	sideCam.setPosition(sidePos);
	sideCam.lookAt(lander.position);

	glm::vec3 frontPos = lander.position + (landerFrontDirection * frontCamDistance);
	frontPos.y = lander.position.y + 5;
	frontCam.setPosition(frontPos);
	frontCam.lookAt(lander.position + glm::vec3(0, 5, 0));

	glm::vec3 thirdPersonPos = lander.position + (landerFrontDirection * thirdPersonDistance);
	thirdPersonPos.y = lander.position.y + thirdPersonHeight;
	thirdPersonCam.setPosition(thirdPersonPos);
	thirdPersonCam.lookAt(lander.position + glm::vec3(0, 5, 0));
	if (bLanderLightOn) {
		landerLight.setPosition(lander.position.x, lander.position.y, lander.position.z);
	}

	// Explosion is now handled inside lander.update()

	thruster.update();
	loadVbo();
	explosion.update();
	if (explosion.sys->particles.size() > 0) {
		loadExplosionVbo();
	}
}

//--------------------------------------------------------------
void ofApp::loadVbo() {
    if (thruster.sys->particles.size() == 0) return;
    
    vector<glm::vec3> points;
    vector<glm::vec3> sizes;
    vector<ofFloatColor> colors;
    
    for (int i = 0; i < thruster.sys->particles.size(); i++) {
        Particle &p = thruster.sys->particles[i];
        
        // Position
        points.push_back(p.position);
        
        // Size (stored in normal channel)
        // sizes.push_back(glm::vec3(p.radius * 2));
		float age_ratio = p.getAge() / p.lifespan;
		float size = p.radius * 2 * (1.0 + age_ratio * 3.0);  // Grows to 4x size
		sizes.push_back(glm::vec3(size));
        
		ofColor startColor = ofColor(15, 30, 80, 180);   // Dark blue
		ofColor endColor = ofColor(5, 10, 40, 0); 
        colors.push_back(startColor.lerp(endColor, age_ratio));
    }
    
    vbo.clear();
    vbo.setVertexData(&points[0], points.size(), GL_DYNAMIC_DRAW);
    vbo.setNormalData(&sizes[0], sizes.size(), GL_DYNAMIC_DRAW);
    vbo.setColorData(&colors[0], colors.size(), GL_DYNAMIC_DRAW);
}

//--------------------------------------------------------------
void ofApp::loadExplosionVbo() {
    if (explosion.sys->particles.size() == 0) return;
    
    vector<glm::vec3> points;
    vector<glm::vec3> sizes;
    vector<ofFloatColor> colors;
    
    for (int i = 0; i < explosion.sys->particles.size(); i++) {
        Particle &p = explosion.sys->particles[i];
        
        // Position
        points.push_back(p.position);
        
        // Size - grows then shrinks
        float age_ratio = p.getAge() / p.lifespan;
        float size = p.radius * 2 * (1.0 + age_ratio * 1.5);  // Grows moderately
        sizes.push_back(glm::vec3(size));
        
        // Color - yellow/orange explosion (fire colors)
        ofColor startColor = ofColor(255, 200, 50, 255);   // Bright yellow-orange
        ofColor endColor = ofColor(150, 50, 0, 0);          // Dark red, fade out
        colors.push_back(startColor.lerp(endColor, age_ratio));
    }
    
    vbo.clear();
    vbo.setVertexData(&points[0], points.size(), GL_DYNAMIC_DRAW);
    vbo.setNormalData(&sizes[0], sizes.size(), GL_DYNAMIC_DRAW);
    vbo.setColorData(&colors[0], colors.size(), GL_DYNAMIC_DRAW);
}

//--------------------------------------------------------------
void ofApp::draw() {

	ofBackground(ofColor::black);

	switch(currentCamera) {
        case EASYCAM:
            cam.begin();
            break;
        case TRACKING:
            trackingCam.begin();
            break;
        case ONBOARD:
            onboardCam.begin();
            break;
		case TOP:
			topCam.begin();
			break;
		case SIDE:
			sideCam.begin();
			break;
		case FRONT:
			frontCam.begin();
			break;
		case THIRDPERSON:
			thirdPersonCam.begin();
			break;
    }
	ofPushMatrix();

	// Sky sphere background (render first)
	//ofDisableLighting();
	glDepthMask(GL_FALSE);          // don’t write depth so sky stays behind
	glDisable(GL_CULL_FACE);        // avoid culling inside faces

	ofPushMatrix();
	// Flip sphere to render its inside (invert X to flip winding)
	ofScale(-1, 1, 1);
	ofSetColor(255);
	backgroundImg.bind();
	skySphere.draw();               // uses texcoords mapped in setup()
	backgroundImg.unbind();
	ofPopMatrix();

	// Restore state
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	if (bWireframe) {                    // wireframe mode  (include axis)
		ofDisableLighting();
		ofSetColor(ofColor::slateGray);
		terrain.drawWireframe();
		if (bLanderLoaded) {
			landerModel.drawWireframe();
			if (!bTerrainSelected) drawAxis(landerModel.getPosition());
		}
		if (bTerrainSelected) drawAxis(ofVec3f(0, 0, 0));
	}
	else {
		ofEnableLighting();              // shaded mode
		terrain.drawFaces();
		ofMesh mesh;
		if (bLanderLoaded && !lander.explosionTriggered) { 
			landerModel.drawFaces();
			if (!bTerrainSelected) drawAxis(landerModel.getPosition());
			if (bDisplayBBoxes) {
				ofNoFill();
				ofSetColor(ofColor::white);
				for (int i = 0; i < landerModel.getNumMeshes(); i++) {
					ofPushMatrix();
					ofMultMatrix(landerModel.getModelMatrix());
					//ofRotate(-90, 1, 0, 0);
					Octree::drawBox(bboxList[i]);
					ofPopMatrix();
				}
			}
			if (diagnosticMode) {
				ofSetColor(ofColor::white);
				ofNoFill();
				Octree::drawBox(landerBounds);
			}
		}
	}
	// render thruster particles 
	// render thruster particles with VBO + Shader
	if (thruster.sys->particles.size() > 0) {
		glDepthMask(GL_FALSE);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnablePointSprites();
		ofSetColor(255, 255, 255, 100);
		shader.begin();
		shader.setUniformTexture("tex", particleTex, 0);
		particleTex.bind();
		vbo.draw(GL_POINTS, 0, thruster.sys->particles.size());
		particleTex.unbind();
		shader.end();
		
		ofDisablePointSprites();
		ofDisableBlendMode();
		glDepthMask(GL_TRUE);
	}
	if (explosion.sys->particles.size() > 0) {
		glDepthMask(GL_FALSE);
		ofEnableBlendMode(OF_BLENDMODE_ADD);
		ofEnablePointSprites();
		
		shader.begin();
		shader.setUniformTexture("tex", particleTex, 0);
		particleTex.bind();
		vbo.draw(GL_POINTS, 0, explosion.sys->particles.size());
		particleTex.unbind();
		shader.end();
		
		ofDisablePointSprites();
		ofDisableBlendMode();
		glDepthMask(GL_TRUE);
	}

	// recursively draw octree
	//
	ofDisableLighting();
	int level = 0;

	if (bDisplayOctree) {
		ofNoFill();
		ofSetColor(ofColor::white);
		octree.draw(numLevels, level);
	}

	// draw landing zone area
	ofNoFill();
	ofSetColor(ofColor::red);
	switch (currentLandingZone) {
	case 0:
		octree.drawBox(landingZone1);
		break;
	case 1:
		octree.drawBox(landingZone2);
		break;
	case 2:
		octree.drawBox(landingZone3);
		break;
	default:
		break;
	}

	// if point selected, draw a sphere
	//
	if (pointSelected) {
		//selectedPoint = octree.mesh.getVertex(selectedNode.points[0]);
		ofVec3f d = selectedPoint - cam.getPosition();
		ofSetColor(ofColor::lightGreen);
		ofDrawSphere(selectedPoint, .02 * d.length());
	}

	ofPopMatrix();
	switch(currentCamera) {
        case EASYCAM:
            cam.end();
            break;
        case TRACKING:
            trackingCam.end();
            break;
        case ONBOARD:
            onboardCam.end();
            break;
		case TOP:
			topCam.end();
			break;
		case SIDE:
			sideCam.end();
			break;
		case FRONT:
			frontCam.end();
			break;
		case THIRDPERSON:
			thirdPersonCam.end();
			break;
    }

	// draw 2D UI elements

	glDepthMask(GL_FALSE);
	ofDisableDepthTest();
	ofDisableLighting();

	if (gameStarted) { // game HUD
		if (gamePaused) { //game pause state
			drawShadedBox(0, 0, ofGetWidth(), ofGetHeight());
			ofSetColor(ofColor::white);
			string pauseMenu1 = "GAME PAUSED";
			ofRectangle text1Bounds = font24.getStringBoundingBox(pauseMenu1, 0, 0);
			string pauseMenu2 = "Press Z to unpause";
			ofRectangle text2Bounds = font16.getStringBoundingBox(pauseMenu2, 0, 0);
			string pauseMenu3 = "Press R to reset game";
			ofRectangle text3Bounds = font16.getStringBoundingBox(pauseMenu3, 0, 0);
			font24.drawString(pauseMenu1, (ofGetWidth() - text1Bounds.width) / 2, (ofGetHeight() + text1Bounds.height) / 5);
			font16.drawString(pauseMenu2, (ofGetWidth() - text2Bounds.width) / 2, (ofGetHeight() + text2Bounds.height) / 5 + text1Bounds.height * 1.5);
			font16.drawString(pauseMenu3, (ofGetWidth() - text3Bounds.width) / 2, (ofGetHeight() + text3Bounds.height) / 5 + (text1Bounds.height + text2Bounds.height) * 1.5);
			pauseSettings.draw();
		} else if (gameWon) { // game win state
			drawShadedBox(0, 0, ofGetWidth(), ofGetHeight());
			string winMessage1 = "ALL ZONES COMPLETE!";
			ofRectangle text1Bounds = font24.getStringBoundingBox(winMessage1, 0, 0);
			string winMessage2 = "Press R to reset game";
			ofRectangle text2Bounds = font16.getStringBoundingBox(winMessage2, 0, 0);
			ofSetColor(ofColor::lightGreen);
			font24.drawString(winMessage1, (ofGetWidth() - text1Bounds.width) / 2, (ofGetHeight() + text1Bounds.height) / 2);
			ofSetColor(ofColor::white);
			font16.drawString(winMessage2, (ofGetWidth() - text2Bounds.width) / 2, (ofGetHeight() + text2Bounds.height) / 2 + text1Bounds.height * 1.5);
		} else {
			if (lander.alive) { // regular game state
				string velocityDisplay = "Velocity: " + ofToString(std::round(glm::length(lander.velocity)));
				string aglDisplay;
				if (aboveGround)
					aglDisplay = "AGL: " + ofToString(std::round(lander.agl * 100.0) / 100.0);
				else
					aglDisplay = "AGL: N/A";
				string fuelDisplay = "Fuel Time Left: " + ofToString(std::round(fuelTimeLeft * 100.0) / 100.0) + "s";
				if (fuelTimeLeft <= 0) fuelDisplay = "Out of Fuel";
				drawShadedBox(0, 0, ofGetWidth() / 5, font16.getStringBoundingBox(velocityDisplay, 0, 0).height * 3 + 60);
				ofSetColor(ofColor::white);
				font16.drawString(velocityDisplay, 10, font16.getStringBoundingBox(velocityDisplay, 0, 0).height + 10);
				font16.drawString(aglDisplay, 10, font16.getStringBoundingBox(velocityDisplay, 0, 0).height * 2 + 30);
				font16.drawString(fuelDisplay, 10, font16.getStringBoundingBox(velocityDisplay, 0, 0).height * 3 + 50);
				
				// Draw achievement message with fade effect
				if (achievementMessageAlpha > 0) {
					ofSetColor(ofColor::yellow, achievementMessageAlpha);
					float msgWidth = font24.getStringBoundingBox(achievementMessage, 0, 0).width;
					float msgHeight = font24.getStringBoundingBox(achievementMessage, 0, 0).height;
					font24.drawString(achievementMessage, 
									  (ofGetWidth() - msgWidth) / 2, 
									  msgHeight + 50);
				}
			} else if (!lander.isExploding() && explosion.sys->particles.size() == 0){ // game over screen
				drawShadedBox(0, 0, ofGetWidth(), ofGetHeight());
				string gameOverMessage1 = "GAME OVER";
				ofRectangle text1Bounds = font24.getStringBoundingBox(gameOverMessage1, 0, 0);
				string gameOverMessage2 = "Press R to reset game";
				ofRectangle text2Bounds = font16.getStringBoundingBox(gameOverMessage2, 0, 0);
				ofSetColor(ofColor::lightCoral);
				font24.drawString(gameOverMessage1, (ofGetWidth() - text1Bounds.width) / 2, (ofGetHeight() + text1Bounds.height) / 2);
				ofSetColor(ofColor::white);
				font16.drawString(gameOverMessage2, (ofGetWidth() - text2Bounds.width) / 2, (ofGetHeight() + text2Bounds.height) / 2 + text1Bounds.height * 1.5);
			}
		}
	} else if (diagnosticMode) { // diagnostic mode
		string dModeText1 = "Diagnostic mode";
		ofRectangle text1Bounds = font24.getStringBoundingBox(dModeText1, 0, 0);
		string dModeText2 = "Drag lander to new start position | Press C to toggle easycam control | Press V to return";
		ofRectangle text2Bounds = font16.getStringBoundingBox(dModeText2, 0, 0);
		drawShadedBox((ofGetWidth() - text2Bounds.width) / 2 - 20, 0, text2Bounds.width + 40, text1Bounds.height + text2Bounds.height + 40);
		ofSetColor(ofColor::white);
		font24.drawString(dModeText1, (ofGetWidth() - text1Bounds.width) / 2, text1Bounds.height + 10);
		font16.drawString(dModeText2, (ofGetWidth() - text2Bounds.width) / 2, text1Bounds.height + text2Bounds.height + 25);
	}else { // game start screen
		drawShadedBox(0, 0, ofGetWidth(), ofGetHeight());
		ofSetColor(ofColor::white);
		string startMessage1 = "3D Lander Game";
		ofRectangle text1Bounds = font24.getStringBoundingBox(startMessage1, 0, 0);
		string startMessage2 = "press 'Space' to start | press V to start diagnostic mode";
		ofRectangle text2Bounds = font16.getStringBoundingBox(startMessage2, 0, 0);
		font24.drawString(startMessage1, (ofGetWidth() - text1Bounds.width) / 2, (ofGetHeight() + text2Bounds.height) / 2 - 16);
		font16.drawString(startMessage2, (ofGetWidth() - text2Bounds.width) / 2, (ofGetHeight() + text2Bounds.height) / 2 + 12);
	}
	
	//if (!bHide) pauseSettings.draw();

	glDepthMask(GL_TRUE);
	ofEnableDepthTest();
	ofEnableLighting();

}

// 
// Draw an XYZ axis in RGB at world (0,0,0) for reference.
//
void ofApp::drawAxis(ofVec3f location) {

	ofPushMatrix();
	ofTranslate(location);

	ofSetLineWidth(1.0);

	// X Axis
	ofSetColor(ofColor(255, 0, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(1, 0, 0));
	

	// Y Axis
	ofSetColor(ofColor(0, 255, 0));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 1, 0));

	// Z Axis
	ofSetColor(ofColor(0, 0, 255));
	ofDrawLine(ofPoint(0, 0, 0), ofPoint(0, 0, 1));

	ofPopMatrix();
}

void ofApp::drawShadedBox(float x, float y, float w, float h) {
	ofEnableAlphaBlending();
	ofFill();
	ofSetColor(0, 0, 0, 200);
	ofDrawRectangle(x, y, w, h);
	ofNoFill();
	ofDisableAlphaBlending();
}

void ofApp::keyPressed(int key) {

	switch (key) {
	case '1':
		camSelect = EASYCAM + 1;
		break;
	case '2':
		camSelect = TRACKING + 1;
		break;
	case '3':
		camSelect = ONBOARD + 1;
		break;
	case '4':
		camSelect = TOP + 1;
		break;
	case '5':
		camSelect = SIDE + 1;
		break;
	case '6':
		camSelect = FRONT + 1;
		break;
	case '7':
		camSelect = THIRDPERSON + 1;
		break;
	case 'W':
	case 'w':
		if (lander.alive) {
			lander.thrustForce.z = lander.THRUST_PWR;
		}
		break;
	case 'S':
	case 's':
		if (lander.alive) {
			lander.thrustForce.z = -lander.THRUST_PWR;
		}
		break;
	case 'A':
	case 'a':
		if (lander.alive) {
			lander.thrustForce.x = -lander.THRUST_PWR;
		}
		break;
	case 'D':
	case 'd':
		if (lander.alive) {
			lander.thrustForce.x = lander.THRUST_PWR;
		}
		break;
	case 'Q':
	case 'q':
		if (lander.alive) {
			lander.turnTorque = -lander.TURN_PWR;
		}
		break;
	case 'E':
	case 'e':
		if (lander.alive) {
			lander.turnTorque = lander.TURN_PWR;
		}
		break;
	case OF_KEY_SPACE:
		if (!gameStarted && !diagnosticMode) startGame();
		if (lander.alive) {
			lander.thrustForce.y = lander.THRUST_PWR * 2;
		}
		break;
	case 'B':
	case 'b':
		bDisplayBBoxes = !bDisplayBBoxes;
		break;
	case 'C':
	case 'c':
		if (cam.getMouseInputEnabled()) cam.disableMouseInput();
		else cam.enableMouseInput();
		break;
	case 'F':
	case 'f':
		ofToggleFullscreen();
		break;
	case 'H':
	case 'h':
		break;
	case 'L':
	case 'l':
		bDisplayLeafNodes = !bDisplayLeafNodes;
		break;
	case 'O':
	case 'o':
		bDisplayOctree = !bDisplayOctree;
		break;
	case 'p':
		bLanderLightOn = !bLanderLightOn;
		if (bLanderLightOn) {
			landerLight.enable();
		}
		else {
			landerLight.disable();
		}
		break;
	case 'R':
	case 'r':
		resetGame();
		break;
	case 'T':
	case 't':
		setCameraTarget();
		break;
	case 'V':
	case 'v':
		if (!gameStarted) {
			if(!diagnosticMode) startDiagnosticMode();
			else stopDiagnosticMode();
		}
		break;
	case 'X':
	case 'x': // changed from w
		toggleWireframeMode();
		break;
	case 'Y':
	case 'y':
		setCameraTargetToLander();
		break;
	case 'Z':
	case 'z':
		toggleGamePause();
		break;
	case OF_KEY_ALT:
		cam.enableMouseInput();
		bAltKeyDown = true;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = true;
		break;
	case OF_KEY_DEL:
		break;
	default:
		break;
	}
}

void ofApp::toggleWireframeMode() {
	bWireframe = !bWireframe;
}

void ofApp::toggleSelectTerrain() {
	bTerrainSelected = !bTerrainSelected;
}

void ofApp::togglePointsDisplay() {
	bDisplayPoints = !bDisplayPoints;
}

void ofApp::keyReleased(int key) {

	switch (key) {
	case 'W':
	case 'w':
	case 'S':
	case 's':
		lander.thrustForce.z = 0;
		break;
	case 'A':
	case 'a':
	case 'D':
	case 'd':
		lander.thrustForce.x = 0;
		break;
	case 'Q':
	case 'q':
	case 'E':
	case 'e':
		lander.turnTorque = 0;
		break;
	case OF_KEY_SPACE:
		lander.thrustForce.y = 0;
		break;
	case OF_KEY_ALT:
		cam.disableMouseInput();
		bAltKeyDown = false;
		break;
	case OF_KEY_CONTROL:
		bCtrlKeyDown = false;
		break;
	case OF_KEY_SHIFT:
		break;
	default:
		break;

	}
}



//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

	
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	// if rover is loaded, test for selection
	//
	if (bLanderLoaded) {
		glm::vec3 origin = cam.getPosition();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);

		ofVec3f min = landerModel.getSceneMin() + landerModel.getPosition();
		ofVec3f max = landerModel.getSceneMax() + landerModel.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		bool hit = bounds.intersect(Ray(Vector3(origin.x, origin.y, origin.z), Vector3(mouseDir.x, mouseDir.y, mouseDir.z)), 0, 10000);
		if (hit) {
			bLanderSelected = true;
			mouseDownPos = getMousePointOnPlane(landerModel.getPosition(), cam.getZAxis());
			mouseLastPos = mouseDownPos;
			bInDrag = true;
		}
		else {
			bLanderSelected = false;
		}
	}
	else {
		ofVec3f p;
		//selectedPoint = p;
	}
}

bool ofApp::raySelectWithOctree(ofVec3f &pointRet) {
	ofVec3f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(mouse);
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	Ray ray = Ray(Vector3(rayPoint.x, rayPoint.y, rayPoint.z),
		Vector3(rayDir.x, rayDir.y, rayDir.z));

	pointSelected = octree.intersect(ray, octree.root, selectedNode);

	if (pointSelected) {
		pointRet = octree.mesh.getVertex(selectedNode.points[0]);
	}
	return pointSelected;
}




//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

	// if moving camera, don't allow mouse interaction
	//
	if (cam.getMouseInputEnabled()) return;

	if (bInDrag) {

		glm::vec3 landerPos = landerModel.getPosition();

		glm::vec3 mousePos = getMousePointOnPlane(landerPos, cam.getZAxis());
		glm::vec3 delta = mousePos - mouseLastPos;
	
		landerPos += delta;
		landerModel.setPosition(landerPos.x, landerPos.y, landerPos.z);
		mouseLastPos = mousePos;

		ofVec3f min = landerModel.getSceneMin() + landerModel.getPosition();
		ofVec3f max = landerModel.getSceneMax() + landerModel.getPosition();

		Box bounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));

		colBoxList.clear();
		octree.intersect(bounds, octree.root, colBoxList);

	}
	else {
		raySelectWithOctree(selectedPoint);
	}
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
	bInDrag = false;
}



// Set the camera to use the selected point as it's new target
//  
void ofApp::setCameraTarget() {
	cam.lookAt(selectedPoint);
}

void ofApp::setCameraTargetToLander() {
	cam.lookAt(lander.position);
}


//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

void ofApp::initCamSetup() { // by Minh Phong Do
	// Setup various cameras
	//
	currentCamera = EASYCAM;
	landerFrontDirection = glm::vec3(0, 0, 1);  // Default forward direction
	trackingCam.setPosition(130, 20, -40);
	frontCamDistance = 5.0;
	sideCamDistance = 20.0;
	topCamHeight = 20.0;
	thirdPersonDistance = 15.0;
	thirdPersonHeight = 10.0;
}

//--------------------------------------------------------------
// setup basic ambient lighting in GL
//
void ofApp::initLightingAndMaterials() { // by Minh Phong Do
	ofEnableLighting();
	ofSetGlobalAmbientColor(ofColor(150, 150, 150));
	keyLight.setup();
	keyLight.enable();
	keyLight.setDirectional();
	keyLight.setPosition(0, 400, 0);
	keyLight.setOrientation(glm::vec3(60, -60, 106));
	keyLight.setDiffuseColor(ofColor(170, 170, 170));
	keyLight.setSpecularColor(ofColor(200, 200, 255));
	keyLight.setAmbientColor(ofColor(120, 120, 120));

	targetLight.setup();
	targetLight.enable();
	targetLight.setSpotlight();
	targetLight.setScale(.05);
	targetLight.setSpotlightCutOff(7);
	targetLight.setPosition(130, 300, -40); // spotlight to target terrain
	targetLight.setOrientation(glm::vec3(-90, 0, 0)); // Point downward toward terrain
	targetLight.setAmbientColor(ofColor(255, 180, 120));
	targetLight.setDiffuseColor(ofColor(255, 200, 150));
	targetLight.setSpecularColor(ofColor(80, 60, 50));

	landerLight.setup();
	landerLight.disable();
	landerLight.setPointLight();
	landerLight.setAmbientColor(ofColor(80, 80, 80));
	landerLight.setDiffuseColor(ofColor(255, 255, 255));
	landerLight.setSpecularColor(ofColor(255, 255, 255));
	landerLight.setAttenuation(1.0, 0.01, 0.001);

	fillLight.setup();
	fillLight.enable();
	fillLight.setDirectional();

	fillLight.setOrientation(glm::vec3(-60, 120, 0));

	fillLight.setDiffuseColor(ofColor(180, 180, 180));
	fillLight.setAmbientColor(ofColor(60, 60, 60));
	fillLight.setSpecularColor(ofColor(0, 0, 0));

	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
} 

void ofApp::savePicture() {
	ofImage picture;
	picture.grabScreen(0, 0, ofGetWidth(), ofGetHeight());
	picture.save("screenshot.png");
	cout << "picture saved" << endl;
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent2(ofDragInfo dragInfo) {

	ofVec3f point;
	mouseIntersectPlane(ofVec3f(0, 0, 0), cam.getZAxis(), point);
	if (landerModel.loadModel(dragInfo.files[0])) {
		landerModel.setScaleNormalization(false);
//		lander.setScale(.1, .1, .1);
	//	lander.setPosition(point.x, point.y, point.z);
		landerModel.setPosition(1, 1, 0);

		bLanderLoaded = true;
		for (int i = 0; i < landerModel.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(landerModel.getMesh(i)));
		}

		cout << "Mesh Count: " << landerModel.getMeshCount() << endl;
	}
	else cout << "Error: Can't load model" << dragInfo.files[0] << endl;
}

bool ofApp::mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point) {
	ofVec2f mouse(mouseX, mouseY);
	ofVec3f rayPoint = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	ofVec3f rayDir = rayPoint - cam.getPosition();
	rayDir.normalize();
	return (rayIntersectPlane(rayPoint, rayDir, planePoint, planeNorm, point));
}

//--------------------------------------------------------------
//
// support drag-and-drop of model (.obj) file loading.  when
// model is dropped in viewport, place origin under cursor
//
void ofApp::dragEvent(ofDragInfo dragInfo) {
	if (landerModel.loadModel(dragInfo.files[0])) {
		bLanderLoaded = true;
		landerModel.setScaleNormalization(false);
		landerModel.setPosition(0, 0, 0);
		cout << "number of meshes: " << landerModel.getNumMeshes() << endl;
		bboxList.clear();
		for (int i = 0; i < landerModel.getMeshCount(); i++) {
			bboxList.push_back(Octree::meshBounds(landerModel.getMesh(i)));
		}

		glm::vec3 origin = cam.getPosition();
		glm::vec3 camAxis = cam.getZAxis();
		glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
		glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
		float distance;

		bool hit = glm::intersectRayPlane(origin, mouseDir, glm::vec3(0, 0, 0), camAxis, distance);
		if (hit) {
			// find the point of intersection on the plane using the distance
			glm::vec3 intersectPoint = origin + distance * mouseDir;

			// Now position the lander's origin at that intersection point
			//
			glm::vec3 min = landerModel.getSceneMin();
			glm::vec3 max = landerModel.getSceneMax();
			float offset = (max.y - min.y) / 2.0;
			landerModel.setPosition(intersectPoint.x, intersectPoint.y - offset, intersectPoint.z);

			// set up bounding box for lander while we are at it
			//
			landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
		}
	}


}

//  intersect the mouse ray with the plane normal to the camera 
//  return intersection point.   (package code above into function)
//
glm::vec3 ofApp::getMousePointOnPlane(glm::vec3 planePt, glm::vec3 planeNorm) {
	// Setup our rays
	//
	glm::vec3 origin = cam.getPosition();
	glm::vec3 camAxis = cam.getZAxis();
	glm::vec3 mouseWorld = cam.screenToWorld(glm::vec3(mouseX, mouseY, 0));
	glm::vec3 mouseDir = glm::normalize(mouseWorld - origin);
	float distance;

	bool hit = glm::intersectRayPlane(origin, mouseDir, planePt, planeNorm, distance);

	if (hit) {
		// find the point of intersection on the plane using the distance 
		glm::vec3 intersectPoint = origin + distance * mouseDir;

		return intersectPoint;
	}
	else return glm::vec3(0, 0, 0);
}
 
int ofApp::updateColPointList() { // by Nathaniel Fadrigon
	ofVec3f min = lander.model->getSceneMin() + lander.model->getPosition();
	ofVec3f max = lander.model->getSceneMax() + lander.model->getPosition();
	landerBounds = Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
	colPointList.clear();
	return octree.intersect(landerBounds, octree.root, colPointList);
}

void ofApp::resetGame() { // by Nathaniel Fadrigon & Minh Phong Do
	lander.position = startPosition;
	lander.rotation = 0.0;
	lander.alive = true;
	lander.explosionPending = false;
	lander.explosionTriggered = false;
	lander.explosionTimer = 0.0;
	lander.thrusterOff();
	lander.updateModel();
	
	// Reset explosion emitter so it can fire again
	explosion.stop();                      // Set started = false
	explosion.fired = false;               // Reset fired flag for oneShot emitter
	explosion.sys->particles.clear();      // Clear old particles
	explosion.sys->reset();                // Reset forces' applied flag
	
	// Reset multi-stage landing system
	currentLandingZone = 0;
	achievementMessage = "";
	achievementMessageAlpha = 0;
	achievementMessageTimer = 0;
	
	gameStarted = false;
	gameWon = false;
}

void ofApp::startGame() { // by Nathaniel Fadrigon
	gameStarted = true;
	gamePaused = false;
	startTime = ofGetElapsedTimef();
	fuelTimeLeft = MAXFUELTIME;
}

void ofApp::toggleGamePause() { // by Nathaniel Fadrigon
	if(gameStarted) gamePaused = !gamePaused;
}

void ofApp::winGame() { // by Nathaniel Fadrigon & Minh Phong Do
	gameWon = true;
	winningSound.play();  // Play winning sound
}

void ofApp::startDiagnosticMode() { // by Nathaniel Fadrigon
	diagnosticMode = true;
	camSelect = 1;
	lander.position = startPosition;
}

void ofApp::stopDiagnosticMode() { // by Nathaniel Fadrigon
	diagnosticMode = false;
	resetGame();
}

void ofApp::updateTargetLight() { // by Minh Phong Do
	// Calculate center position of current landing zone and position spotlight above it
	glm::vec3 centerPos;
	
	if (currentLandingZone == 0) {
		// Zone 1: (-29, 7, -92) to (8, 12, -55)
		centerPos = glm::vec3((-29 + 8) / 2.0f, 0, (-92 + -55) / 2.0f);
	} else if (currentLandingZone == 1) {
		// Zone 2: (116, 10, -55) to (154, 15, -18)
		centerPos = glm::vec3((116 + 154) / 2.0f, 0, (-55 + -18) / 2.0f);
	} else if (currentLandingZone == 2) {
		// Zone 3: (72, -20, 87) to (110, -15, 124)
		centerPos = glm::vec3((72 + 110) / 2.0f, 0, (87 + 124) / 2.0f);
	} else {
		// Game complete, keep last position
		return;
	}
	
	// Position light 300 units above center of zone
	targetLight.setPosition(centerPos.x, 300, centerPos.z);
}
