
//--------------------------------------------------------------
//
//  Kevin M. Smith
//
//  Simple Octree Implementation 11/10/2020
// 
//  Copyright (c) by Kevin M. Smith
//  Copying or use without permission is prohibited by law. 
//


#include "Octree.h"
 

//draw a box from a "Box" class  
//
void Octree::drawBox(const Box &box) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	ofVec3f p = ofVec3f(center.x(), center.y(), center.z());
	float w = size.x();
	float h = size.y();
	float d = size.z();
	ofDrawBox(p, w, h, d);
}

// return a Mesh Bounding Box for the entire Mesh
//
Box Octree::meshBounds(const ofMesh & mesh) {
	int n = mesh.getNumVertices();
	ofVec3f v = mesh.getVertex(0);
	ofVec3f max = v;
	ofVec3f min = v;
	for (int i = 1; i < n; i++) {
		ofVec3f v = mesh.getVertex(i);

		if (v.x > max.x) max.x = v.x;
		else if (v.x < min.x) min.x = v.x;

		if (v.y > max.y) max.y = v.y;
		else if (v.y < min.y) min.y = v.y;

		if (v.z > max.z) max.z = v.z;
		else if (v.z < min.z) min.z = v.z;
	}
	cout << "vertices: " << n << endl;
//	cout << "min: " << min << "max: " << max << endl;
	return Box(Vector3(min.x, min.y, min.z), Vector3(max.x, max.y, max.z));
}

// getMeshPointsInBox:  return an array of indices to points in mesh that are contained 
//                      inside the Box.  Return count of points found;
//
int Octree::getMeshPointsInBox(const ofMesh & mesh, const vector<int>& points,
	Box & box, vector<int> & pointsRtn)
{
	int count = 0;
	for (int i = 0; i < points.size(); i++) {
		ofVec3f v = mesh.getVertex(points[i]);
		if (box.inside(Vector3(v.x, v.y, v.z))) {
			count++;
			pointsRtn.push_back(points[i]);
		}
	}
	return count;
}

// getMeshFacesInBox:  return an array of indices to Faces in mesh that are contained 
//                      inside the Box.  Return count of faces found;
//
int Octree::getMeshFacesInBox(const ofMesh & mesh, const vector<int>& faces,
	Box & box, vector<int> & facesRtn)
{
	int count = 0;
	for (int i = 0; i < faces.size(); i++) {
		ofMeshFace face = mesh.getFace(faces[i]);
		ofVec3f v[3];
		v[0] = face.getVertex(0);
		v[1] = face.getVertex(1);
		v[2] = face.getVertex(2);
		Vector3 p[3];
		p[0] = Vector3(v[0].x, v[0].y, v[0].z);
		p[1] = Vector3(v[1].x, v[1].y, v[1].z);
		p[2] = Vector3(v[2].x, v[2].y, v[2].z);
		if (box.inside(p,3)) {
			count++;
			facesRtn.push_back(faces[i]);
		}
	}
	return count;
}

//  Subdivide a Box into eight(8) equal size boxes, return them in boxList;
//
void Octree::subDivideBox8(const Box & box, vector<Box> & boxList) {
	Vector3 min = box.parameters[0];
	Vector3 max = box.parameters[1];
	Vector3 size = max - min;
	Vector3 center = size / 2 + min;
	float xdist = (max.x() - min.x()) / 2;
	float ydist = (max.y() - min.y()) / 2;
	float zdist = (max.z() - min.z()) / 2;
	Vector3 h = Vector3(0, ydist, 0);

	//  generate ground floor
	//
	Box b[8];
	b[0] = Box(min, center);
	b[1] = Box(b[0].min() + Vector3(xdist, 0, 0), b[0].max() + Vector3(xdist, 0, 0));
	b[2] = Box(b[1].min() + Vector3(0, 0, zdist), b[1].max() + Vector3(0, 0, zdist));
	b[3] = Box(b[2].min() + Vector3(-xdist, 0, 0), b[2].max() + Vector3(-xdist, 0, 0));

	boxList.clear();
	for (int i = 0; i < 4; i++)
		boxList.push_back(b[i]);

	// generate second story
	//
	for (int i = 4; i < 8; i++) {
		b[i] = Box(b[i - 4].min() + h, b[i - 4].max() + h);
		boxList.push_back(b[i]);
	}
}

void Octree::create(const ofMesh & geo, int numLevels) {
	// initialize octree structure
	//
	mesh = geo;
	int level = 0;
	root.box = meshBounds(mesh);
	if (!bUseFaces) {
		for (int i = 0; i < mesh.getNumVertices(); i++) {
			root.points.push_back(i);
		}
	}
	// recursively buid octree
	//
	level++;
    subdivide(mesh, root, numLevels, level);
}
             
void Octree::subdivide(const ofMesh & mesh, TreeNode & node, int numLevels, int level) {
	if (level >= numLevels) return; //max octree levels reached
	level++;
	vector<Box> childBoxes; //to store boxes created with subDivideBox8
	
	subDivideBox8(node.box, childBoxes);
	
	for (int i = 0; i < childBoxes.size(); i++) {
		TreeNode n = TreeNode(); //create new node to store box and points
		vector<int> pointsInBox; //to store points returned by getMeshPointsInBox
		n.box = childBoxes[i];
		int numPoints = getMeshPointsInBox(mesh, node.points, n.box, n.points);

		if (n.points.size() > 0) {
			node.children.push_back(n);
			TreeNode& child = node.children.back();
			if (child.points.size() > 1) subdivide(mesh, child, numLevels, level); 
			//else child.bIsLeaf = true; can just check children.empty()
		}
		//
	}
	//parent node is now useless after subdivision, assign children to parent/root
}

// Implement functions below for Homework project
//

// recursive function for finding ray intersection with octree
// returns nodeRtn for the node found and bool intersects
//
bool Octree::intersect(const Ray &ray, const TreeNode & node, TreeNode & nodeRtn) {  // by Nathaniel Fadrigon
	bool intersects = node.box.intersect(ray, 0, FLT_MAX);
	if (!intersects) { //return early if no intersection
		return intersects;
	}
	if (node.children.empty()) { //leaf node
		nodeRtn = node;
		return intersects;
	}
	for (int i = 0; i < node.children.size(); i++) { // for each child node...
		const TreeNode& child = node.children.at(i);
		TreeNode tmp; 
		if (intersect(ray, child, tmp)) {
			nodeRtn = tmp; //only reassigns nodeRtn if intersects = true
			return true; //return when the first intersecting leaf node is found
		}
	}
	// if node isn't leaf node and no children intersect, return false
	return false;
}

// recursive function for finding box intersection with octree boxes
// returns boxListRtn for a list of intersected boxes found and bool intersects
//
bool Octree::intersect(const Box &box, TreeNode & node, vector<Box> & boxListRtn) {  // by Nathaniel Fadrigon
	bool intersects = node.box.overlap(box);
	if (!intersects) { //return early if no intersection
		return intersects;
	}
	if (node.children.empty()) { //leaf node
		//Box& b = node.box;
		boxListRtn.push_back(node.box); //add leaf node box to the boxList
		return intersects;
	}
	for (int i = 0; i < node.children.size(); i++) { // for each child node...
		TreeNode& child = node.children.at(i);
		intersect(box, child, boxListRtn); //recurse through all children
	}
	// if node isn't leaf node and no children intersect, return false
	return false;
}

// recursive function for finding box intersection with octree node points
// returns pointListRtn for a list of points contained in intersected nodes found and bool intersects
//
bool Octree::intersect( Box & box, TreeNode & node, vector<int> & pointListRtn) {  // by Nathaniel Fadrigon
	bool intersects = node.box.overlap(box);
	if (!intersects) { //return early if no intersection
		return intersects;
	}
	if (node.children.empty()) { //leaf node
		getMeshPointsInBox(mesh, node.points, box, pointListRtn);
		return intersects;
	}
	for (int i = 0; i < node.children.size(); i++) { // for each child node...
		TreeNode & child = node.children.at(i);
		intersect(box, child, pointListRtn); //recurse through all children
	}
	// if node isn't leaf node and no children intersect, return false
	return false;
}

// recursive function for drawing octree
//
void Octree::draw(TreeNode & node, int numLevels, int level) {  // by Nathaniel Fadrigon
	//add flag for intersection when in drag mode, save list of intersected leaf nodes and only draw those
	if (level >= numLevels) return; //max octree levels reached
	ofSetColor(colors[level%11]);
	drawBox(node.box);
	level++;
	for (int i = 0; i < node.children.size(); i++) {
		draw(node.children[i], numLevels, level);
	}
}




