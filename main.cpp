#include <iostream>
#include <cassert>
#include <random>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/type_precision.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "gl_core_3_3.h"
#include <GL/freeglut.h>
#include "util.hpp"
#include "mesh.hpp"
using namespace std;
using namespace glm;

// Mesh vertex format
struct Vtx {
	vec3 pos;		// Position
	vec3 norm;		// Normal
};

// Ray vertex format
struct Ray {
	vec3 orig;
	vec3 dir;
};

// Global state
GLint width, height;			// Window size
GLuint texWidth, texHeight;		// Texture size
vector<glm::u8vec3> texData;	// Texture pixel data
GLuint texture;			// Texture object
GLuint shader;			// Shader program
GLuint uniXform;		// Shader location of xform mtx
GLuint vao;				// Vertex array object
GLuint vbuf;			// Vertex buffer
GLuint ibuf;			// Index buffer
GLsizei vcount;			// Number of vertices
std::mt19937 rng;		// Random number generator


// Drawing state
bool drawing;			// Whether we are drawing
glm::u8vec3 drawColor;	// What color to draw in
Mesh* mesh;
vector<Vtx> objVerts;
vector<vec3> orthogonalVerts; // relative to +z axis direction
vector<vec3> perspectiveVerts;
vector<vec3> pushbroomVerts;
vector<vec3> imagePlaneVerts;
int objType;			// 7:cube 8:teapot 9:3d_triangle 10:teapot_less
int glcType;			// 4:perspective 5:orthogonal 6:pushbroom
float transX;
float transY;
float transZ;
float rotateY;
float rotateX;
glm::u8vec3 bgColor;

// Constants
const int MENU_CHANGE_BG_COLOR = 2;
const int MENU_EXIT = 3;			// Exit application
const int GLC_PERSPECTIVE = 4;			// Perspective GLC
const int GLC_ORTHOGONAL = 5;			// Perspective GLC
const int GLC_PUSHBROOM = 6;
const int OBJ_CUBE = 7;
//const int OBJ_TEAPOT = 8;
const int OBJ_3DTRIANGLE = 9;
const int OBJ_TEAPOT_LESS = 10;

// Initialization functions
void initState();
void initGLUT(int* argc, char** argv);
void initOpenGL();
void initTexture();
void initPlaneVerts();

// Callback functions
void display();
void reshape(GLint width, GLint height);
void keyRelease(unsigned char key, int x, int y);
void mouseBtn(int button, int state, int x, int y);
void mouseMove(int x, int y);
void idle();
void menu(int cmd);
void cleanup();
void keyboard(unsigned char key, int x, int y);

//Util functions
glm::u8vec3 randColor();

int main(int argc, char** argv) {
	try {
		cout << "Instruction: "<< endl;
		cout << "Left click mouse to get menu.Use WASD to translate the objects." << endl;
		cout << "Use R or T to rotate the object in x and y axis in adding 30 degree per time." << endl;
		cout <<	"Extra: Can change to different objects and camera in real time. Use the object's normal vector to create normal shading."<< endl;
		cout << "Other: Can change background color in real-time randomly." << endl;
		cout << "Attection: Please don't click the window when rendering the objects or it will redisplay and waste more time." << endl;
		// Initialize
		initState();
		initGLUT(&argc, argv);
		initOpenGL();
		initTexture();
		initPlaneVerts();

	} catch (const exception& e) {
		// Handle any errors
		cerr << "Fatal error: " << e.what() << endl;
		cleanup();
		return -1;
	}

	// Execute main loop
	glutMainLoop();

	return 0;
}

void initState() {
	// Initialize global state
	width = 0;
	height = 0;
	texWidth = 256;
	texHeight = 256;
	bgColor = u8vec3(255, 255, 255);
	texData.resize(texWidth * texHeight, bgColor);
	texture = 0;
	shader = 0;
	uniXform = 0;
	vao = 0;
	vbuf = 0;
	ibuf = 0;
	vcount = 0;
	mesh = NULL;
	objType = OBJ_CUBE;
	glcType = GLC_PERSPECTIVE;
	transX = 0.f;
	transY = 0.f;
	transZ = 0.f;
	rotateY = 0.f;
	rotateX = 0.f;

	// Initialize random number generator
	std::random_device rd;
	rng = std::mt19937(rd());
}

void initGLUT(int* argc, char** argv) {
	// Set window and context settings
	width = 800; height = 600;
	glutInit(argc, argv);
	glutInitWindowSize(width, height);
	glutInitContextVersion(3, 3);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	// Create the window
	glutCreateWindow("GLC Window | Use WASD to translate | Use R or T to rotate");

	// Create a menu
	glutCreateMenu(menu);
	glutAddMenuEntry("Perspective View", GLC_PERSPECTIVE);
	glutAddMenuEntry("Orthogonal View", GLC_ORTHOGONAL);
	glutAddMenuEntry("PushBroom View", GLC_PUSHBROOM);
	glutAddMenuEntry("Cube", OBJ_CUBE);
	//glutAddMenuEntry("Teapot", OBJ_TEAPOT);
	glutAddMenuEntry("3D Triangle", OBJ_3DTRIANGLE);
	glutAddMenuEntry("Teapot 3d less", OBJ_TEAPOT_LESS);
	glutAddMenuEntry("Change background color", MENU_CHANGE_BG_COLOR);
	glutAddMenuEntry("Exit", MENU_EXIT);
	glutAttachMenu(GLUT_RIGHT_BUTTON);

	// GLUT callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardUpFunc(keyRelease);
	//glutMouseFunc(mouseBtn);
	glutIdleFunc(idle);
	glutCloseFunc(cleanup);
	glutKeyboardFunc(keyboard);
}

void initOpenGL() {
	// Set clear color and depth
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	// Allow unpacking non-aligned pixel data
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// Compile and link shader program
	vector<GLuint> shaders;
	shaders.push_back(compileShader(GL_VERTEX_SHADER, "sh_v.glsl"));
	shaders.push_back(compileShader(GL_FRAGMENT_SHADER, "sh_f.glsl"));
	shader = linkProgram(shaders);
	// Release shader sources
	for (auto s = shaders.begin(); s != shaders.end(); ++s)
		glDeleteShader(*s);
	shaders.clear();
	// Locate uniforms
	uniXform = glGetUniformLocation(shader, "xform");
	GLuint uniTex = glGetUniformLocation(shader, "tex");

	// Bind texture image unit
	glUseProgram(shader);
	glUniform1i(uniTex, 0);
	glUseProgram(0);

	assert(glGetError() == GL_NO_ERROR);
}

void initTexture() {
	// Create a surface (quad) to draw the texture onto
	struct vert {
		glm::vec3 pos;	// 2D Position (assume z=0)
		glm::vec2 tc;	// Texture coordinates
	};
	vector<vert> verts = {
		{ glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f) },
		{ glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f) },
		{ glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec2(1.0f, 1.0f) },
		{ glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec2(0.0f, 1.0f) },
	};
	vector<GLubyte> ids = { 0, 1, 2, 2, 3, 0 };
	vcount = ids.size();

	// Create vertex array object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create vertex buffer
	glGenBuffers(1, &vbuf);
	glBindBuffer(GL_ARRAY_BUFFER, vbuf);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(vert), verts.data(), GL_STATIC_DRAW);
	// Specify vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vert), 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vert), (GLvoid*)sizeof(glm::vec3));
	// Create index buffer
	glGenBuffers(1, &ibuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ids.size() * sizeof(GLubyte), ids.data(), GL_STATIC_DRAW);

	// Cleanup state
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Create texture object
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, texData.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);

	assert(glGetError() == GL_NO_ERROR);
}

void initPlaneVerts() {
	// orthogonal Camera model vector definition
	orthogonalVerts = {
		vec3(-1.0f, -1.0f, 1.0f),
		vec3(1.0f, -1.0f, 1.0f),
		vec3(1.0f, 1.0f, 1.0f)
	};

	// perspective camera model vector definition
	perspectiveVerts = {
		vec3(-0.5f, -0.5f, 1.0f),
		vec3(0.5f, -0.5f, 1.0f),
		vec3(0.5f, 0.5f, 1.0f),
	};

	// pushbroom camera model vector definition
	pushbroomVerts = {
		vec3(-1.f, 0.f, 0.f),
		vec3(1.f, 0.f, 0.f),
		vec3(1.f, 0.f, 0.f)
	};

	// image place vertor definition
	imagePlaneVerts = {
		vec3(-1.0f, -1.0f, 0.0f),
		vec3(1.0f, -1.0f, 0.0f),
		vec3(1.0f, 1.0f, 0.0f)
	};
}


void drawMesh(Mesh* mesh, mat4 xform) {
	// Scale and center mesh using bounding box
	pair<vec3, vec3> meshBB = mesh->boundingBox();
	mat4 fixBB = scale(mat4(1.0f), vec3(1.0f / length(meshBB.second - meshBB.first)));
	fixBB = glm::translate(fixBB, -(meshBB.first + meshBB.second) / 2.0f);
	// Concatenate all transformations and upload to shader
	xform = xform * fixBB;
	glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));

	// Draw mesh
	mesh->draw();
}

float RayTriangleIntersection(Ray ray, vector<Vtx> triangle) {
	vec3 norm = triangle[0].norm;
	vec3 v1 = triangle[0].pos;
	vec3 v2 = triangle[1].pos;
	vec3 v3 = triangle[2].pos;
	float depth = -99999.0;

	// Decide whether the ray can interact with the plane
	if (fabs(dot(norm, ray.dir)) < 0.001) {
		// It means ray and triangle plane will not have intersection
		return depth;
	}
	// It has intersection with the plane (not exactly with triangle!)
	// Get intersection point 
	float t = dot(v1 - ray.orig, norm) / dot(ray.dir, norm);
	if (t < 0) {
		return depth;
	}
	vec3 P = ray.orig + t * ray.dir;

	// Decide whether P is inside the triangle
	// Define 3 edge vector according to counter clockwise
	vec3 e1 = v2 - v1;
	vec3 e2 = v3 - v2;
	vec3 e3 = v1 - v3;
	// Define 3 vector from P to each vertex coordinate
	vec3 pp1 = P - v1;
	vec3 pp2 = P - v2;
	vec3 pp3 = P - v3;
	// Use right hand rule: decide whether P is located on the left side of each vector ei
	// if yes: then P inside the triangle
	if (dot(cross(e1, pp1), norm) >= 0 &&
		dot(cross(e2, pp2), norm) >= 0 &&
		dot(cross(e3, pp3), norm) >= 0) {
		// return the P's depth
		depth = P.z;
	}

	//cout << "depth: " << depth << endl;
	return depth;
}

vec3 samplerObjectTriangle(vector<Vtx> triangle) {
	return triangle[0].norm;
}

vec3 castRay2Objects(Ray ray, vector<Vtx> vertices) {
	float minDepth = -99999.0;
	vec3 norm = vec3(0.0f);
	for (int i = 0; i < vertices.size(); i += 3) {
		vector<Vtx> triangle;
		triangle.push_back(vertices[i+0]);
		triangle.push_back(vertices[i+1]);
		triangle.push_back(vertices[i+2]);
	
		// !Attention to the Depth +-
		float depth = RayTriangleIntersection(ray, triangle);
		if (depth > minDepth) {
			minDepth = depth;
			norm = samplerObjectTriangle(triangle);
		}
	}

	return norm;
}

u8vec3 generateColor(vec3 norm) {
	// normalize norm
	norm = normalize(norm);
	int r = 255 * (norm.x * 0.5f + 0.5f);
	int g = 255 * (norm.y * 0.5f + 0.5f);
	int b = 255 * (norm.z * 0.5f + 0.5f);
	//cout << "rgb: " << r << ", " << g << ", " << b << endl;
	u8vec3 color(r, g, b);
	return color;
}

Ray generateRay(vector<vec3> imagePlaneVerts, vector<vec3> frontPlaneVerts, vec3 texPixelPos) {
	// Get each coord from defined verts
	float s1 = imagePlaneVerts[0].x, t1 = imagePlaneVerts[0].y;
	float s2 = imagePlaneVerts[1].x, t2 = imagePlaneVerts[1].y;
	float s3 = imagePlaneVerts[2].x, t3 = imagePlaneVerts[2].y;
	float si = texPixelPos.x, ti = texPixelPos.y;
	vec2 sti(texPixelPos);
	vec2 uv1(frontPlaneVerts[0]);
	vec2 uv2(frontPlaneVerts[1]);
	vec2 uv3(frontPlaneVerts[2]);
	
	// GLC formula calculation
	float demo_alpha = s1 * t2 + s2 * t3 + s3 * t1 - s3 * t2 - s1 * t3 - s2 * t1;
	float demo_beta = s2 * t1 + s1 * t3 + s3 * t2 - s3 * t1 - s2 * t3 - s1 * t2;
	float alpha = (si * t2 + s2 * t3 + s3 * ti - si * t3 - s3 * t2 - s2 * ti) / demo_alpha;
	float beta = (si * t1 + s3 * ti + s1 * t3 - si * t3 - s1 * ti - s3 * t1) / demo_beta;
	vec2 uvi = alpha * uv1 + beta * uv2 + (1 - alpha - beta) * uv3;

	vec3 orig = vec3(uvi, 1.0f);
	vec3 dir = vec3(sti - uvi, -1.0f);
	Ray ray = {
		orig,
		dir
	};
	//cout << "----------------" << endl;
	//cout << ray.orig.x << ", " << ray.orig.y << ", " << ray.orig.z << endl;
	//cout << ray.dir.x << ", " << ray.dir.y << ", " << ray.dir.z << endl;
	//cout << "----------------" << endl;
	return ray;
}

vec3 texData2WorldCoords(int idx, int texW, int texH, int clipW, int clipH) {
	// From index to s,t
	int s = idx % texW;
	int t = idx / texW;
	// Translate
	s = s - 0.5 * texW;  // + 0.5;
	t = t - 0.5 * texH;  //+ 0.5;
	// Scale
	float ss = s * ((float)clipW / texW);
	float tt = t * ((float)clipH / texH);
	
	return vec3(ss, tt, 0.0f);
}

// Change the color of the texture pixel at the given mouse coordinates
void drawPoint(glm::ivec2 texPos, glm::u8vec3 color) {
	if (texPos.x >= 0 && texPos.x < texWidth && texPos.y >= 0 && texPos.y < texHeight) {
		// If inside the texture, color and re-upload
		auto idx = texPos.y * texWidth + texPos.x;
		texData[idx] = color;

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, texData.data());
		glBindTexture(GL_TEXTURE_2D, 0);

		glutPostRedisplay();
	}
}

// Generate a random color
glm::u8vec3 randColor() {
	std::uniform_int_distribution<unsigned> distr(1, 32);
	glm::u8vec3 color(
		(distr(rng) << 3) - 1,
		(distr(rng) << 3) - 1,
		(distr(rng) << 3) - 1);
	return color;
}

void GLCRender(vector<vec3> uvPlaneVerts, vector<Vtx> objVerts, vector<u8vec3>texData) {

	for (int i = 0; i < texData.size(); i++) {
		vec3 curPixelPos = texData2WorldCoords(i, texWidth, texHeight, 5, 5);
		//cout << curPixelPos.x << ", " << curPixelPos.y << endl;
		Ray ray = generateRay(imagePlaneVerts, uvPlaneVerts, curPixelPos);
		vec3 norm = castRay2Objects(ray, objVerts);
		
		if (norm.x == 0 && norm.y == 0 && norm.z == 0) {
			// No intersection
			texData[i] = bgColor;
		}
		else {
			// Has intersection
			u8vec3 color = generateColor(norm);
			texData[i] = color;
		}

		glBindTexture(GL_TEXTURE_2D, texture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, texData.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

void loadMesh(Mesh* mesh) {
	objVerts.clear(); // Mush clear to avoid data overlap
	vector<vec3> rawVerts = vector<vec3>(mesh->raw_vertices.size());

	// Rotate all the vertices
	mat4 xform = mat4(1.f);
	mat4 transMat = translate(mat4(1.f), vec3(transX, transY, transZ));
	mat4 rotateMat = rotate(mat4(1.f), radians(rotateY), vec3(0, 1, 0));
	mat4 rotateMat2 = rotate(mat4(1.f), radians(rotateX), vec3(1, 0, 0));
	xform = rotateMat2 * rotateMat * transMat * xform;
	for (int i = 0; i < mesh->raw_vertices.size(); i++) {
		vec4 rawVert = vec4(mesh->raw_vertices[i], 1.f);
		rawVert = xform * rawVert;
		rawVerts[i] = vec3(rawVert.x, rawVert.y, rawVert.z - 5.0f);
		//cout << rawVerts[i].x << ", " << rawVerts[i].y << ", " << rawVerts[i].z << endl;
	}

	// Regenerate the vertices
	objVerts = vector<Vtx>(mesh->v_elements.size());
	for (int i = 0; i < mesh->v_elements.size(); i += 3) {
		// Store positions
		objVerts[i + 0].pos = rawVerts[mesh->v_elements[i + 0]];
		objVerts[i + 1].pos = rawVerts[mesh->v_elements[i + 1]];
		objVerts[i + 2].pos = rawVerts[mesh->v_elements[i + 2]];
		// Calculate normals
		vec3 normal = normalize(cross(objVerts[i + 1].pos - objVerts[i + 0].pos,
			objVerts[i + 2].pos - objVerts[i + 0].pos));
		objVerts[i + 0].norm = normal;
		objVerts[i + 1].norm = normal;
		objVerts[i + 2].norm = normal;
	}
}

void display() {

	vector<vec3> GLCVerts = perspectiveVerts;

	try {
		// Clear the back buffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get ready to draw
		glUseProgram(shader);

		// Fix aspect ratio
		glm::mat4 xform(1.0f);
		float winAspect = (float)width / (float)height;
		float texAspect = (float)texWidth / (float)texHeight;
		xform[0][0] = glm::min(1.0f, texAspect / winAspect);
		xform[1][1] = glm::min(1.0f, winAspect / texAspect);
		// Send transformation matrix to shader
		glUniformMatrix4fv(uniXform, 1, GL_FALSE, value_ptr(xform));

		// Load Mesh to get vertex pos and norm
		switch (objType) {
		case OBJ_CUBE:
			mesh = new Mesh("models/cube.obj");
			cout << "loading cube..." << endl;
			break;
		case OBJ_TEAPOT_LESS:
			mesh = new Mesh("models/teapot_less.obj");
			cout << "loading teapot in 3d less..." << endl;
			break;
		case OBJ_3DTRIANGLE:
			mesh = new Mesh("models/3d_triangle.obj");
			cout << "loading 3D triangle..." << endl;
			break;
		/*case OBJ_TEAPOT:
			mesh = new Mesh("models/teapot.obj");
			cout << "loading teapot..." << endl;
			break;*/
		}

		loadMesh(mesh);

		switch (glcType) {
		case GLC_PERSPECTIVE:
			GLCVerts = perspectiveVerts;
			break;
			
		case GLC_ORTHOGONAL:
			GLCVerts = orthogonalVerts;
			break;

		case GLC_PUSHBROOM:
			GLCVerts = pushbroomVerts;
			break;
		}
		
		GLCRender(GLCVerts, objVerts, texData);

		// Draw the textured quad
		glBindVertexArray(vao);
		glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, texture);
		glDrawElements(GL_TRIANGLES, vcount, GL_UNSIGNED_BYTE, NULL);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindVertexArray(0);

		// Revert context state
		glUseProgram(0);

		// Display the back buffer
		glutSwapBuffers();

	} catch (const exception& e) {
		cerr << "Fatal error: " << e.what() << endl;
		glutLeaveMainLoop();
	}
}

void reshape(GLint width, GLint height) {
	::width = width;
	::height = height;
	glViewport(0, 0, width, height);
}

void keyRelease(unsigned char key, int x, int y) {
	switch (key) {
	case 27:	// Escape key
		menu(MENU_EXIT);
		break;
	}
}

// Convert a position in screen space into texture space
glm::ivec2 mouseToTexCoord(int x, int y) {
	glm::vec3 mousePos(x, y, 1.0f);

	// Convert screen coordinates into clip space
	glm::mat3 screenToClip(1.0f);
	screenToClip[0][0] = 2.0f / width;
	screenToClip[1][1] = -2.0f / height;	// Flip y coordinate
	screenToClip[2][0] = -1.0f;
	screenToClip[2][1] = 1.0f;

	// Invert the aspect ratio correction (from display())
	float winAspect = (float)width / (float)height;
	float texAspect = (float)texWidth / (float)texHeight;
	glm::mat3 invAspect(1.0f);
	invAspect[0][0] = glm::max(1.0f, winAspect / texAspect);
	invAspect[1][1] = glm::max(1.0f, texAspect / winAspect);

	// Convert to texture coordinates
	glm::mat3 quadToTex(1.0f);
	quadToTex[0][0] = texWidth / 2.0f;
	quadToTex[1][1] = texHeight / 2.0f;
	quadToTex[2][0] = texWidth / 2.0f;
	quadToTex[2][1] = texHeight / 2.0f;

	// Get texture coordinate that was clicked on
	glm::ivec2 texPos = glm::ivec2(glm::floor(quadToTex * invAspect * screenToClip * mousePos));
	return texPos;
}

void mouseBtn(int button, int state, int x, int y) {
	cout << "this function is still in developing..." << endl;
}

void mouseMove(int x, int y) {
	
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd' || key == 'D') {
		transX += 0.2f;
		glutPostRedisplay();
	}
	if (key == 'a' || key == 'A') {
		transX -= 0.2f;
		glutPostRedisplay();
	}
	if (key == 'w' || key == 'W') {
		transY += 0.2f;
		glutPostRedisplay();
	}
	if (key == 's' || key == 'S') {
		transY -= 0.2f;
		glutPostRedisplay();
	}
	if (key == 'R' || key == 'r') {
		rotateY += 30;
		if (rotateY > 360) {
			rotateY = 30;
		}
		glutPostRedisplay();
	}
	if (key == 'T' || key == 't') {
		rotateX += 30;
		if (rotateX > 360) {
			rotateX = 30;
		}
		glutPostRedisplay();
	}
}

void idle() {}

void menu(int cmd) {
	switch (cmd) {

	case GLC_PERSPECTIVE:
		glcType = GLC_PERSPECTIVE;
		glutPostRedisplay();
		break;

	case GLC_ORTHOGONAL:
		glcType = GLC_ORTHOGONAL;
		glutPostRedisplay();
		break;

	case GLC_PUSHBROOM:
		glcType = GLC_PUSHBROOM;
		glutPostRedisplay();
		break;

	case OBJ_CUBE:
		objType = OBJ_CUBE;
		glutPostRedisplay();
		break;

	/*case OBJ_TEAPOT:
		objType = OBJ_TEAPOT;
		glutPostRedisplay();
		break;*/

	case OBJ_TEAPOT_LESS:
		objType = OBJ_TEAPOT_LESS;
		glutPostRedisplay();
		break;

	case OBJ_3DTRIANGLE:
		objType = OBJ_3DTRIANGLE;
		glutPostRedisplay();
		break;

	case MENU_CHANGE_BG_COLOR:
		bgColor = randColor();
		glutPostRedisplay();
		break;

	case MENU_EXIT:
		glutLeaveMainLoop();
		break;
	}
}

void cleanup() {
	// Release all resources
	if (texture) { glDeleteTextures(1, &texture); texture = 0; }
	if (shader) { glDeleteProgram(shader); shader = 0; }
	uniXform = 0;
	if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
	if (vbuf) { glDeleteBuffers(1, &vbuf); vbuf = 0; }
	if (ibuf) { glDeleteBuffers(1, &ibuf); ibuf = 0; }
	if (mesh) { delete mesh;  mesh = NULL; }
	vcount = 0;
}
