/**
 * @file   MasterNode.cpp
 * @author Sebastian Maisch <sebastian.maisch@uni-ulm.de>
 * @date   2016.11.25
 *
 * @brief  Implementation of the master application node.
 */

#include "MasterNode.h"
#include <imgui.h>
#include "core/imgui/imgui_impl_glfw_gl3.h"
#include <iostream>

namespace viscom {

    MasterNode::MasterNode(ApplicationNodeInternal* appNode) :
        ApplicationNodeImplementation{ appNode },
        grid_(GRID_COLS_, GRID_ROWS_, GRID_HEIGHT_, &meshpool_),
        cellular_automaton_(&grid_, 0.2),
        interaction_mode_(InteractionMode::GRID)
    {
        grid_state_ = {};
        automaton_transition_time_delta_ = 0.0;
        automaton_has_transitioned_ = false;
    }

    MasterNode::~MasterNode() = default;


    void MasterNode::InitOpenGL() {
        ApplicationNodeImplementation::InitOpenGL();
        
        grid_.loadShader(GetApplication()->GetGPUProgramManager()); // for viewing build states...
        grid_.uploadVertexData(); // ...for debug purposes

        cellular_automaton_.init(GetApplication()->GetGPUProgramManager());

        outerInfluence_->Grid = &grid_;
        glm::vec3 gridPos = grid_.grid_center_;
        glm::vec2 pos = GetCirclePos(glm::vec2(gridPos.y, gridPos.z), range, viewAngle);
        GetCamera()->SetPosition(glm::vec3(0, pos.x, pos.y));
        glm::quat lookDir = glm::toQuat(glm::lookAt(GetCamera()->GetPosition(), gridPos, glm::vec3(0, 1, 0)));
        GetCamera()->SetOrientation(lookDir);

    }

    /* Sync step 1: Master sets values of shared objects to the values of corresponding non-shared objects */
    void MasterNode::PreSync() {
        ApplicationNodeImplementation::PreSync();
        outerInfluence_->MeshComponent->preSync();
        meshpool_.preSync();
        synchronized_grid_translation_.setVal(grid_.getTranslation());
        synchronized_automaton_transition_time_delta_.setVal(cellular_automaton_.getTimeDeltaNormalized());
        synchronized_automaton_has_transitioned_.setVal(automaton_has_transitioned_);
        // Grid state sync only when automaton changed it
        if (automaton_has_transitioned_) {
            synchronized_grid_state_.setVal(std::vector<roomgame::GRID_STATE_ELEMENT>(
                cellular_automaton_.getGridBuffer(),
                cellular_automaton_.getGridBuffer() + cellular_automaton_.getGridBufferElements()));
        }
    }

    /* Sync step 2: Master sends shared objects to the central SharedData singleton
     * (Order in decoding on slaves must be the same as in encoding)
    */
    void MasterNode::EncodeData() {
        ApplicationNodeImplementation::EncodeData();
        outerInfluence_->MeshComponent->encode();
        meshpool_.encode();
        sgct::SharedData::instance()->writeObj<glm::vec3>(&synchronized_grid_translation_);
        sgct::SharedData::instance()->writeFloat(&synchronized_automaton_transition_time_delta_);
        sgct::SharedData::instance()->writeBool(&synchronized_automaton_has_transitioned_);
        if (automaton_has_transitioned_)
            sgct::SharedData::instance()->writeVector(&synchronized_grid_state_);
    }

    /* Sync step 3: Master updates its copies of cluster-wide variables with data it just synced
     * (Slaves update their copies with the data they received)
     * These copies are good because in contrast to SGCT shared objects they are not mutex locked on each access
     * Note, that master does not need this update because its data is always up to date...
     * ... but it is currently done to access the same variables with the same code on master and slaves
    */
    void MasterNode::UpdateSyncedInfo() {
        ApplicationNodeImplementation::UpdateSyncedInfo();
        outerInfluence_->MeshComponent->updateSyncedMaster();
        meshpool_.updateSyncedMaster();
        // Of course the following variables are redundant on master 
        // but help to write "unified" code in ApplicationNodeImplementation
        grid_translation_ = synchronized_grid_translation_.getVal();
        automaton_transition_time_delta_ = synchronized_automaton_transition_time_delta_.getVal();
        automaton_has_transitioned_ = synchronized_automaton_has_transitioned_.getVal();
        // GPU data upload behind check if GL was initialized
        if (last_grid_state_texture_.id > 0 && current_grid_state_texture_.id > 0) {
            // ensure that this happens only once after a automaton transition to have last and current state right
            if (automaton_has_transitioned_) {
                uploadGridStateToGPU();
            }
        }
    }

    /* This SGCT stage is called only once before each frame, regardless of the number of viewports */
    void MasterNode::UpdateFrame(double t1, double t2) {
        ApplicationNodeImplementation::UpdateFrame(t1, t2);

#ifdef WITH_TUIO
        if (!inputBuffer.empty()) {
            handleInputBuffer();
        }
#endif

        //cellular_automaton_.setTransitionTime(automaton_transition_time);
        //cellular_automaton_.setDamagePerCell(automaton_damage_per_cell);
        automaton_has_transitioned_ = cellular_automaton_.transition(clock_.t_in_sec);


        updateManager_.ManageUpdates(min(clock_.deltat(), 0.25));
    }

    /* This SGCT stage draws the scene to the current back buffer (left, right or both).
    This stage can be called several times per frame if multiple viewports and/or if stereoscopic rendering is active.*/
    void MasterNode::DrawFrame(FrameBuffer& fbo) {
        ApplicationNodeImplementation::DrawFrame(fbo);

        glm::mat4 viewProj = GetCamera()->GetViewPerspectiveMatrix();
        outerInfluence_->ViewPersMat = viewProj;

        grid_.updateProjection(viewProj);
        fbo.DrawToFBO([&] {
            if (render_mode_ == RenderMode::DBG) grid_.onFrame();
        });

    }

    void MasterNode::Draw2D(FrameBuffer& fbo) {
        int attackChanceGrowth = outerInfluence_->getAttackChanceGrowth();
        float outInfluenceSpeed = outerInfluence_->getBaseSpeed();

        fbo.DrawToFBO([&]() {
            ImGui::SetNextWindowPos(ImVec2(700, 60), ImGuiSetCond_FirstUseEver);
            ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiSetCond_FirstUseEver);
            ImGui::GetIO().FontGlobalScale = 1.5f;
            if (ImGui::Begin("Roomgame Controls")) {
                //ImGui::SetWindowFontScale(2.0f);
                ImGui::Text("Interaction mode: %s", (interaction_mode_ == GRID) ? "GRID" :
                    ((interaction_mode_ == AUTOMATON) ? "AUTOMATON" : "CAMERA"));
            }

            ImGui::Spacing();
            if (ImGui::CollapsingHeader("Settings"))
            {
                ImGui::Spacing();
                ImGui::Text("Outer Influence");
                ImGui::Spacing();
                if (ImGui::SliderInt("Attack chance", &attackChanceGrowth, 1, 10, "%.0f%%")) {
                    outerInfluence_->setAttackChanceGrowth(attackChanceGrowth);
                }

                ImGui::Spacing();
                if (ImGui::SliderFloat("Speed", &outInfluenceSpeed, 0.2f, 1.0f)) {
                    outerInfluence_->setBaseSpeed(outInfluenceSpeed);
                }

                if (ImGui::Button("Reset Values")) {
                    resetPlaygroundValues();
                }
            }

            ImGui::Spacing();
            if (ImGui::Button("Reset Playground")) {
                resetPlaygroundValues();
                reset();
            }

            ImGui::End();
        });

        ApplicationNodeImplementation::Draw2D(fbo);
    }

    void MasterNode::PostDraw() {
        ApplicationNodeImplementation::PostDraw();
    }

    void MasterNode::CleanUp() {
        grid_.cleanup();
        cellular_automaton_.cleanup();
        ApplicationNodeImplementation::CleanUp();
    }

    glm::vec2 MasterNode::GetCirclePos(glm::vec2 center, float radius, int angle) {
        float x = center.x + radius*cos(glm::radians<float>(static_cast<float>(angle)));
        float y = center.y + radius*sin(glm::radians<float>(static_cast<float>(angle)));

        return glm::vec2(x, y);
    }

    /* Switch input modes by keyboard on master
     * [C] key down: camera control mode
     * [S] key hit: switch between interacting with automaton state or building rooms
     * [D] key down: debug render mode
    */
    bool MasterNode::KeyboardCallback(int key, int scancode, int action, int mods) {
        // Keys switch input modes
        static InteractionMode mode_before_switch_to_camera = interaction_mode_;
        if (key == GLFW_KEY_C) {
            if (action == GLFW_PRESS) {
                mode_before_switch_to_camera = interaction_mode_;
                interaction_mode_ = InteractionMode::CAMERA;
            }
            else if (action == GLFW_RELEASE) {
                interaction_mode_ = (InteractionMode)mode_before_switch_to_camera;
            }
        }
        else if (interaction_mode_ == InteractionMode::CAMERA) {
            glm::vec3 gridPos = grid_.grid_center_;

            int right = key == GLFW_KEY_D ? 1 : 0;
            int left = key == GLFW_KEY_A ? 1 : 0;
            int up = key == GLFW_KEY_W ? 1 : 0;
            int down = key == GLFW_KEY_S ? 1 : 0;
            int xAxis = right - left;
            int yAxis = up - down;
            GetCamera()->SetPosition(GetCamera()->GetPosition() + glm::vec3(xAxis, yAxis, 0)*0.1f);
            int upWards = key == GLFW_KEY_UP ? 1 : 0;
            int downWards = key == GLFW_KEY_DOWN ? 1 : 0;
            int rotate = upWards - downWards;
            if (rotate != 0) {
                viewAngle += 5 * rotate;
                viewAngle = glm::clamp(viewAngle, 90, 170);
                glm::vec2 pos = GetCirclePos(glm::vec2(gridPos.y, gridPos.z), range, viewAngle);
                GetCamera()->SetPosition(glm::vec3(0, pos.x, pos.y));
                std::cout << range << "/" << viewAngle << std::endl;
            }
            glm::quat lookDir = glm::toQuat(glm::lookAt(GetCamera()->GetPosition(), gridPos, glm::vec3(0, 1, 0)));
            GetCamera()->SetOrientation(lookDir);
        }
        else if (key == GLFW_KEY_D) {
            if (action == GLFW_PRESS) {
                render_mode_ = RenderMode::DBG;
            }
            else if (action == GLFW_RELEASE) {
                render_mode_ = RenderMode::NORMAL;
            }
        }
        else if (key == GLFW_KEY_S && action == GLFW_PRESS) {
            if (interaction_mode_ == AUTOMATON) interaction_mode_ = InteractionMode::GRID;
            else interaction_mode_ = InteractionMode::AUTOMATON;
        }
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_KeyCallback(key, scancode, action, mods);
#endif
        return ApplicationNodeImplementation::KeyboardCallback(key, scancode, action, mods);
    }

    /* Mouse/touch control room placement
    * click to place rooms
    */
    bool MasterNode::MouseButtonCallback(int button, int action) {
        if (button == GLFW_MOUSE_BUTTON_LEFT) {
            if (interaction_mode_ == InteractionMode::GRID) {
                if (action == GLFW_PRESS) grid_.onTouch(-1);
                else if (action == GLFW_RELEASE) grid_.onRelease(-1);
            }
            else if (interaction_mode_ == InteractionMode::AUTOMATON) {
                if (action == GLFW_PRESS) grid_.populateCircleAtLastMousePosition(1);
            }
            else if (interaction_mode_ == InteractionMode::CAMERA) {
//                camera_.HandleMouse(button, action, 0, this);
            }
        }
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MouseButtonCallback(button, action, 0);
#endif
        return ApplicationNodeImplementation::MouseButtonCallback(button, action);
    }

    /* MousePosCallback constantly updates interaction targets with cursor position
     * Workaround for missing cursor position in MouseButtonCallback:
     * Interaction targets can use their last cursor position
    */
    bool MasterNode::MousePosCallback(double x, double y) {
        viscom::math::Line3<float> ray = GetCamera()->GetPickRay({ x,y });
        grid_.onMouseMove(-1, ray[0], ray[1]);
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_MousePositionCallback(x, y);
#endif
        return ApplicationNodeImplementation::MousePosCallback(x, y);
    }

    /* Mouse scroll events are used to zoom, when in camera mode */
    bool MasterNode::MouseScrollCallback(double xoffset, double yoffset) {
        if (interaction_mode_ == InteractionMode::CAMERA) {
            float change = (float)yoffset*0.1f;
            glm::vec3 camToGrid = GetCamera()->GetPosition() - grid_.grid_center_;
            if (glm::length(camToGrid) > 0.5 || yoffset > 0) {
                GetCamera()->SetPosition(GetCamera()->GetPosition() + camToGrid*change);
                range = glm::distance(GetCamera()->GetPosition(), grid_.grid_center_);
            }
//            camera_.HandleMouse(0, 0, (float)yoffset, this);
        }
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_ScrollCallback(xoffset, yoffset);
#endif
        return ApplicationNodeImplementation::MouseScrollCallback(xoffset, yoffset);
    }

    bool MasterNode::CharCallback(unsigned int character, int mods) {
#ifndef VISCOM_CLIENTGUI
        ImGui_ImplGlfwGL3_CharCallback(character);
#endif
        return ApplicationNodeImplementation::CharCallback(character, mods);
    }

    bool MasterNode::AddTuioCursor(TUIO::TuioCursor* tcur)
    {
        // grid_.onTouch(-1); //TODO @Tobias: pass the CursorID here,
        // ID is saved in interaction object by RoomInteractiveGrid::handleTouchedCell,
            // this object lives for the duration of one touch gesture
            // and is used to distinguish clicks/touches, to reuse IDs of accidentally interrupted touch gestures, etc.
            // (see RoomInteractiveGrid::handleHoveredCell + handleRelease + GridInteraction::testTemporalAndSpatialProximity)
#ifdef WITH_TUIO
        mtx.lock();
        inputBuffer.push_back(input{ INPUT_ADD,tcur->getX(),tcur->getY(), tcur->getCursorID() });
        mtx.unlock();
#endif
        //std::cout << "add    cur  " << tcur->getCursorID() << " (" << tcur->getSessionID() << "/" << tcur->getTuioSourceID() << ") " << tcur->getX() << " " << tcur->getY() << std::endl;
        return true;
    }

    bool MasterNode::UpdateTuioCursor(TUIO::TuioCursor* tcur)
    {
#ifdef WITH_TUIO
        mtx.lock();
        inputBuffer.push_back(input{ INPUT_UPDATE,tcur->getX(),tcur->getY(), tcur->getCursorID() });
        mtx.unlock();
#endif
        //std::cout << "set    cur  " << tcur->getCursorID() << " (" << tcur->getSessionID() << "/" << tcur->getTuioSourceID() << ") " << tcur->getX() << " " << tcur->getY()
        //    << " " << tcur->getMotionSpeed() << " " << tcur->getMotionAccel() << " " << std::endl;
        return true;
    }

    bool MasterNode::RemoveTuioCursor(TUIO::TuioCursor* tcur)
    {
#ifdef WITH_TUIO
        mtx.lock();
        inputBuffer.push_back(input{ INPUT_REMOVE,tcur->getX(),tcur->getY(), tcur->getCursorID() });
        mtx.unlock();
#endif
        //std::cout << "delete cur  " << tcur->getCursorID() << " (" << tcur->getSessionID() << "/" << tcur->getTuioSourceID() << ")" << std::endl;
        return true;
    }

    bool MasterNode::handleInputBuffer() {
#ifdef WITH_TUIO
        if (mtx.try_lock()) {
            //std::cout << "DEBUG: working on input with size " << inputBuffer.size() << std::endl;
            for (int i = 0; i < inputBuffer.size(); i++) {
                input tmp = inputBuffer.at(i);
                viscom::math::Line3<float> ray = GetCamera()->GetPickRay({ tmp.x,tmp.y });
                if (tmp.type == INPUT_UPDATE) {
                    grid_.onMouseMove(tmp.id, ray[0], ray[1]);
                }
                else if (tmp.type == INPUT_ADD) {
                    grid_.onMouseMove(tmp.id, ray[0], ray[1]);
                    grid_.onTouch(tmp.id);
                }
                else if (tmp.type == INPUT_REMOVE) {
                    grid_.onMouseMove(tmp.id, ray[0], ray[1]);
                    grid_.onRelease(tmp.id);
                }
            }
            inputBuffer.clear();
            mtx.unlock();
            return true;
        }
        else {
            std::cout << "could not lock" << std::endl;
            return false;
        }
#endif
        return true;
    }

    void MasterNode::reset() {
        grid_.forEachCell([&](GridCell *cell) {
            grid_.buildAt(cell->getCol(), cell->getRow(), GridCell::EMPTY, InteractiveGrid::BuildMode::Replace);
            grid_.updateHealthPoints(cell, GridCell::MAX_HEALTH);
            grid_.reset();
        });
        outerInfluence_->MeshComponent->sourcePositions_.clear();
    }

    void MasterNode::resetPlaygroundValues()
    {
        outerInfluence_->resetValues();
    }
}