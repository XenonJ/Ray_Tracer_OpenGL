#include <string>
#include <iostream>
#include <fstream>
#include <math.h>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/names.h>

#include "MyGLCanvas.h"

using namespace std;

class MyAppWindow;
MyAppWindow* win;

class MyAppWindow : public Fl_Window {
public:
	// shader
	Fl_Button* reloadButton;

	// files
	Fl_Button* openSceneFileButton;
	Fl_Button* openPlyFileButton;
	Fl_Button* openPlaneButton;

	// shape
	Fl_Slider* segmentsXSlider;
	Fl_Slider* segmentsYSlider;

	// rotate
	Fl_Slider* rotUSlider;
	Fl_Slider* rotVSlider;
	Fl_Slider* rotWSlider;

	// camera translate
	Fl_Button* upButton;
	Fl_Button* downButton;
	Fl_Button* leftButton;
	Fl_Button* rightButton;
	Fl_Button* forwardButton;
	Fl_Button* backButton;

	// mesh translate
	Fl_Button* meshUpButton;
	Fl_Button* meshDownButton;
	Fl_Button* meshLeftButton;
	Fl_Button* meshRightButton;
	Fl_Button* meshForwardButton;
	Fl_Button* meshBackButton;

	MyGLCanvas* canvas;

private:
	// 飞行控制相关变量
	bool flightMode;
	float planeSpeed;
	float planeThrottle;
	float planeRoll;
	float planePitch;
	float planeYaw;
	float targetRoll;
	float targetPitch;
	float targetYaw;
	glm::vec3 planePosition;
	glm::vec3 planeDirection;
	glm::vec3 planeUp;
	int lastMouseX, lastMouseY;

public:
	static const float ROLL_SPEED;
	static const float PITCH_SPEED;
	static const float YAW_SPEED;
	static const float LERP_FACTOR;
	static const float MIN_SPEED;
	static const float MAX_SPEED;
	static const float ACCELERATION;

public:
	// APP WINDOW CONSTRUCTOR
	MyAppWindow(int W, int H, const char* L = 0);

	static void idleCB(void* userdata) {
		win->updatePlaneMovement();
		win->canvas->redraw();
	}

    int handle(int event) override {
        if (!flightMode) {
            return Fl_Window::handle(event);
        }

        int key;

        switch (event) {
            case FL_KEYDOWN:
                key = Fl::event_key();
                switch (key) {
                    case 'w':
                    case 'W':
                        planeThrottle = 1.0f;
                        return 1;
                    case 's':
                    case 'S':
                        planeThrottle = -1.0f;
                        return 1;
                    case 'a':
                    case 'A':
                        targetRoll = ROLL_SPEED;
                        return 1;
                    case 'd':
                    case 'D':
                        targetRoll = -ROLL_SPEED;
                        return 1;
                }
                break;

            case FL_KEYUP:
                key = Fl::event_key();
                switch (key) {
                    case 'w':
                    case 'W':
                    case 's':
                    case 'S':
                        planeThrottle = 0.0f;
                        return 1;
                    case 'a':
                    case 'A':
                    case 'd':
                    case 'D':
                        targetRoll = 0.0f;
                        return 1;
                }
                break;

            case FL_PUSH:
                lastMouseX = Fl::event_x();
                lastMouseY = Fl::event_y();
                return 1;

            case FL_DRAG:
                if (flightMode) {
                    int dx = Fl::event_x() - lastMouseX;
                    int dy = Fl::event_y() - lastMouseY;
                    
                    targetPitch += dy * PITCH_SPEED * 0.01f;
                    targetYaw += dx * YAW_SPEED * 0.01f;
                    
                    lastMouseX = Fl::event_x();
                    lastMouseY = Fl::event_y();
                    return 1;
                }
                break;
        }
        return Fl_Window::handle(event);
    }

private:
	void updateGUIValues() {
		segmentsXSlider->value(canvas->segmentsX);
		segmentsYSlider->value(canvas->segmentsY);

		rotUSlider->value(canvas->camera->rotU);
		rotVSlider->value(canvas->camera->rotV);
		rotWSlider->value(canvas->camera->rotW);
	}

	static void floatCB(Fl_Widget* w, void* userdata) {
		float value = ((Fl_Slider*)w)->value();
		*((float*)userdata) = value;
	}

	static void intCB(Fl_Widget* w, void* userdata) {
		int value = ((Fl_Button*)w)->value();
		printf("value: %d\n", value);
		*((int*)userdata) = value;
	}

	static void reloadCB(Fl_Widget* w, void* userdata) {
		win->canvas->reloadShaders();
	}

	static void loadSceneFileCB(Fl_Widget*w, void*data) {
		Fl_File_Chooser G_chooser("", "", Fl_File_Chooser::MULTI, "");
		G_chooser.show();
		G_chooser.directory("./data");
		while (G_chooser.shown()) {
			Fl::wait();
		}

		// Print the results
		if (G_chooser.value() == NULL) {
			printf("User cancelled file chooser\n");
			return;
		}

		cout << "Loading new scene file from: " << G_chooser.value() << endl;
		win->canvas->loadSceneFile(G_chooser.value());
		win->updateGUIValues();
		win->canvas->redraw();
	}

	static void loadPLYFileCB(Fl_Widget* w, void* data) {
		Fl_File_Chooser G_chooser("", "", Fl_File_Chooser::MULTI, "");
		G_chooser.show();
		G_chooser.directory("./data/ply");
		while (G_chooser.shown()) {
			Fl::wait();
		}

		// Print the results
		if (G_chooser.value() == NULL) {
			printf("User cancelled file chooser\n");
			return;
		}

		cout << "Loading new PLY file from: " << G_chooser.value() << endl;
		win->canvas->loadPLY(G_chooser.value());
		win->canvas->redraw();
	}

	static void loadPlaneCB(Fl_Widget* w, void* data) {
		win->canvas->loadPlane();
		win->flightMode = true;
		win->canvas->redraw();
	}

	static void cameraUPCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(0.0f, 0.5f, 0.0f));
	}

	static void cameraDOWNCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(0.0f, -0.5f, 0.0f));
	}

	static void cameraLEFTCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(-0.5f, 0.0f, 0.0f));
	}

	static void cameraRIGHTCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(0.5f, 0.0f, 0.0f));
	}

	static void cameraFORWARDCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(0.0f, 0.0f, 0.5f));
	}

	static void cameraBACKCB(Fl_Widget* w, void* data) {
		win->canvas->camera->translate(glm::vec3(0.0f, 0.0f, -0.5f));
	}

	static void meshUPCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(0.0f, 0.5f, 0.0f);
}

static void meshDOWNCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(0.0f, -0.5f, 0.0f);
}

static void meshLEFTCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(-0.5f, 0.0f, 0.0f);
}

static void meshRIGHTCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(0.5f, 0.0f, 0.0f);
}

static void meshFORWARDCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(0.0f, 0.0f, 0.5f);
}

static void meshBACKCB(Fl_Widget* w, void* data) {
    win->canvas->meshTranslate += glm::vec3(0.0f, 0.0f, -0.5f);
}

// 左右旋转(Roll)
static void rollLeftCB(Fl_Widget* w, void* data) {
    win->canvas->targetRoll -= win->canvas->rotationSpeed;
}

static void rollRightCB(Fl_Widget* w, void* data) {
    win->canvas->targetRoll += win->canvas->rotationSpeed;
}

// 上下旋转(Pitch)
static void pitchUpCB(Fl_Widget* w, void* data) {
    win->canvas->targetPitch += win->canvas->rotationSpeed;
}

static void pitchDownCB(Fl_Widget* w, void* data) {
    win->canvas->targetPitch -= win->canvas->rotationSpeed;
}

// 左右转向(Yaw)
static void yawLeftCB(Fl_Widget* w, void* data) {
    win->canvas->targetYaw -= win->canvas->rotationSpeed;
}

static void yawRightCB(Fl_Widget* w, void* data) {
    win->canvas->targetYaw += win->canvas->rotationSpeed;
}

	static void segmentsCB(Fl_Widget* w, void* userdata) {
		int value = ((Fl_Slider*)w)->value();
		printf("value: %d\n", value);
		*((int*)userdata) = value;
		win->canvas->setSegments();
	}

	static void cameraRotateCB(Fl_Widget* w, void* userdata) {
		win->canvas->camera->setRotUVW(win->rotUSlider->value(), win->rotVSlider->value(), win->rotWSlider->value());
	}

	void updatePlaneMovement() {
		if (!flightMode) return;

		// 更新飞机姿态
		planeRoll = glm::mix(planeRoll, targetRoll, LERP_FACTOR);
		planePitch = glm::mix(planePitch, targetPitch, LERP_FACTOR);
		planeYaw = glm::mix(planeYaw, targetYaw, LERP_FACTOR);

		// 更新速度
		planeSpeed = glm::clamp(planeSpeed + planeThrottle * ACCELERATION, MIN_SPEED, MAX_SPEED);

		// 更新方向向量
		glm::mat4 rotation = glm::mat4(1.0f);
		rotation = glm::rotate(rotation, planeYaw, glm::vec3(0.0f, 1.0f, 0.0f));
		rotation = glm::rotate(rotation, planePitch, glm::vec3(1.0f, 0.0f, 0.0f));
		rotation = glm::rotate(rotation, planeRoll, glm::vec3(0.0f, 0.0f, 1.0f));

		planeDirection = glm::vec3(rotation * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
		planeUp = glm::vec3(rotation * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f));

		// 更新位置
		planePosition += planeDirection * planeSpeed * 0.016f;

		// 更新相机位置（跟随飞机）
		glm::vec3 cameraOffset = -planeDirection * 10.0f + planeUp * 3.0f;
		glm::vec3 cameraPosition = planePosition + cameraOffset;
		
		// 更新相机视角
		canvas->camera->orientLookAt(
			cameraPosition,     // 相机位置
			planePosition,      // 看向飞机的位置
			planeUp            // 使用飞机的上向量作为相机的上向量
		);
	}

};

const float MyAppWindow::ROLL_SPEED = 0.5f;
const float MyAppWindow::PITCH_SPEED = 0.5f;
const float MyAppWindow::YAW_SPEED = 0.5f;
const float MyAppWindow::LERP_FACTOR = 0.1f;
const float MyAppWindow::MIN_SPEED = 0.5f;
const float MyAppWindow::MAX_SPEED = 3.0f;
const float MyAppWindow::ACCELERATION = 0.3f;

MyAppWindow::MyAppWindow(int W, int H, const char* L) : Fl_Window(W, H, L) {
	flightMode = false;
	planeSpeed = MIN_SPEED;
	planeThrottle = 0.0f;
	planeRoll = 0.0f;
	planePitch = 0.0f;
	planeYaw = 0.0f;
	targetRoll = 0.0f;
	targetPitch = 0.0f;
	targetYaw = 0.0f;
	planePosition = glm::vec3(0.0f, 10.0f, -20.0f);
	planeDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	planeUp = glm::vec3(0.0f, 1.0f, 0.0f);
	lastMouseX = 0;
	lastMouseY = 0;

	begin();

	canvas = new MyGLCanvas(10, 10, w() - 320, h() - 20);

	Fl_Pack* packCol1 = new Fl_Pack(w() - 310, 30, 150, h(), "");
	packCol1->box(FL_DOWN_FRAME);
	packCol1->type(Fl_Pack::VERTICAL);
	packCol1->spacing(30);
	packCol1->begin();

		Fl_Pack* packShaders = new Fl_Pack(w() - 100, 30, 100, h(), "Shader");
		packShaders->box(FL_DOWN_FRAME);
		packShaders->labelfont(1);
		packShaders->type(Fl_Pack::VERTICAL);
		packShaders->spacing(0);
		packShaders->begin();

			reloadButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "Reload");
			reloadButton->callback(reloadCB, (void*)this);

		packShaders->end();

		Fl_Pack* loadPack = new Fl_Pack(w() - 100, 30, 100, h(), "Scene Files");
		loadPack->box(FL_DOWN_FRAME);
		loadPack->labelfont(1);
		loadPack->type(Fl_Pack::VERTICAL);
		loadPack->spacing(0);
		loadPack->begin();

			openSceneFileButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "Load File");
			openSceneFileButton->callback(loadSceneFileCB, (void*)this);

			openPlyFileButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "Load PLY File");
			openPlyFileButton->callback(loadPLYFileCB, (void*)this);

			openPlaneButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "Load Plane");
			openPlaneButton->callback(loadPlaneCB, (void*)this);

		loadPack->end();

		Fl_Pack* radioPack = new Fl_Pack(w() - 100, 30, 100, h(), "Shape");
		radioPack->box(FL_DOWN_FRAME);
		radioPack->labelfont(1);
		radioPack->type(Fl_Pack::VERTICAL);
		radioPack->spacing(0);
		radioPack->begin();
			//slider for controlling number of segments in X
			Fl_Box *segmentsXTextbox = new Fl_Box(0, 0, packCol1->w() - 20, 20, "SegmentsX");
			segmentsXSlider = new Fl_Value_Slider(0, 0, packCol1->w() - 20, 20, "");
			segmentsXSlider->align(FL_ALIGN_TOP);
			segmentsXSlider->type(FL_HOR_SLIDER);
			segmentsXSlider->bounds(3, 60);
			segmentsXSlider->step(1);
			segmentsXSlider->value(canvas->segmentsX);
			segmentsXSlider->callback(segmentsCB, (void*)(&(canvas->segmentsX)));


			//slider for controlling number of segments in Y
			Fl_Box *segmentsYTextbox = new Fl_Box(0, 0, packCol1->w() - 20, 20, "SegmentsY");
			segmentsYSlider = new Fl_Value_Slider(0, 0, packCol1->w() - 20, 20, "");
			segmentsYSlider->align(FL_ALIGN_TOP);
			segmentsYSlider->type(FL_HOR_SLIDER);
			segmentsYSlider->bounds(3, 60);
			segmentsYSlider->step(1);
			segmentsYSlider->value(canvas->segmentsY);
			segmentsYSlider->callback(segmentsCB, (void*)(&(canvas->segmentsY)));

		radioPack->end();

		Fl_Pack* rotPack = new Fl_Pack(w() - 100, 30, 100, h(), "Camera Rotate");
		rotPack->box(FL_DOWN_FRAME);
		rotPack->labelfont(1);
		rotPack->type(Fl_Pack::VERTICAL);
		rotPack->spacing(0);
		rotPack->begin();

			Fl_Box *rotUTextBox = new Fl_Box(0, 0, packCol1->w() - 20, 20, "RotateU");
			rotUSlider = new Fl_Value_Slider(0, 0, packCol1->w() - 20, 20, "");
			rotUSlider->align(FL_ALIGN_TOP);
			rotUSlider->type(FL_HOR_SLIDER);
			rotUSlider->bounds(-179, 179);
			rotUSlider->step(1);
			rotUSlider->value(canvas->camera->rotU);
			rotUSlider->callback(cameraRotateCB);

			Fl_Box *rotVTextBox = new Fl_Box(0, 0, packCol1->w() - 20, 20, "RotateV");
			rotVSlider = new Fl_Value_Slider(0, 0, packCol1->w() - 20, 20, "");
			rotVSlider->align(FL_ALIGN_TOP);
			rotVSlider->type(FL_HOR_SLIDER);
			rotVSlider->bounds(-179, 179);
			rotVSlider->step(1);
			rotVSlider->value(canvas->camera->rotV);
			rotVSlider->callback(cameraRotateCB);

			Fl_Box *rotWTextBox = new Fl_Box(0, 0, packCol1->w() - 20, 20, "RotateW");
			rotWSlider = new Fl_Value_Slider(0, 0, packCol1->w() - 20, 20, "");
			rotWSlider->align(FL_ALIGN_TOP);
			rotWSlider->type(FL_HOR_SLIDER);
			rotWSlider->bounds(-179, 179);
			rotWSlider->step(1);
			rotWSlider->value(canvas->camera->rotW);
			rotWSlider->callback(cameraRotateCB);

		rotPack->end();


	packCol1->end();


	Fl_Pack* packCol2 = new Fl_Pack(w() - 155, 30, 150, h(), "");
	packCol2->box(FL_DOWN_FRAME);
	packCol2->type(Fl_Pack::VERTICAL);
	packCol2->spacing(30);
	packCol2->begin();

		Fl_Pack* transPack = new Fl_Pack(w() - 100, 30, 100, h(), "Camera Translate");
		transPack->box(FL_DOWN_FRAME);
		transPack->labelfont(1);
		transPack->type(Fl_Pack::VERTICAL);
		transPack->spacing(0);
		transPack->begin();

			upButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "UP");
			upButton->callback(cameraUPCB, (void*)this);

			downButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "DOWN");
			downButton->callback(cameraDOWNCB, (void*)this);

			leftButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "LEFT");
			leftButton->callback(cameraLEFTCB, (void*)this);

			rightButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "RIGHT");
			rightButton->callback(cameraRIGHTCB, (void*)this);

			forwardButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "FORWARD");
			forwardButton->callback(cameraFORWARDCB, (void*)this);

			backButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "BACK");
			backButton->callback(cameraBACKCB, (void*)this);

		transPack->end();

		Fl_Pack* meshTransPack = new Fl_Pack(w() - 100, 30, 100, h(), "Mesh Translate");
		meshTransPack->box(FL_DOWN_FRAME);
		meshTransPack->labelfont(1);
		meshTransPack->type(Fl_Pack::VERTICAL);
		meshTransPack->spacing(0);
		meshTransPack->begin();

			meshUpButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "UP");
			meshUpButton->callback(meshUPCB, (void*)this);

			meshDownButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "DOWN");
			meshDownButton->callback(meshDOWNCB, (void*)this);

			meshLeftButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "LEFT");
			meshLeftButton->callback(meshLEFTCB, (void*)this);

			meshRightButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "RIGHT");
			meshRightButton->callback(meshRIGHTCB, (void*)this);

			meshForwardButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "FORWARD");
			meshForwardButton->callback(meshFORWARDCB, (void*)this);

			meshBackButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "BACK");
			meshBackButton->callback(meshBACKCB, (void*)this);

		meshTransPack->end();

	packCol2->end();

	end();
}

int main(int argc, char** argv) {
	win = new MyAppWindow(850, 475, "Environment Mapping");
	win->resizable(win);
	Fl::add_idle(MyAppWindow::idleCB);
	win->show();
	return(Fl::run());
}
