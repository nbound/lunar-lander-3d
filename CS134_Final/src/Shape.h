#pragma once
#include "ofMain.h"

class Shape {
public:
	Shape();
	virtual void draw();

	virtual bool inside(glm::vec3 p) {
		return false;
	}

	virtual glm::mat4 getTransform();
	glm::vec3 getPointInObjectSpace(glm::vec3 p);

	glm::vec3 position;
	float rotation; //in degrees
	glm::vec3 scale;
	float defaultSize; //size of shape drawn by default
	glm::vec3 velocity; //vector indicating current velocity of shape
	float speed; //scalar representing magnitude of velocity
	glm::vec3 accel; //vector indicating rate of change of velocity
};
