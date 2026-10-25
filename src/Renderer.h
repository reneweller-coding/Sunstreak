/**
 * @file Renderer.h
 * @brief Draws a frame: sky, world, cars, particles and weather into an HDR target, then bloom, the speed blur, tone
 *        mapping and FXAA onto the screen. The HUD is drawn afterwards by the game.
 *
 * Everything is drawn relative to a floating origin at the camera (doubles in, floats out), so a run can go on for
 * thousands of kilometres without jitter.
 *
 * @author Rene Weller
 */
#pragma once

#include "Biome.h"
#include "Car.h"
#include "Models.h"
#include "Particles.h"
#include "Profiler.h"
#include "Traffic.h"
#include "World.h"

#include <raylib.h>

#include <vector>

namespace sunstreak
{

/** @brief Where the camera is and where it looks. */
struct CameraView
{
    Vec3d   m_pos;              ///< world position
    Vector3 m_forward{ 0, 0, 1 };  ///< viewing direction
    Vector3 m_up{ 0, 1, 0 };       ///< up (with roll)
    float   m_fovY = 60.0f;     ///< vertical field of view (degrees)
};

/** @brief A car drawn from the player's models: a rival or the ghost. */
struct CarDraw
{
    double m_s = 0;        ///< distance along the road
    float  m_x = 0;        ///< lateral offset
    float  m_yaw = 0;      ///< yaw against the road
    int    m_car = 0;      ///< model
    int    m_paint = 0;    ///< paint
    float  m_alpha = 1.0f; ///< below 1: a see-through ghost without a shadow
};

/** @brief A patrol car of the police, its light bar flashing or dark. */
struct PoliceDraw
{
    double m_s = 0;       ///< distance along the road
    float  m_x = 0;       ///< lateral offset
    float  m_yaw = 0;     ///< yaw against the road
    int    m_model = 0;   ///< index into Models::m_traffic
    int    m_flash = -1;  ///< -1 dark, 0 the red half lit, 1 the blue half
};

/** @brief A lone model in the scene (the landmark viewer, a crossing). */
struct ScenePiece
{
    const Mesh *m_mesh = nullptr;   ///< the model
    Vec3d       m_pos;              ///< where its origin stands
    float       m_yaw = 0.0f;       ///< turned about the vertical (rad)
};

/** @brief Everything one frame shows. */
struct SceneInput
{
    const Track     *m_track = nullptr;       ///< the road
    const World     *m_world = nullptr;       ///< the chunks
    const Models    *m_models = nullptr;      ///< the meshes
    const PlayerCar *m_player = nullptr;      ///< the player (nullptr: none)
    int              m_car = 0;               ///< the player's car
    int              m_paint = 0;             ///< and its paint
    const Traffic   *m_traffic = nullptr;     ///< traffic and cones
    const Particles *m_particles = nullptr;   ///< smoke, dust, sparks
    Environment      m_env{};                 ///< light and air at the camera
    Surface          m_surface{};             ///< colours at the camera (neon grid)
    float            m_time = 0.0f;           ///< seconds since start (animation)
    float            m_dt = 0.0f;             ///< frame time (weather)
    float            m_flash = 0.0f;          ///< lightning 0..1
    float            m_speedBlur = 0.0f;      ///< 0..1 radial blur
    float            m_aberration = 0.0f;     ///< 0..1 colour fringes (boost)
    float            m_windStreaks = 0.0f;    ///< 0..1 air streaks at speed
    bool             m_drawPlayer = true;     ///< false in the bumper view
    float            m_behind = 60.0f;        ///< metres of scenery drawn behind the player (more when looking back)
    std::vector<CarDraw> m_cars;              ///< rivals and the ghost
    std::vector<PoliceDraw> m_police;         ///< the police
    std::vector<ScenePiece> m_puddles;        ///< the puddles the rain left (m_mesh unused: the models' puddle)
    float            m_slush = 0.0f;          ///< 0..1 slush on the road (snow)
    bool             m_shadows = true;        ///< real-time sun shadows
    float            m_lensRain = 0.0f;       ///< 0..1 drops on the lens
    bool             m_showcase = false;      ///< lone models instead of the world (the landmark viewer, a crossing)
    std::vector<ScenePiece> m_pieces;         ///< those models
    Vec3d            m_focus;                 ///< where the shadows centre then
};

/** @brief The renderer. */
class Renderer
{
public:
    /** @brief Compiles the shaders (needs the window). */
    void init();

    /** @brief Frees shaders and targets. */
    void unload();

    /**
     * @brief The GPU time of the render passes.
     * @return the timer
     */
    GpuTimer &gpu() { return m_gpu; }

    /**
     * @brief Draws a frame onto the screen (the caller is between BeginDrawing and EndDrawing).
     * @param[in] in           what to show
     * @param[in] cam          from where
     * @param[in] renderScale  3D resolution against the window
     */
    void render( const SceneInput &in, const CameraView &cam, float renderScale );

private:
    /** @brief A compiled shader and the uniform locations it needs. */
    struct Program
    {
        Shader m_shader{};   ///< the shader
        /**
         * @brief A uniform's location.
         * @param[in] name  the uniform
         * @return its location (-1 if unused)
         */
        int loc( const char *name ) const { return GetShaderLocation( m_shader, name ); }
    };

    /**
     * @brief Makes the targets match the window.
     * @param[in] w      window width
     * @param[in] h      window height
     * @param[in] scale  render scale
     */
    void ensureTargets( int w, int h, float scale );

    /** @brief Frees the targets. */
    void unloadTargets();

    /**
     * @brief Renders the shadow map from the sun around the camera.
     * @param[in] in      the frame
     * @param[in] cam     the camera
     * @param[in] origin  the floating origin
     */
    void drawShadows( const SceneInput &in, const CameraView &cam, const Vec3d &origin );

    /**
     * @brief Draws the sky into the scene target.
     * @param[in] in   the frame
     * @param[in] cam  the camera
     */
    void drawSky( const SceneInput &in, const CameraView &cam );

    /**
     * @brief Draws the 3D world into the scene target.
     * @param[in] in      the frame
     * @param[in] cam     the camera
     * @param[in] origin  the floating origin
     */
    void drawWorld( const SceneInput &in, const CameraView &cam, const Vec3d &origin );

    /**
     * @brief Draws particles, rain, snow and air streaks.
     * @param[in] in      the frame
     * @param[in] cam     the camera
     * @param[in] origin  the floating origin
     */
    void drawParticles( const SceneInput &in, const CameraView &cam, const Vec3d &origin );

    /**
     * @brief Bloom, composite and FXAA onto the screen.
     * @param[in] in  the frame
     */
    void post( const SceneInput &in );

    /**
     * @brief Draws a texture over a whole target with the current shader (flipping a render texture upright).
     * @param[in] tex  the source
     * @param[in] w    destination width
     * @param[in] h    destination height
     */
    static void blit( const Texture2D &tex, int w, int h );

    /**
     * @brief Draws one mesh at many places with the world shader: instanced from three on (cones, puddles, shadows).
     * @param[in] mesh        the mesh
     * @param[in] transforms  where (relative to the floating origin)
     */
    void drawMany( const Mesh &mesh, const std::vector<Matrix> &transforms );

    Program  m_world;       ///< lit world
    Program  m_sky;         ///< sky
    Program  m_particle;    ///< sprites
    Program  m_bright;      ///< bloom bright pass
    Program  m_blur;        ///< bloom blur
    Program  m_composite;   ///< tone mapping and effects
    Program  m_fxaa;        ///< anti-aliasing
    Program  m_depth;       ///< shadow pass
    Program  m_mirror;      ///< reflections on the wet road and in puddles
    GpuTimer m_gpu;         ///< GPU time per pass
    Material m_material{};  ///< carries the world shader
    int      m_instanceAttrib = -1;   ///< the world shader's instanceTransform attribute
    Material m_depthMaterial{};   ///< carries the depth shader

    RenderTexture2D m_shadowMap{};       ///< depth from the sun
    Matrix          m_lightVP{};         ///< the sun's view and projection
    float           m_shadowStrength = 0.0f;   ///< 0 no shadows this frame
    Vector2         m_sunUv{};           ///< the sun on screen (texture coordinates)
    float           m_flare = 0.0f;      ///< lens flare strength this frame

    RenderTexture2D m_scene{};    ///< HDR scene with depth
    RenderTexture2D m_bloomA{};   ///< 1/4 resolution
    RenderTexture2D m_bloomB{};   ///< 1/4 resolution, ping-pong
    RenderTexture2D m_bloomC{};   ///< 1/8 resolution
    RenderTexture2D m_bloomD{};   ///< 1/8 resolution, ping-pong
    RenderTexture2D m_ldr{};      ///< tone-mapped picture
    RenderTexture2D m_mirrored{}; ///< the HDR scene with its reflections
    int             m_targetW = 0;      ///< scene target width
    int             m_targetH = 0;      ///< scene target height
    int             m_screenW = 0;      ///< window width the targets were made for
    int             m_screenH = 0;      ///< window height

    std::vector<Vector3> m_drops;           ///< rain or snow, relative to the camera
    std::vector<Vector3> m_streaks;         ///< air streaks, relative to the camera
    std::vector<Vector3> m_shafts;          ///< light shafts under the sea (their feet), relative to the camera
    Vec3d                m_lastCam;         ///< the camera last frame
    Vector3              m_camVel{};        ///< camera velocity (m/s)
    bool                 m_haveLastCam = false;   ///< m_lastCam is valid
};

} // namespace sunstreak
