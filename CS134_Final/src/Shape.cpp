#include "Shape.h"



Shape::Shape() { //default constructor
	position = glm::vec3(0, 0, 0);
	rotation = 0.0; // degrees, might want to change for 3d rotation but we'll keep it like this for now
	scale = glm::vec3(1, 1, 1);
	defaultSize = 20.0;
}

void Shape::draw() { // draw a box by default if not overridden
	ofPushMatrix();
	ofMultMatrix(getTransform());
	ofDrawBox(defaultSize);
	ofPopMatrix();
}

glm::mat4 Shape::getTransform() { //get transformation matrix based on position, rotation, and scale
	glm::mat4 T = glm::translate(glm::mat4(1.0), position); //translation matrix starts as identity matrix, then translated to position
	glm::mat4 R = glm::rotate(glm::mat4(1.0), glm::radians(rotation), glm::vec3(0, 0, -1)); //rotation matrix, 3rd parameter defaults to (0,0,-1) for z-axis rotation
	glm::mat4 S = glm::scale(glm::mat4(1.0), scale); //scaling matrix
	return T * R * S; //combined transformation matrix: scale first, then rotate, then translate (opposite order for multiplication)
}

glm::vec3 Shape::getPointInObjectSpace(glm::vec3 p) { //convert point p from world/screen space to object space
	return glm::inverse(getTransform()) * glm::vec4(p, 1.0); //multiply by inverse transform matrix
}
