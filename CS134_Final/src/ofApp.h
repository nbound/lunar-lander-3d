#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include  "ofxAssimpModelLoader.h"
#include "Octree.h"
#include "Shape.h"
#include "Particle.h"
#include "Lander.h"
#include <glm/gtx/intersect.hpp>


class ofApp : public ofBaseApp{

	public:
		void setup();
		void update();
		void draw();
		void drawShadedBox(float x, float y, float w, float h); // for drawing shaded boxes for ui
		void loadVbo();
		void loadExplosionVbo();
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y );
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void mouseEntered(int x, int y);
		void mouseExited(int x, int y);
		void windowResized(int w, int h);
		void dragEvent2(ofDragInfo dragInfo);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
		void drawAxis(ofVec3f);
		void initLightingAndMaterials();
		void initCamSetup();
		void updateTargetLight();  // Update spotlight to current landing zone
		void savePicture();
		void toggleWireframeMode();
		void togglePointsDisplay();
		void toggleSelectTerrain();
		void setCameraTarget();
		void setCameraTargetToLander(); // additional method for pointing easycam to lander position
		bool mouseIntersectPlane(ofVec3f planePoint, ofVec3f planeNorm, ofVec3f &point);
		bool raySelectWithOctree(ofVec3f &pointRet);
		glm::vec3 getMousePointOnPlane(glm::vec3 p , glm::vec3 n);

		int updateColPointList();

		// game state
		void startGame();
		void resetGame();
		void toggleGamePause();
		void winGame();
		void startDiagnosticMode();
		void stopDiagnosticMode();
		bool gameStarted;
		bool gamePaused;
		bool gameWon;
		bool diagnosticMode;

		float startTime;
		glm::vec3 startPosition;
	
		// Multi-stage landing system
		int currentLandingZone;  // 0 = need zone 1, 1 = need zone 2, 2 = need zone 3
		string achievementMessage;
		float achievementMessageAlpha;
		float achievementMessageTimer;		ofEasyCam cam;
		enum CameraView {
			EASYCAM,      // Free navigation camera
			TRACKING,     // Fixed position tracking the lander
			ONBOARD,       // Camera on the lander
			TOP,
			SIDE,
			FRONT,
			THIRDPERSON
		};

		CameraView currentCamera;
		ofCamera trackingCam;
		ofCamera onboardCam;
		ofCamera topCam;
		ofCamera sideCam;
		ofCamera frontCam;
		ofCamera thirdPersonCam;

		glm::vec3 landerFrontDirection;
		float frontCamDistance;
		float sideCamDistance;
		float topCamHeight;
		float thirdPersonDistance;
		float thirdPersonHeight;

		ofxAssimpModelLoader terrain, landerModel;
		ofLight light;
		Box boundingBox, landerBounds;
		Box testBox;
		vector<Box> colBoxList;
		vector<int> colPointList;
		bool bLanderSelected = false;
		Octree octree;
		int maxOctreeLevel;
		TreeNode selectedNode;
		glm::vec3 mouseDownPos, mouseLastPos;
		bool bInDrag = false;

		Lander lander;
		GravityForce * gravForce;
		TurbulenceForce * turbForce;
		TreeNode groundNode;
		bool aboveGround;

		ParticleEmitter thruster = ParticleEmitter();

		ParticleEmitter explosion = ParticleEmitter();  // Explosion emitter
		ImpulseRadialForce * explosionForce;            // Explosive burst force
		
		ofVbo vbo;
		ofShader shader;
		ofImage particleTex;

		ofxIntSlider numLevels;
		ofxIntSlider camSelect;
		ofxToggle bTimingInfo;
		ofxPanel pauseSettings;

		bool bAltKeyDown;
		bool bCtrlKeyDown;
		bool bWireframe;
		bool bDisplayPoints;
		bool bPointSelected;
		bool bHide;
		bool pointSelected = false;
		bool bDisplayLeafNodes = false;
		bool bDisplayOctree = false;
		bool bDisplayBBoxes = false;
		
		bool bLanderLoaded;
		bool bTerrainSelected;
	
		ofVec3f selectedPoint;
		ofVec3f intersectPoint;

		vector<Box> bboxList;

		const float selectionRange = 4.0;

		//Lighting with ofLight
		ofLight keyLight, targetLight, landerLight, fillLight;
		bool bLanderLightOn;
		ofImage backgroundImg;
		ofSpherePrimitive skySphere;

		// font
		ofTrueTypeFont font24;
		ofTrueTypeFont font16;

		const float MAXFUELTIME = 120.0; // seconds of fuel available
		float fuelTimeLeft;

		// Sound effects
		ofSoundPlayer achievementSound;
		ofSoundPlayer winningSound;
		ofSoundPlayer bgm;
	
		Box landingZone1;
		Box landingZone2;
		Box landingZone3;		
};
