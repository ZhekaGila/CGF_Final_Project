#include "geometry/StageGeometry.h"
#include "geometry/Primitives.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace cgf {

std::vector<ScenePart> createStageGeometry()
{
    std::vector<ScenePart> parts;

    constexpr float PI = 3.14159265359f;

    // ------------------------------------------------------------
    // Helper functions
    // ------------------------------------------------------------

    // Create an axis-aligned box and place it in the scene.
    auto addBox = [&](const std::string& name,
                      const glm::vec3& size,
                      const glm::vec3& color,
                      const glm::vec3& position)
    {
        ScenePart part;
        part.name = name;
        part.mesh = makeBox(size, color);
        part.transform = glm::translate(glm::mat4(1.0f), position);
        parts.push_back(part);
    };

    // Create a box rotated around the Z axis. This is useful for
    // guitar necks, strings, microphone booms, and character limbs.
    auto addRotatedBox = [&](const std::string& name,
                             const glm::vec3& size,
                             const glm::vec3& color,
                             const glm::vec3& position,
                             float rotationZDegrees)
    {
        ScenePart part;
        part.name = name;
        part.mesh = makeBox(size, color);
        part.transform =
            glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f),
                        glm::radians(rotationZDegrees),
                        glm::vec3(0.0f, 0.0f, 1.0f));
        parts.push_back(part);
    };

    // makeCylinder() creates a cylinder of unit height along the Y axis.
    auto addCylinder = [&](const std::string& name,
                           float radius,
                           float height,
                           const glm::vec3& color,
                           const glm::vec3& position)
    {
        ScenePart part;
        part.name = name;
        part.mesh = makeCylinder(radius, color);
        part.transform =
            glm::translate(glm::mat4(1.0f), position) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, height, 1.0f));
        parts.push_back(part);
    };

    // Rotate a cylinder so its axis points along Z instead of Y.
    // This is used for horizontally mounted drums.
    auto addCylinderAlongZ = [&](const std::string& name,
                                 float radius,
                                 float length,
                                 const glm::vec3& color,
                                 const glm::vec3& position)
    {
        ScenePart part;
        part.name = name;
        part.mesh = makeCylinder(radius, color);
        part.transform =
            glm::translate(glm::mat4(1.0f), position) *
            glm::rotate(glm::mat4(1.0f), glm::radians(90.0f),
                        glm::vec3(1.0f, 0.0f, 0.0f)) *
            glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, length, 1.0f));
        parts.push_back(part);
    };

    auto addSphere = [&](const std::string& name,
                         const glm::vec3& color,
                         const glm::vec3& position,
                         const glm::vec3& scale)
    {
        ScenePart part;
        part.name = name;
        part.mesh = makeSphere(color);
        part.transform =
            glm::translate(glm::mat4(1.0f), position) *
            glm::scale(glm::mat4(1.0f), scale);
        parts.push_back(part);
    };

    // Build a rectangular limb between two points in the XY plane.
    // The Z coordinate is interpolated by placing the segment halfway
    // between the two supplied points.
    auto addLimb = [&](const std::string& name,
                       const glm::vec3& start,
                       const glm::vec3& end,
                       float width,
                       float depth,
                       const glm::vec3& color)
    {
        const float dx = end.x - start.x;
        const float dy = end.y - start.y;
        const float length = std::sqrt(dx * dx + dy * dy);

        if (length < 0.001f)
            return;

        const glm::vec3 center = (start + end) * 0.5f;
        const float angleDegrees =
            std::atan2(-dx, dy) * 180.0f / PI;

        addRotatedBox(
            name,
            glm::vec3(width, length, depth),
            color,
            center,
            angleDegrees
        );
    };
        // Create a fully 3D cylindrical limb between two points.
    auto addLimb3D = [&](const std::string& name,
                         const glm::vec3& start,
                         const glm::vec3& end,
                         float radius,
                         const glm::vec3& color)
    {
        const glm::vec3 delta = end - start;
        const float length = glm::length(delta);

        if (length < 0.001f)
            return;

        const glm::vec3 direction = delta / length;
        const glm::vec3 defaultAxis(0.0f, 1.0f, 0.0f);

        float cosine = glm::dot(defaultAxis, direction);

        if (cosine > 1.0f) cosine = 1.0f;
        if (cosine < -1.0f) cosine = -1.0f;

        glm::vec3 rotationAxis =
            glm::cross(defaultAxis, direction);

        const float axisLength = glm::length(rotationAxis);
        const float angle = std::acos(cosine);

        glm::mat4 rotation(1.0f);

        if (axisLength > 0.0001f)
        {
            rotation = glm::rotate(
                glm::mat4(1.0f),
                angle,
                rotationAxis / axisLength
            );
        }
        else if (cosine < 0.0f)
        {
            rotation = glm::rotate(
                glm::mat4(1.0f),
                PI,
                glm::vec3(1.0f, 0.0f, 0.0f)
            );
        }

        ScenePart part;
        part.name = name;
        part.mesh = makeCylinder(radius, color);

        part.transform =
            glm::translate(glm::mat4(1.0f),
                           (start + end) * 0.5f) *
            rotation *
            glm::scale(glm::mat4(1.0f),
                       glm::vec3(1.0f, length, 1.0f));

        parts.push_back(part);
    };

    // Add a simple face looking towards the camera (+Z direction).
    auto addFaceDetails = [&](const std::string& prefix,
                              float x,
                              float y,
                              float z,
                              const glm::vec3& hairColor)
    {
        const glm::vec3 eyeColor(0.025f, 0.02f, 0.025f);
        const glm::vec3 mouthColor(0.12f, 0.035f, 0.035f);

        addSphere(prefix + " Hair", hairColor,
                  glm::vec3(x, y + 0.245f, z - 0.005f),
                  glm::vec3(0.29f, 0.14f, 0.25f));

        addRotatedBox(prefix + " Hair Fringe",
                      glm::vec3(0.34f, 0.075f, 0.055f),
                      hairColor,
                      glm::vec3(x, y + 0.155f, z + 0.225f),
                      0.0f);

        // Eyes
        addSphere(prefix + " Left Eye", eyeColor,
                  glm::vec3(x - 0.09f, y + 0.025f, z + 0.252f),
                  glm::vec3(0.035f, 0.042f, 0.022f));

        addSphere(prefix + " Right Eye", eyeColor,
                  glm::vec3(x + 0.09f, y + 0.025f, z + 0.252f),
                  glm::vec3(0.035f, 0.042f, 0.022f));

        // Nose and mouth
        addSphere(prefix + " Nose",
                  glm::vec3(0.72f, 0.47f, 0.34f),
                  glm::vec3(x, y - 0.045f, z + 0.275f),
                  glm::vec3(0.045f, 0.055f, 0.035f));

        addBox(prefix + " Mouth",
               glm::vec3(0.105f, 0.022f, 0.018f),
               mouthColor,
               glm::vec3(x, y - 0.145f, z + 0.255f));

        // Ears
        addSphere(prefix + " Left Ear",
                  glm::vec3(0.72f, 0.48f, 0.35f),
                  glm::vec3(x - 0.275f, y - 0.015f, z + 0.015f),
                  glm::vec3(0.065f, 0.085f, 0.055f));

        addSphere(prefix + " Right Ear",
                  glm::vec3(0.72f, 0.48f, 0.35f),
                  glm::vec3(x + 0.275f, y - 0.015f, z + 0.015f),
                  glm::vec3(0.065f, 0.085f, 0.055f));
    };

    // ------------------------------------------------------------
    // 1. STAGE FLOOR AND PODIUM
    // ------------------------------------------------------------

    const glm::vec3 darkFloor(0.16f, 0.17f, 0.20f);
    const glm::vec3 stageColor(0.09f, 0.09f, 0.11f);

    addBox("Stage Floor",
        glm::vec3(16.0f, 1.9f, 10.0f),
        darkFloor,
        glm::vec3(0.0f, -0.95f, 0.0f));

    const glm::vec3 stageMetal(0.48f, 0.50f, 0.55f);
    const glm::vec3 stagePanel(0.08f, 0.08f, 0.10f);
    const glm::vec3 stageFrame(0.28f, 0.30f, 0.34f);

    addBox("Stage Upper Edge",
        glm::vec3(16.1f, 0.12f, 0.15f),
        stageMetal,
        glm::vec3(0.0f, -0.06f, 5.0f));

    addBox("Stage Lower Edge",
        glm::vec3(16.1f, 0.12f, 0.15f),
        stageMetal,
        glm::vec3(0.0f, -1.44f, 5.0f));

    for (int i = 0; i < 8; i++)
    {
        float x = -7.0f + i * 2.0f;

        addBox("Stage Front Panel " + std::to_string(i),
            glm::vec3(1.88f, 1.12f, 0.08f),
            stagePanel,
            glm::vec3(x, -0.75f, 5.05f));
    }

    for (int i = 0; i < 9; i++)
    {
        float x = -8.0f + i * 2.0f;

        addBox("Stage Panel Frame " + std::to_string(i),
            glm::vec3(0.08f, 1.30f, 0.12f),
            stageFrame,
            glm::vec3(x, -0.75f, 5.08f));
    }

    addBox("Stage Left Edge",
        glm::vec3(0.12f, 0.12f, 10.0f),
        stageMetal,
        glm::vec3(-8.0f, -0.06f, 0.0f));

    addBox("Stage Right Edge",
        glm::vec3(0.12f, 0.12f, 10.0f),
        stageMetal,
        glm::vec3(8.0f, -0.06f, 0.0f));    

    const glm::vec3 ledColor(0.05f, 0.35f, 0.65f);

    addBox("Stage LED Front",
        glm::vec3(15.9f, 0.045f, 0.06f),
        ledColor,
        glm::vec3(0.0f, -0.18f, 5.18f));

    addBox("Stage LED Left",
        glm::vec3(0.06f, 0.045f, 9.9f),
        ledColor,
        glm::vec3(-8.08f, -0.18f, 0.0f));

    addBox("Stage LED Right",
        glm::vec3(0.06f, 0.045f, 9.9f),
        ledColor,
        glm::vec3(8.08f, -0.18f, 0.0f));

    addBox("Stage LED Bottom",
        glm::vec3(15.9f, 0.035f, 0.06f),
        ledColor,
        glm::vec3(0.0f, -1.32f, 5.18f));


    addBox("Front Podium",
           glm::vec3(5.0f, 0.3f, 1.5f),
           stageColor,
           glm::vec3(0.0f, 0.15f, 4.0f));

    // ------------------------------------------------------------
    // 2. BACK WALL AND DECORATIVE PANELS
    // ------------------------------------------------------------

    const glm::vec3 wallColor(0.18f, 0.08f, 0.20f);
    const glm::vec3 panelColor(0.35f, 0.12f, 0.30f);

    addBox("Back Wall",
           glm::vec3(16.0f, 6.0f, 0.3f),
           wallColor,
           glm::vec3(0.0f, 3.0f, -4.8f));

    addBox("Left Wall Panel",
           glm::vec3(4.0f, 2.0f, 0.1f),
           panelColor,
           glm::vec3(-5.0f, 3.0f, -4.6f));

    addBox("Right Wall Panel",
           glm::vec3(4.0f, 2.0f, 0.1f),
           panelColor,
           glm::vec3(5.0f, 3.0f, -4.6f));

    // ------------------------------------------------------------
    // 3. SPOTLIGHT RIGS
    // ------------------------------------------------------------

    const glm::vec3 metalColor(0.25f, 0.27f, 0.30f);
    const glm::vec3 lampColor(1.0f, 0.75f, 0.25f);

    addBox("Spotlight Horizontal Bar",
           glm::vec3(12.0f, 0.15f, 0.15f),
           metalColor,
           glm::vec3(0.0f, 5.5f, -2.5f));

    addCylinder("Left Spotlight Pole", 0.08f, 5.5f,
                metalColor, glm::vec3(-6.0f, 2.75f, -2.5f));

    addCylinder("Right Spotlight Pole", 0.08f, 5.5f,
                metalColor, glm::vec3(6.0f, 2.75f, -2.5f));

    for (int i = 0; i < 5; ++i)
    {
        const float x = -5.0f + static_cast<float>(i) * 2.5f;
        const std::string number = std::to_string(i + 1);

        addCylinder("Spotlight Fixture " + number,
                    0.18f, 0.4f, metalColor,
                    glm::vec3(x, 5.2f, -2.5f));

        addSphere("Spotlight Bulb " + number,
                  lampColor,
                  glm::vec3(x, 4.95f, -2.5f),
                  glm::vec3(0.12f, 0.12f, 0.12f));
    }

    // ------------------------------------------------------------
    // 4. MICROPHONE AND STAND
    // ------------------------------------------------------------

    const glm::vec3 micMetal(0.62f, 0.66f, 0.70f);
    const glm::vec3 micBlack(0.035f, 0.04f, 0.05f);
    const float micZ = 1.85f;

    // Vertical stand and round floor base.
    addCylinder("Microphone Stand", 0.035f, 2.0f,
                micMetal, glm::vec3(-0.18f, 1.0f, micZ));

    addCylinder("Microphone Base", 0.25f, 0.055f,
                micBlack, glm::vec3(-0.18f, 0.0275f, micZ));

    // Height-adjustment collar.
    addCylinder("Microphone Stand Collar", 0.055f, 0.10f,
                micBlack, glm::vec3(-0.18f, 1.48f, micZ));

    // Horizontal boom, microphone body, and rounded grille.
    addRotatedBox("Microphone Boom",
                  glm::vec3(0.62f, 0.045f, 0.045f),
                  micMetal,
                  glm::vec3(0.13f, 1.99f, micZ),
                  0.0f);

    addRotatedBox("Microphone Body",
                  glm::vec3(0.25f, 0.085f, 0.085f),
                  micBlack,
                  glm::vec3(0.43f, 2.02f, micZ),
                  0.0f);

    addSphere("Microphone Grille",
              glm::vec3(0.18f, 0.19f, 0.21f),
              glm::vec3(0.565f, 2.02f, micZ),
              glm::vec3(0.085f, 0.075f, 0.075f));

    // Small silver band between the microphone body and grille.
    addRotatedBox("Microphone Grille Ring",
                  glm::vec3(0.025f, 0.095f, 0.095f),
                  micMetal,
                  glm::vec3(0.52f, 2.02f, micZ),
                  0.0f);

    // ------------------------------------------------------------
    // 5. DRUM SET
    // ------------------------------------------------------------

    const glm::vec3 drumRed(0.55f, 0.045f, 0.075f);
    const glm::vec3 drumDarkRed(0.22f, 0.025f, 0.035f);
    const glm::vec3 drumHead(0.72f, 0.73f, 0.75f);
    const glm::vec3 drumSilver(0.65f, 0.67f, 0.70f);

    // Bass drum: horizontal cylinder, with its front facing the camera (+Z).
    addCylinderAlongZ("Bass Drum Shell",
                      0.58f, 0.65f, drumRed,
                      glm::vec3(-3.0f, 0.58f, -2.0f));

    addSphere("Bass Drum Front Head",
              drumHead,
              glm::vec3(-3.0f, 0.58f, -1.635f),
              glm::vec3(0.49f, 0.49f, 0.035f));

    addSphere("Bass Drum Center Logo",
              drumDarkRed,
              glm::vec3(-3.0f, 0.58f, -1.592f),
              glm::vec3(0.085f, 0.085f, 0.018f));

    // Two tom drums are also oriented horizontally.
    addCylinderAlongZ("Left Tom Drum",
                      0.32f, 0.38f, drumRed,
                      glm::vec3(-3.80f, 1.35f, -2.05f));

    addCylinderAlongZ("Right Tom Drum",
                      0.32f, 0.38f, drumRed,
                      glm::vec3(-2.35f, 1.35f, -2.05f));

    addSphere("Left Tom Drum Head",
              drumHead,
              glm::vec3(-3.80f, 1.35f, -1.825f),
              glm::vec3(0.265f, 0.265f, 0.025f));

    addSphere("Right Tom Drum Head",
              drumHead,
              glm::vec3(-2.35f, 1.35f, -1.825f),
              glm::vec3(0.265f, 0.265f, 0.025f));

    // Vertical supports below the tom drums.
    addCylinder("Left Tom Support", 0.035f, 0.85f,
                drumSilver, glm::vec3(-3.80f, 0.55f, -2.05f));

    addCylinder("Right Tom Support", 0.035f, 0.85f,
                drumSilver, glm::vec3(-2.35f, 0.55f, -2.05f));

    // Cymbals and the stands underneath them.
    addCylinder("Left Cymbal", 0.50f, 0.05f,
                drumSilver, glm::vec3(-4.50f, 1.80f, -2.0f));

    addCylinder("Right Cymbal", 0.50f, 0.05f,
                drumSilver, glm::vec3(-1.80f, 1.80f, -2.0f));

    addCylinder("Left Cymbal Stand", 0.025f, 1.70f,
                metalColor, glm::vec3(-4.50f, 0.925f, -2.0f));

    addCylinder("Right Cymbal Stand", 0.025f, 1.70f,
                metalColor, glm::vec3(-1.80f, 0.925f, -2.0f));

    // Small cymbal mounting posts.
    addCylinder("Left Cymbal Mount", 0.045f, 0.12f,
                metalColor, glm::vec3(-4.50f, 1.86f, -2.0f));

    addCylinder("Right Cymbal Mount", 0.045f, 0.12f,
                metalColor, glm::vec3(-1.80f, 1.86f, -2.0f));
    // ------------------------------------------------------------
    // 5A. DRUMMER
    // ------------------------------------------------------------
    // Seated drummer, positioned behind the existing drum kit.
    const glm::vec3 drummerSkin(0.85f, 0.62f, 0.45f);
    const glm::vec3 drummerShirt(0.10f, 0.12f, 0.17f);
    const glm::vec3 drummerPants(0.055f, 0.06f, 0.075f);
    const glm::vec3 drummerHair(0.035f, 0.025f, 0.022f);
    const glm::vec3 drumStickColor(0.72f, 0.48f, 0.25f);

    // Drum throne.
    addCylinder("Drummer Stool Base", 0.24f, 0.06f,
                metalColor, glm::vec3(-3.0f, 0.10f, -3.35f));
    addCylinder("Drummer Stool Stem", 0.045f, 0.75f,
                metalColor, glm::vec3(-3.0f, 0.50f, -3.35f));
    addCylinder("Drummer Stool Seat", 0.30f, 0.10f,
                glm::vec3(0.045f, 0.045f, 0.055f),
                glm::vec3(-3.0f, 0.90f, -3.35f));

    // Torso, neck, head and face.
    // One solid rounded drummer torso
    addSphere("Drummer Torso",
            drummerShirt,
            glm::vec3(-3.0f, 1.48f, -3.35f),
            glm::vec3(0.40f, 0.48f, 0.25f));
    addSphere("Drummer Neck", drummerSkin,
              glm::vec3(-3.0f, 1.91f, -3.34f),
              glm::vec3(0.13f, 0.13f, 0.13f));
    addSphere("Drummer Head", drummerSkin,
              glm::vec3(-3.0f, 2.18f, -3.35f),
              glm::vec3(0.25f, 0.29f, 0.23f));
    addFaceDetails("Drummer", -3.0f, 2.18f, -3.35f, drummerHair);

    // Bent legs in a seated position.
    const glm::vec3 drummerLeftHip(-3.22f, 1.12f, -3.28f);
    const glm::vec3 drummerLeftKnee(-3.66f, 0.70f, -2.78f);
    const glm::vec3 drummerLeftAnkle(-3.78f, 0.16f, -2.28f);
    const glm::vec3 drummerRightHip(-2.78f, 1.12f, -3.28f);
    const glm::vec3 drummerRightKnee(-2.34f, 0.70f, -2.78f);
    const glm::vec3 drummerRightAnkle(-2.22f, 0.16f, -2.28f);

    addLimb("Drummer Left Thigh", drummerLeftHip, drummerLeftKnee,
            0.22f, 0.22f, drummerPants);
    addLimb("Drummer Left Shin", drummerLeftKnee, drummerLeftAnkle,
            0.17f, 0.18f, drummerPants);
    addLimb("Drummer Right Thigh", drummerRightHip, drummerRightKnee,
            0.22f, 0.22f, drummerPants);
    addLimb("Drummer Right Shin", drummerRightKnee, drummerRightAnkle,
            0.17f, 0.18f, drummerPants);
    addBox("Drummer Left Shoe", glm::vec3(0.30f, 0.14f, 0.42f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(-3.78f, 0.10f, -2.16f));
    addBox("Drummer Right Shoe", glm::vec3(0.30f, 0.14f, 0.42f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(-2.22f, 0.10f, -2.16f));

    // ------------------------------------------------------------
// ------------------------------------------------------------
// DRUMMER ARMS - 3D CONNECTED ARMS
// ------------------------------------------------------------

// Left arm joints
const glm::vec3 drummerLeftShoulder(-3.34f, 1.78f, -3.22f);
const glm::vec3 drummerLeftElbow(-3.60f, 1.63f, -2.76f);
const glm::vec3 drummerLeftWrist(-3.75f, 1.72f, -2.30f);

// Right arm joints
const glm::vec3 drummerRightShoulder(-2.66f, 1.78f, -3.22f);
const glm::vec3 drummerRightElbow(-2.40f, 1.63f, -2.76f);
const glm::vec3 drummerRightWrist(-2.25f, 1.72f, -2.30f);


// Upper arms
addLimb3D("Drummer Left Upper Arm",
          drummerLeftShoulder,
          drummerLeftElbow,
          0.095f, drummerSkin);

addLimb3D("Drummer Right Upper Arm",
          drummerRightShoulder,
          drummerRightElbow,
          0.095f, drummerSkin);


// Elbow joints
addSphere("Drummer Left Elbow",
          drummerSkin,
          drummerLeftElbow,
          glm::vec3(0.105f, 0.105f, 0.105f));

addSphere("Drummer Right Elbow",
          drummerSkin,
          drummerRightElbow,
          glm::vec3(0.105f, 0.105f, 0.105f));


// Forearms
addLimb3D("Drummer Left Forearm",
          drummerLeftElbow,
          drummerLeftWrist,
          0.078f, drummerSkin);

addLimb3D("Drummer Right Forearm",
          drummerRightElbow,
          drummerRightWrist,
          0.078f, drummerSkin);


// Hands
addSphere("Drummer Left Hand",
          drummerSkin,
          drummerLeftWrist,
          glm::vec3(0.105f, 0.09f, 0.10f));

addSphere("Drummer Right Hand",
          drummerSkin,
          drummerRightWrist,
          glm::vec3(0.105f, 0.09f, 0.10f));


// ------------------------------------------------------------
// DRUMSTICKS
// ------------------------------------------------------------

// Left stick: directed towards the left cymbal
addLimb3D("Drummer Left Drumstick",
          drummerLeftWrist,
          glm::vec3(-4.25f, 1.82f, -1.78f),
          0.018f, drumStickColor);

// Right stick: directed towards the right cymbal
addLimb3D("Drummer Right Drumstick",
          drummerRightWrist,
          glm::vec3(-1.75f, 1.82f, -1.78f),
          0.018f, drumStickColor);
    // ------------------------------------------------------------
    // 6. ELECTRIC GUITAR
    // ------------------------------------------------------------

    const glm::vec3 guitarColor(0.88f, 0.17f, 0.045f);
    const glm::vec3 guitarDark(0.07f, 0.055f, 0.045f);
    const glm::vec3 guitarNeckColor(0.36f, 0.20f, 0.095f);
    const glm::vec3 guitarBoardColor(0.10f, 0.065f, 0.045f);
    const glm::vec3 guitarMetal(0.78f, 0.79f, 0.82f);
    const glm::vec3 guitarPickguardColor(0.80f, 0.77f, 0.68f);

    // The guitar is in front of the guitarist (positive Z), so it is
    // visible instead of being hidden inside the character's torso.
    const float guitarZ = 0.34f;
    const float guitarAngle = -22.0f;

    // Two rounded bouts and a central waist create a recognizable
    // double-cutaway electric-guitar silhouette from simple spheres.
    // Lower body
addSphere("Guitar Lower Body",
          guitarColor,
          glm::vec3(3.0f, 0.94f, guitarZ),
          glm::vec3(0.35f, 0.39f, 0.13f));

// Upper body
addSphere("Guitar Upper Body",
          guitarColor,
          glm::vec3(3.0f, 1.30f, guitarZ),
          glm::vec3(0.30f, 0.33f, 0.125f));

// Narrow waist
addSphere("Guitar Body Waist",
          guitarColor,
          glm::vec3(3.0f, 1.12f, guitarZ + 0.005f),
          glm::vec3(0.245f, 0.37f, 0.115f));

// Cream pickguard
addSphere("Guitar Pickguard",
          guitarPickguardColor,
          glm::vec3(2.91f, 1.12f, 0.475f),
          glm::vec3(0.15f, 0.25f, 0.025f));
    // Upper and lower horns improve the electric-guitar silhouette.
    addSphere("Guitar Upper Horn", guitarColor,
              glm::vec3(3.25f, 1.48f, guitarZ),
              glm::vec3(0.16f, 0.30f, 0.13f));
    addSphere("Guitar Lower Horn", guitarColor,
              glm::vec3(2.75f, 1.47f, guitarZ),
              glm::vec3(0.15f, 0.27f, 0.13f));

    // Dark pickguard on the front face.
    
    // Neck and dark fretboard, tilted upward to the right.
    const glm::vec3 neckCenter(3.35f, 2.05f, guitarZ);

    addRotatedBox("Guitar Neck",
                  glm::vec3(0.145f, 1.58f, 0.13f),
                  guitarNeckColor,
                  neckCenter,
                  guitarAngle);

    addRotatedBox("Guitar Fretboard",
                  glm::vec3(0.112f, 1.55f, 0.035f),
                  guitarBoardColor,
                  glm::vec3(neckCenter.x, neckCenter.y, 0.415f),
                  guitarAngle);

    // Headstock continues the same angle as the neck.
    addRotatedBox("Guitar Headstock",
                  glm::vec3(0.24f, 0.34f, 0.14f),
                  guitarNeckColor,
                  glm::vec3(3.72f, 2.89f, guitarZ),
                  guitarAngle);

    // Six strings run from the bridge, along the body and neck, to the headstock.
    const glm::vec3 localX(std::cos(glm::radians(guitarAngle)),
                           std::sin(glm::radians(guitarAngle)),
                           0.0f);

    for (int i = 0; i < 6; ++i)
    {
        const float offset = -0.05f + static_cast<float>(i) * 0.02f;
        const glm::vec3 stringPosition(
            neckCenter.x + localX.x * offset,
            neckCenter.y + localX.y * offset,
            0.485f
        );

        addRotatedBox("Guitar String " + std::to_string(i + 1),
                      glm::vec3(0.005f, 1.82f, 0.005f),
                      guitarMetal,
                      stringPosition,
                      guitarAngle);
    }

    // Frets are short metal bars across the fretboard.
    for (int i = 0; i < 7; ++i)
    {
        const float distance = -0.48f + static_cast<float>(i) * 0.15f;
        const glm::vec3 fretPosition(
            neckCenter.x + std::sin(glm::radians(-guitarAngle)) * distance,
            neckCenter.y + std::cos(glm::radians(guitarAngle)) * distance,
            0.465f
        );

        addRotatedBox("Guitar Fret " + std::to_string(i + 1),
                      glm::vec3(0.135f, 0.014f, 0.012f),
                      guitarMetal,
                      fretPosition,
                      guitarAngle);
    }

    // Bridge, pickups, and control knobs.
    addRotatedBox("Guitar Bridge",
                  glm::vec3(0.27f, 0.055f, 0.035f),
                  guitarMetal,
                  glm::vec3(3.0f, 0.88f, 0.49f),
                  0.0f);

    addRotatedBox("Guitar Neck Pickup",
                  glm::vec3(0.22f, 0.065f, 0.035f),
                  guitarDark,
                  glm::vec3(3.0f, 1.39f, 0.49f),
                  0.0f);

    addRotatedBox("Guitar Bridge Pickup",
                  glm::vec3(0.22f, 0.065f, 0.035f),
                  guitarDark,
                  glm::vec3(3.0f, 1.05f, 0.49f),
                  0.0f);
    // Middle pickup and six pole pieces on each pickup.
    addRotatedBox("Guitar Middle Pickup",
                  glm::vec3(0.22f, 0.065f, 0.035f),
                  guitarDark,
                  glm::vec3(3.0f, 1.22f, 0.49f),
                  0.0f);
    for (int pickup = 0; pickup < 3; ++pickup)
    {
        const float pickupY = 1.39f - static_cast<float>(pickup) * 0.17f;
        for (int pole = 0; pole < 6; ++pole)
        {
            const float poleX = 2.925f + static_cast<float>(pole) * 0.03f;
            addSphere("Guitar Pickup Pole", guitarMetal,
                      glm::vec3(poleX, pickupY, 0.516f),
                      glm::vec3(0.009f, 0.009f, 0.009f));
        }
    }

    // Six small bridge saddles and a pickup selector switch.
    for (int saddle = 0; saddle < 6; ++saddle)
    {
        const float saddleX = 2.93f + static_cast<float>(saddle) * 0.028f;
        addSphere("Guitar Bridge Saddle", guitarMetal,
                  glm::vec3(saddleX, 0.88f, 0.518f),
                  glm::vec3(0.012f, 0.025f, 0.012f));
    }
    addRotatedBox("Guitar Pickup Selector",
                  glm::vec3(0.025f, 0.16f, 0.025f),
                  guitarMetal,
                  glm::vec3(2.76f, 1.22f, 0.515f),
                  -18.0f);

    addSphere("Guitar Control Knob 1",
              guitarPickguardColor,
              glm::vec3(2.78f, 0.91f, 0.50f),
              glm::vec3(0.045f, 0.045f, 0.025f));

    addSphere("Guitar Control Knob 2",
              guitarPickguardColor,
              glm::vec3(2.84f, 0.82f, 0.50f),
              glm::vec3(0.045f, 0.045f, 0.025f));
    addSphere("Guitar Control Knob 3",
              guitarPickguardColor,
              glm::vec3(2.96f, 0.80f, 0.50f),
              glm::vec3(0.045f, 0.045f, 0.025f));
    addSphere("Guitar Output Jack Plate", guitarMetal,
              glm::vec3(3.27f, 0.91f, 0.49f),
              glm::vec3(0.055f, 0.055f, 0.018f));

    // Tuning pegs on both sides of the headstock.
    for (int i = 0; i < 3; ++i)
    {
        const float y = 2.78f + static_cast<float>(i) * 0.09f;

        addSphere("Guitar Tuning Peg Left " + std::to_string(i + 1),
                  guitarMetal,
                  glm::vec3(3.61f - static_cast<float>(i) * 0.035f,
                            y, 0.39f),
                  glm::vec3(0.045f, 0.035f, 0.035f));

        addSphere("Guitar Tuning Peg Right " + std::to_string(i + 1),
                  guitarMetal,
                  glm::vec3(3.82f - static_cast<float>(i) * 0.035f,
                            y - 0.035f, 0.39f),
                  glm::vec3(0.045f, 0.035f, 0.035f));
    }

    // ------------------------------------------------------------
    // 7. SINGER
    // ------------------------------------------------------------

    const glm::vec3 skinColor(0.85f, 0.62f, 0.45f);
    const glm::vec3 shirtColor(0.05f, 0.20f, 0.65f);
    const glm::vec3 pantsColor(0.08f, 0.08f, 0.12f);
    const glm::vec3 hairColor(0.12f, 0.055f, 0.025f);

    // Head and torso.
    addSphere("Singer Head",
              skinColor,
              glm::vec3(0.0f, 2.30f, 1.20f),
              glm::vec3(0.28f, 0.32f, 0.25f));

    addFaceDetails("Singer", 0.0f, 2.30f, 1.20f, hairColor);

    addBox("Singer Torso",
           glm::vec3(0.65f, 0.90f, 0.35f),
           shirtColor,
           glm::vec3(0.0f, 1.45f, 1.20f));

    // Legs and shoes.
    addBox("Singer Left Leg",
           glm::vec3(0.22f, 0.90f, 0.25f),
           pantsColor,
           glm::vec3(-0.18f, 0.45f, 1.20f));

    addBox("Singer Right Leg",
           glm::vec3(0.22f, 0.90f, 0.25f),
           pantsColor,
           glm::vec3(0.18f, 0.45f, 1.20f));

    addBox("Singer Left Shoe",
           glm::vec3(0.28f, 0.13f, 0.40f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(-0.18f, 0.065f, 1.28f));

    addBox("Singer Right Shoe",
           glm::vec3(0.28f, 0.13f, 0.40f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(0.18f, 0.065f, 1.28f));

    // Raised right arm reaches towards the microphone.
    const glm::vec3 singerRightShoulder(0.27f, 1.78f, 1.38f);
    const glm::vec3 singerRightElbow(0.43f, 1.91f, 1.53f);
    const glm::vec3 singerRightWrist(0.32f, 2.035f, 1.73f);

    addLimb("Singer Right Upper Arm",
            singerRightShoulder, singerRightElbow,
            0.18f, 0.17f, skinColor);

    addLimb("Singer Right Forearm",
            singerRightElbow, singerRightWrist,
            0.15f, 0.15f, skinColor);

    addSphere("Singer Right Elbow",
              skinColor, singerRightElbow,
              glm::vec3(0.09f, 0.09f, 0.085f));

    addSphere("Singer Right Hand",
              skinColor, singerRightWrist,
              glm::vec3(0.105f, 0.10f, 0.085f));

    // Left arm is relaxed and slightly bent.
    const glm::vec3 singerLeftShoulder(-0.27f, 1.78f, 1.36f);
    const glm::vec3 singerLeftElbow(-0.45f, 1.39f, 1.43f);
    const glm::vec3 singerLeftWrist(-0.34f, 1.06f, 1.48f);

    addLimb("Singer Left Upper Arm",
            singerLeftShoulder, singerLeftElbow,
            0.18f, 0.17f, skinColor);

    addLimb("Singer Left Forearm",
            singerLeftElbow, singerLeftWrist,
            0.15f, 0.15f, skinColor);

    addSphere("Singer Left Elbow",
              skinColor, singerLeftElbow,
              glm::vec3(0.09f, 0.09f, 0.085f));

    addSphere("Singer Left Hand",
              skinColor, singerLeftWrist,
              glm::vec3(0.10f, 0.10f, 0.085f));

    // ------------------------------------------------------------
    // 8. GUITARIST
    // ------------------------------------------------------------

    const glm::vec3 guitaristShirt(0.10f, 0.45f, 0.25f);
    const glm::vec3 guitaristHair(0.045f, 0.025f, 0.018f);

    addSphere("Guitarist Head",
              skinColor,
              glm::vec3(3.0f, 2.30f, 0.0f),
              glm::vec3(0.28f, 0.32f, 0.25f));

    addFaceDetails("Guitarist", 3.0f, 2.30f, 0.0f, guitaristHair);

    addBox("Guitarist Torso",
           glm::vec3(0.65f, 0.90f, 0.35f),
           guitaristShirt,
           glm::vec3(3.0f, 1.45f, 0.0f));
    // Shoulder strap placed slightly in front of the guitarist's torso.
    addLimb("Guitar Shoulder Strap",
            glm::vec3(3.34f, 2.05f, 0.18f),
            glm::vec3(2.70f, 0.82f, 0.18f),
            0.085f, 0.035f,
            glm::vec3(0.035f, 0.035f, 0.045f));

    // Legs and shoes.
    addBox("Guitarist Left Leg",
           glm::vec3(0.22f, 0.90f, 0.25f),
           pantsColor,
           glm::vec3(2.82f, 0.45f, 0.0f));

    addBox("Guitarist Right Leg",
           glm::vec3(0.22f, 0.90f, 0.25f),
           pantsColor,
           glm::vec3(3.18f, 0.45f, 0.0f));

    addBox("Guitarist Left Shoe",
           glm::vec3(0.28f, 0.13f, 0.40f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(2.82f, 0.065f, 0.08f));

    addBox("Guitarist Right Shoe",
           glm::vec3(0.28f, 0.13f, 0.40f),
           glm::vec3(0.025f, 0.025f, 0.035f),
           glm::vec3(3.18f, 0.065f, 0.08f));

    // The guitarist's left hand reaches the neck to play notes.
    const glm::vec3 guitaristLeftShoulder(3.28f, 1.78f, 0.14f);
    const glm::vec3 guitaristLeftElbow(3.48f, 1.91f, 0.34f);
    const glm::vec3 guitaristLeftWrist(3.49f, 2.23f, 0.54f);

    addLimb("Guitarist Left Upper Arm",
            guitaristLeftShoulder, guitaristLeftElbow,
            0.18f, 0.17f, skinColor);

    addLimb("Guitarist Left Forearm",
            guitaristLeftElbow, guitaristLeftWrist,
            0.15f, 0.15f, skinColor);

    addSphere("Guitarist Left Elbow",
              skinColor, guitaristLeftElbow,
              glm::vec3(0.09f, 0.09f, 0.085f));

    addSphere("Guitarist Left Hand",
              skinColor, guitaristLeftWrist,
              glm::vec3(0.105f, 0.10f, 0.085f));

    // The guitarist's right hand is positioned over the guitar body
    // to represent strumming.
    const glm::vec3 guitaristRightShoulder(2.72f, 1.78f, 0.12f);
    const glm::vec3 guitaristRightElbow(2.53f, 1.48f, 0.34f);
    const glm::vec3 guitaristRightWrist(2.91f, 1.17f, 0.54f);

    addLimb("Guitarist Right Upper Arm",
            guitaristRightShoulder, guitaristRightElbow,
            0.18f, 0.17f, skinColor);

    addLimb("Guitarist Right Forearm",
            guitaristRightElbow, guitaristRightWrist,
            0.15f, 0.15f, skinColor);

    addSphere("Guitarist Right Elbow",
              skinColor, guitaristRightElbow,
              glm::vec3(0.09f, 0.09f, 0.085f));

    addSphere("Guitarist Right Hand",
              skinColor, guitaristRightWrist,
              glm::vec3(0.105f, 0.10f, 0.085f));

    const glm::vec3 clubWallColor(0.32f, 0.23f, 0.20f);

    addBox("Club Left Wall",
        glm::vec3(0.4f, 10.0f, 32.0f),
        clubWallColor,
        glm::vec3(-9.0f, 5.0f, 10.0f));

    addBox("Club Right Wall",
        glm::vec3(0.4f, 10.0f, 32.0f),
        clubWallColor,
        glm::vec3(9.0f, 5.0f, 10.0f));

    addBox("Club Front Wall",
        glm::vec3(18.0f, 10.0f, 0.4f),
        clubWallColor,
        glm::vec3(0.0f, 5.0f, -6.0f));

    addBox("Club Rear Left Wall",
        glm::vec3(7.5f, 10.0f, 0.4f),
        clubWallColor,
        glm::vec3(-5.25f, 5.0f, 26.0f));

    addBox("Club Rear Right Wall",
        glm::vec3(7.5f, 10.0f, 0.4f),
        clubWallColor,
        glm::vec3(5.25f, 5.0f, 26.0f));

    addBox("Club Entrance Top",
        glm::vec3(3.0f, 6.5f, 0.4f),
        clubWallColor,
        glm::vec3(0.0f, 6.75f, 26.0f));

    return parts;
}

}
