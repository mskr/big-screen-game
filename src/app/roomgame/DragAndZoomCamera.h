#ifndef DRAG_AND_ZOOM_CAMERA_H
#define DRAG_AND_ZOOM_CAMERA_H

#include <sgct/Engine.h>

class DragAndZoomCamera {
	glm::mat4 view_projection_;
	bool isTouched_;
	glm::vec2 last_position_;
	float drag_speed_;
	float zoom_speed_;
public:
	DragAndZoomCamera(glm::mat4 view_projection = glm::mat4(1));
	void onTouch();
	void onRelease();
	void onMouseMove(float newx, float newy);
	void onScroll(float offset);
	glm::mat4 getViewProjection() { return view_projection_; }
};

#endif