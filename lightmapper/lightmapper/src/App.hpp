/*
Responsible for file IO and ImGui menuing.
Also contains the main window handling clear/draw and input events
which includes updating the view matrix and passing in the appropriate
projectin matrices depending on the mode
*/

#pragma once

#include "scene.h"
#include "Callbacks.hpp"

#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

class App final
{
public:
    App();
    ~App();

    App(const App&) = delete;
    App(App&&) = delete;
    App& operator = (const App&) = delete;
    App& operator = (App&&) = delete;

    void run();

private:
    Scene m_scene;
    GLFWwindow* m_window;
    bool m_initOK;
    bool m_mapLoaded;
    glm::mat4 m_viewMatrix;
    std::array<float, 3> m_clearColour = { 0.6f, 0.8f, 1.0f };

    glm::vec3 m_cameraPosition;
    glm::vec2 m_cameraRotation;

    glm::vec3 m_orthoPosition;
    glm::vec3 m_mousePosition;

    std::vector<RoomData> m_mapData;
    std::int32_t m_currentRoom = -1;
    void loadMapData(const std::string&);
    void loadModel(const std::string&);
    void importObjFile(const std::string&);

    struct ImportTransform final
    {
        glm::vec3 scale = glm::vec3(1.f);
        glm::vec3 rotation = glm::vec3(0.f);
    }m_importTransform;

    std::string m_outputPath;

    void handleEvents();
    void update();

    glm::vec3 getMousePosition();
    void calcViewMatrix();
    void draw();

    void bakeModel();
    void bakeAll();
    bool m_bakeAll;
    bool m_saveOutput;
    bool m_showImportWindow;

    struct LastPaths final
    {
        std::string lastImport;
        std::string lastExport;
        std::string lastTexture;
        std::string lastMap;
        std::string lastModel;
        std::string measurePath = "assets/measure_texture.png";
    }m_lastPaths;

    void updateSceneGeometry(const RoomData&);
    void exportModel(const std::string&, bool, bool, bool);

    void setZUp(bool);

    //ui stuffs
    void mapBrowserWindow();
    void statusWindow();
    void debugWindow();
    void hitboxWindow();
    bool m_smoothTextures;
    bool m_hitboxMode;

    GLFWcursor* m_pointerCursor = nullptr;
    GLFWcursor* m_crossCursor = nullptr;
    RectMesh* m_selectedRect = nullptr;
    void selectRect(RectMesh*);
    void grabHandle();

    enum class MouseState
    {
        HandleStart, HandleEnd, None
    }m_mouseState = MouseState::None;

    friend void mouse_move_callback(GLFWwindow*, double, double);
    friend void mouse_button_callback(GLFWwindow*, int, int, int);
    friend void keypress_callback(GLFWwindow*, int, int, int, int);
};
