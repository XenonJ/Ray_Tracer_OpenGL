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

	// shape
	Fl_Slider* segmentsXSlider;
	Fl_Slider* segmentsYSlider;

	// rotate
	Fl_Slider* rotUSlider;
	Fl_Slider* rotVSlider;
	Fl_Slider* rotWSlider;

	MyGLCanvas* canvas;

public:
	// APP WINDOW CONSTRUCTOR
	MyAppWindow(int W, int H, const char* L = 0);

	static void idleCB(void* userdata) {
		win->canvas->redraw();
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

	static void segmentsCB(Fl_Widget* w, void* userdata) {
		int value = ((Fl_Slider*)w)->value();
		printf("value: %d\n", value);
		*((int*)userdata) = value;
		win->canvas->setSegments();
	}

	static void cameraRotateCB(Fl_Widget* w, void* userdata) {
		win->canvas->camera->setRotUVW(win->rotUSlider->value(), win->rotVSlider->value(), win->rotWSlider->value());
	}

};


MyAppWindow::MyAppWindow(int W, int H, const char* L) : Fl_Window(W, H, L) {
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

			openPlyFileButton = new Fl_Button(0, 0, packCol1->w() - 20, 20, "Load PLY File?");
			// openPlyFileButton->callback(loadSceneFileCB, (void*)this);

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
