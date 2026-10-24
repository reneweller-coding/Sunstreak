/**
 * @file Renderer.cpp
 * @brief The frame: sky, world, particles, weather and post-processing.
 * @author reneweller
 */
#include "Renderer.h"

#include "Festival.h"

#include "Shaders.h"

#include <rlgl.h>

namespace sunstreak
{

namespace
{

constexpr float Bend = 2.2e-5f;            ///< how fast the ground drops with distance (1/m)
constexpr int   DropCount = 1600;          ///< rain drops or snowflakes around the camera
constexpr int   StreakCount = 90;          ///< air streaks around the camera
constexpr int   ShaftCount = 16;           ///< light shafts round the camera under the sea
constexpr float NearPlane = 0.3f;          ///< near clip (m)
constexpr float FarPlane = 2600.0f;        ///< far clip (m)
constexpr int   ShadowSize = 2048;         ///< shadow map resolution
constexpr float ShadowExtent = 160.0f;     ///< the shadow map's side (m)
constexpr int   ShadowSlot = 10;           ///< texture unit of the shadow map

/**
 * @brief A render target, optionally half-float and with depth.
 * @param[in] w      width
 * @param[in] h      height
 * @param[in] hdr    half-float colour
 * @param[in] depth  with a depth buffer
 * @return the target
 */
RenderTexture2D loadTarget( int w, int h, bool hdr, bool depth )
{
    RenderTexture2D t = { 0 };
    t.id = rlLoadFramebuffer();
    rlEnableFramebuffer( t.id );
    const int format = hdr ? PIXELFORMAT_UNCOMPRESSED_R16G16B16A16 : PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    t.texture.id = rlLoadTexture( nullptr, w, h, format, 1 );
    t.texture.width = w;
    t.texture.height = h;
    t.texture.format = format;
    t.texture.mipmaps = 1;
    rlFramebufferAttach( t.id, t.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0 );
    if( depth )
    {
        t.depth.id = rlLoadTextureDepth( w, h, true );
        t.depth.width = w;
        t.depth.height = h;
        t.depth.format = 19;
        t.depth.mipmaps = 1;
        rlFramebufferAttach( t.id, t.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0 );
    }
    if( !rlFramebufferComplete( t.id ) ) TraceLog( LOG_WARNING, "render target %dx%d is incomplete", w, h );
    rlDisableFramebuffer();
    SetTextureFilter( t.texture, TEXTURE_FILTER_BILINEAR );
    SetTextureWrap( t.texture, TEXTURE_WRAP_CLAMP );
    return t;
}

/**
 * @brief Sets a float uniform.
 * @param[in] s     the shader
 * @param[in] name  the uniform
 * @param[in] v     the value
 */
void setF( const Shader &s, const char *name, float v )
{
    SetShaderValue( s, GetShaderLocation( s, name ), &v, SHADER_UNIFORM_FLOAT );
}

/**
 * @brief Sets a vec2 uniform.
 * @param[in] s     the shader
 * @param[in] name  the uniform
 * @param[in] x     first component
 * @param[in] y     second component
 */
void setV2( const Shader &s, const char *name, float x, float y )
{
    const float v[2] = { x, y };
    SetShaderValue( s, GetShaderLocation( s, name ), v, SHADER_UNIFORM_VEC2 );
}

/**
 * @brief Sets a vec3 uniform.
 * @param[in] s     the shader
 * @param[in] name  the uniform
 * @param[in] v     the value
 */
void setV3( const Shader &s, const char *name, Vector3 v )
{
    SetShaderValue( s, GetShaderLocation( s, name ), &v, SHADER_UNIFORM_VEC3 );
}

/**
 * @brief The direction to the sun.
 * @param[in] env  the environment
 * @return a unit vector
 */
Vector3 sunDirection( const Environment &env )
{
    const float e = env.m_sunElevation * DEG2RAD, a = env.m_sunAzimuth * DEG2RAD;
    return Vector3{ -std::sin( a ) * std::cos( e ), std::sin( e ), std::cos( a ) * std::cos( e ) };
}

/**
 * @brief A model matrix from a pose.
 * @param[in] p       the pose
 * @param[in] origin  the floating origin
 * @return the matrix
 */
Matrix poseMatrix( const Pose &p, const Vec3d &origin )
{
    return basisMatrix( p.m_left, p.m_up, p.m_forward, relativeTo( p.m_pos, origin ) );
}

/**
 * @brief One camera-facing or ribbon quad into raylib's batch.
 * @param[in] a     corner
 * @param[in] b     corner
 * @param[in] c     corner
 * @param[in] d     corner
 * @param[in] col   colour
 */
void quad( Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color col )
{
    rlCheckRenderBatchLimit( 4 );
    rlColor4ub( col.r, col.g, col.b, col.a );
    rlTexCoord2f( 0.0f, 0.0f );
    rlVertex3f( a.x, a.y, a.z );
    rlTexCoord2f( 0.0f, 1.0f );
    rlVertex3f( b.x, b.y, b.z );
    rlTexCoord2f( 1.0f, 1.0f );
    rlVertex3f( c.x, c.y, c.z );
    rlTexCoord2f( 1.0f, 0.0f );
    rlVertex3f( d.x, d.y, d.z );
}

/**
 * @brief Draws a quad whose first two corners have one colour and the last two another (a gradient along it).
 * @param[in] a     first corner
 * @param[in] b     second corner
 * @param[in] c     third corner
 * @param[in] d     fourth corner
 * @param[in] colA  colour at a and b
 * @param[in] colC  colour at c and d
 */
void quad2( Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color colA, Color colC )
{
    rlCheckRenderBatchLimit( 4 );
    rlColor4ub( colA.r, colA.g, colA.b, colA.a );
    rlTexCoord2f( 0.0f, 0.0f );
    rlVertex3f( a.x, a.y, a.z );
    rlTexCoord2f( 0.0f, 1.0f );
    rlVertex3f( b.x, b.y, b.z );
    rlColor4ub( colC.r, colC.g, colC.b, colC.a );
    rlTexCoord2f( 1.0f, 1.0f );
    rlVertex3f( c.x, c.y, c.z );
    rlTexCoord2f( 1.0f, 0.0f );
    rlVertex3f( d.x, d.y, d.z );
}

/**
 * @brief Whether a distance lies in a tunnel.
 * @param[in] track  the road
 * @param[in] s      the distance
 * @return 1 inside, 0 outside
 */
float inTunnel( const Track &track, double s )
{
    return ( track.segment( static_cast<long>( s / SegmentLength ) ).m_flags & SEG_TUNNEL ) ? 1.0f : 0.0f;
}

} // namespace

void Renderer::init()
{
    m_world.m_shader = LoadShaderFromMemory( shaders::WorldVs, shaders::WorldFs );
    m_sky.m_shader = LoadShaderFromMemory( shaders::ScreenVs, shaders::SkyFs );
    m_particle.m_shader = LoadShaderFromMemory( shaders::ParticleVs, shaders::ParticleFs );
    m_bright.m_shader = LoadShaderFromMemory( shaders::ScreenVs, shaders::BrightFs );
    m_blur.m_shader = LoadShaderFromMemory( shaders::ScreenVs, shaders::BlurFs );
    m_composite.m_shader = LoadShaderFromMemory( shaders::ScreenVs, shaders::CompositeFs );
    m_fxaa.m_shader = LoadShaderFromMemory( shaders::ScreenVs, shaders::FxaaFs );
    m_depth.m_shader = LoadShaderFromMemory( shaders::DepthVs, shaders::DepthFs );
    m_material = LoadMaterialDefault();
    m_material.shader = m_world.m_shader;
    m_depthMaterial = LoadMaterialDefault();
    m_depthMaterial.shader = m_depth.m_shader;
    // A depth-only target for the sun's view.
    m_shadowMap.id = rlLoadFramebuffer();
    m_shadowMap.texture.width = ShadowSize;
    m_shadowMap.texture.height = ShadowSize;
    m_shadowMap.depth.id = rlLoadTextureDepth( ShadowSize, ShadowSize, false );
    m_shadowMap.depth.width = ShadowSize;
    m_shadowMap.depth.height = ShadowSize;
    m_shadowMap.depth.format = 19;
    m_shadowMap.depth.mipmaps = 1;
    rlFramebufferAttach( m_shadowMap.id, m_shadowMap.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_TEXTURE2D, 0 );
    if( !rlFramebufferComplete( m_shadowMap.id ) ) TraceLog( LOG_WARNING, "shadow map framebuffer incomplete" );
    rlDisableFramebuffer();
    Rng r( 77 );
    m_drops.resize( DropCount );
    for( Vector3 &d : m_drops ) d = Vector3{ r.range( -30, 30 ), r.range( -6, 16 ), r.range( -12, 48 ) };
    m_streaks.resize( StreakCount );
    for( Vector3 &d : m_streaks ) d = Vector3{ r.range( -12, 12 ), r.range( -1.5f, 4.0f ), r.range( -20, 60 ) };
    m_shafts.resize( ShaftCount );
    for( Vector3 &d : m_shafts ) d = Vector3{ r.range( -45, 45 ), -3.0f, r.range( -40, 100 ) };
}

void Renderer::unloadTargets()
{
    for( RenderTexture2D *t : { &m_scene, &m_bloomA, &m_bloomB, &m_bloomC, &m_bloomD, &m_ldr } )
    {
        if( t->id ) UnloadRenderTexture( *t );
        *t = RenderTexture2D{};
    }
}

void Renderer::unload()
{
    unloadTargets();
    for( Program *p : { &m_world, &m_sky, &m_particle, &m_bright, &m_blur, &m_composite, &m_fxaa, &m_depth } ) UnloadShader( p->m_shader );
    MemFree( m_material.maps );
    m_material.maps = nullptr;
    MemFree( m_depthMaterial.maps );
    m_depthMaterial.maps = nullptr;
    rlUnloadTexture( m_shadowMap.depth.id );
    rlUnloadFramebuffer( m_shadowMap.id );
}

void Renderer::ensureTargets( int w, int h, float scale )
{
    const int tw = std::max( 64, static_cast<int>( static_cast<float>( w ) * scale ) );
    const int th = std::max( 64, static_cast<int>( static_cast<float>( h ) * scale ) );
    if( tw == m_targetW && th == m_targetH && w == m_screenW && h == m_screenH && m_scene.id ) return;
    unloadTargets();
    m_targetW = tw;
    m_targetH = th;
    m_screenW = w;
    m_screenH = h;
    m_scene = loadTarget( tw, th, true, true );
    m_bloomA = loadTarget( std::max( 16, tw / 4 ), std::max( 16, th / 4 ), true, false );
    m_bloomB = loadTarget( std::max( 16, tw / 4 ), std::max( 16, th / 4 ), true, false );
    m_bloomC = loadTarget( std::max( 8, tw / 8 ), std::max( 8, th / 8 ), true, false );
    m_bloomD = loadTarget( std::max( 8, tw / 8 ), std::max( 8, th / 8 ), true, false );
    m_ldr = loadTarget( tw, th, false, false );
}

void Renderer::blit( const Texture2D &tex, int w, int h )
{
    DrawTexturePro( tex, Rectangle{ 0, 0, static_cast<float>( tex.width ), -static_cast<float>( tex.height ) },
                    Rectangle{ 0, 0, static_cast<float>( w ), static_cast<float>( h ) }, Vector2{ 0, 0 }, 0.0f, WHITE );
}

void Renderer::render( const SceneInput &in, const CameraView &cam, float renderScale )
{
    ensureTargets( GetScreenWidth(), GetScreenHeight(), renderScale );

    // Camera motion for the weather, which lives in the camera's neighbourhood.
    Vector3 delta{};
    if( m_haveLastCam )
    {
        delta = relativeTo( cam.m_pos, m_lastCam );
        if( Vector3Length( delta ) > 60.0f ) delta = Vector3{};
    }
    m_camVel = in.m_dt > 0.0f ? Vector3Scale( delta, 1.0f / in.m_dt ) : Vector3{};
    m_lastCam = cam.m_pos;
    m_haveLastCam = true;
    const Vector3 flatFwd = Vector3Normalize( Vector3{ cam.m_forward.x, 0.0f, cam.m_forward.z } );
    const Vector3 boxCenter = Vector3Scale( flatFwd, 18.0f );
    for( size_t i = 0; i < m_drops.size(); ++i )
    {
        Vector3 &d = m_drops[i];
        d = Vector3Subtract( d, delta );
        if( in.m_env.m_underwater > 0.3f )
        { // Under the sea: every fourth a bubble rising, the rest the sea's snow drifting down.
            const float fi = static_cast<float>( i );
            if( i % 4 == 0 )
            {
                d.y += ( 1.3f + 0.5f * std::sin( fi * 1.7f ) ) * in.m_dt;
                d.x += std::sin( in.m_time * 3.0f + fi ) * 0.35f * in.m_dt;
            }
            else
            {
                d.y -= 0.25f * in.m_dt;
                d.x += std::sin( in.m_time * 0.6f + fi * 0.7f ) * 0.3f * in.m_dt;
            }
        }
        else if( std::max( { in.m_env.m_leaves, in.m_env.m_petals, in.m_env.m_confetti } ) > std::max( in.m_env.m_rain, in.m_env.m_snow ) )
        { // Leaves and petals: drifting down, swaying.
            const float fi = static_cast<float>( i );
            d.y -= ( 0.9f + 0.4f * std::sin( fi * 2.3f ) ) * in.m_dt;
            d.x += std::sin( in.m_time * 1.7f + fi ) * 1.1f * in.m_dt;
            d.z += std::cos( in.m_time * 1.3f + fi * 0.7f ) * 0.6f * in.m_dt;
        }
        else if( in.m_env.m_dust > std::max( in.m_env.m_rain, in.m_env.m_snow ) )
        { // Sand driven across the road by the wind.
            d.x += ( 13.0f + 4.0f * std::sin( in.m_time * 0.7f + static_cast<float>( i ) * 0.3f ) ) * in.m_dt;
            d.y += std::sin( in.m_time * 2.1f + static_cast<float>( i ) ) * 0.8f * in.m_dt;
        }
        else if( in.m_env.m_snow > in.m_env.m_rain )
        {
            d.y -= 1.6f * in.m_dt;
            d.x += std::sin( in.m_time * 1.3f + static_cast<float>( i ) ) * 0.5f * in.m_dt;
        }
        else
        {
            d.y -= 19.0f * in.m_dt;
        }
        Vector3 rel = Vector3Subtract( d, boxCenter );
        if( rel.x > 30 ) d.x -= 60; else if( rel.x < -30 ) d.x += 60;
        if( rel.z > 30 ) d.z -= 60; else if( rel.z < -30 ) d.z += 60;
        if( d.y < -6 ) d.y += 22; else if( d.y > 16 ) d.y -= 22;
        if( in.m_env.m_dust > 0.02f && d.y > 7.0f ) d.y -= 13.0f;   // blowing sand stays low
    }
    const Vector3 shaftCenter = Vector3Scale( flatFwd, 30.0f );
    for( Vector3 &d : m_shafts )
    {
        d = Vector3Subtract( d, delta );
        d.y = -3.0f;
        Vector3 rel = Vector3Subtract( d, shaftCenter );
        if( rel.x > 45 ) d.x -= 90; else if( rel.x < -45 ) d.x += 90;
        if( rel.z > 70 ) d.z -= 140; else if( rel.z < -70 ) d.z += 140;
    }
    const Vector3 streakCenter = Vector3Scale( flatFwd, 22.0f );
    for( Vector3 &d : m_streaks )
    {
        d = Vector3Subtract( d, delta );
        Vector3 rel = Vector3Subtract( d, streakCenter );
        if( rel.x > 14 ) d.x -= 28; else if( rel.x < -14 ) d.x += 28;
        if( rel.z > 40 ) d.z -= 80; else if( rel.z < -40 ) d.z += 80;
    }

    const Vec3d origin = cam.m_pos;
    drawShadows( in, cam, origin );

    // Where the sun stands on screen, for the lens flare.
    m_flare = 0.0f;
    const Vector3 sun = sunDirection( in.m_env );
    if( Vector3DotProduct( sun, Vector3Normalize( cam.m_forward ) ) > 0.2f && in.m_env.m_sunIntensity > 0.5f && in.m_env.m_eclipse < 0.5f )
    {
        Camera3D c3{};
        c3.position = Vector3{ 0, 0, 0 };
        c3.target = cam.m_forward;
        c3.up = cam.m_up;
        c3.fovy = cam.m_fovY;
        c3.projection = CAMERA_PERSPECTIVE;
        const Vector2 p = GetWorldToScreenEx( Vector3Scale( sun, 500.0f ), c3, m_targetW, m_targetH );
        m_sunUv = Vector2{ p.x / static_cast<float>( m_targetW ), 1.0f - p.y / static_cast<float>( m_targetH ) };
        if( m_sunUv.x > -0.1f && m_sunUv.x < 1.1f && m_sunUv.y > -0.1f && m_sunUv.y < 1.1f )
            m_flare = std::min( 1.0f, in.m_env.m_sunIntensity ) * ( 1.0f - in.m_env.m_cloudAmount * 0.6f );
    }

    BeginTextureMode( m_scene );
    ClearBackground( BLACK );
    drawSky( in, cam );
    drawWorld( in, cam, origin );
    EndTextureMode();
    post( in );
}

void Renderer::drawShadows( const SceneInput &in, const CameraView &cam, const Vec3d &origin )
{
    const Environment &e = in.m_env;
    const Vector3      sun = sunDirection( e );
    m_shadowStrength = in.m_shadows && sun.y > 0.07f && e.m_sunIntensity > 0.4f && e.m_eclipse < 0.5f
                           ? clampf( ( e.m_sunIntensity - 0.4f ) * 1.5f, 0.0f, 0.85f ) * ( 1.0f - e.m_cloudAmount * 0.5f )
                           : 0.0f;
    if( m_shadowStrength <= 0.0f || !in.m_track || ( !in.m_world && !in.m_showcase ) ) return;
    auto pieceMatrix = [&]( const ScenePiece &p )
    {
        const Vector3 o = relativeTo( p.m_pos, origin );
        return MatrixMultiply( MatrixRotateY( p.m_yaw ), MatrixTranslate( o.x, o.y, o.z ) );
    };

    // The box follows the camera, snapped to the map's texels so the shadows do not shimmer.
    const Vector3 flatFwd = Vector3Normalize( Vector3{ cam.m_forward.x, 0.0f, cam.m_forward.z } );
    Vector3       center = in.m_showcase ? relativeTo( in.m_focus, origin ) : Vector3Scale( flatFwd, ShadowExtent * 0.3f );
    const Vector3 lightFwd = Vector3Negate( sun );
    const Vector3 lightRight = Vector3Normalize( Vector3CrossProduct( lightFwd, Vector3{ 0, 1, 0 } ) );
    const Vector3 lightUp = Vector3CrossProduct( lightRight, lightFwd );
    const double  texel = ShadowExtent / ShadowSize;
    const double  wx = origin.m_x + center.x, wy = origin.m_y + center.y, wz = origin.m_z + center.z;
    const double  cr = wx * lightRight.x + wy * lightRight.y + wz * lightRight.z;
    const double  cu = wx * lightUp.x + wy * lightUp.y + wz * lightUp.z;
    const float   dr = static_cast<float>( std::round( cr / texel ) * texel - cr );
    const float   du = static_cast<float>( std::round( cu / texel ) * texel - cu );
    center = Vector3Add( center, Vector3Add( Vector3Scale( lightRight, dr ), Vector3Scale( lightUp, du ) ) );

    Camera3D light{};
    light.position = Vector3Add( center, Vector3Scale( sun, 300.0f ) );
    light.target = center;
    light.up = Vector3{ 0, 1, 0 };
    light.fovy = ShadowExtent;
    light.projection = CAMERA_ORTHOGRAPHIC;
    rlSetClipPlanes( 1.0, 700.0 );
    BeginTextureMode( m_shadowMap );
    rlClearScreenBuffers();
    BeginMode3D( light );
    m_lightVP = MatrixMultiply( rlGetMatrixModelview(), rlGetMatrixProjection() );
    rlDisableBackfaceCulling();
    const Track &track = *in.m_track;
    const double playerS = in.m_player ? in.m_player->m_s : 0.0;
    if( in.m_world )
        for( const Chunk &c : in.m_world->chunks() )
        {
            const double c0 = static_cast<double>( c.m_index ) * ChunkLength;
            if( c0 > playerS + ShadowExtent || c0 + ChunkLength < playerS - 60.0 || !c.m_props.vertexCount ) continue;
            const Vector3 o = relativeTo( c.m_origin, origin );
            DrawMesh( c.m_props, m_depthMaterial, MatrixTranslate( o.x, o.y, o.z ) );
        }
    for( const ScenePiece &p : in.m_pieces )
        if( p.m_mesh && p.m_mesh->vertexCount ) DrawMesh( *p.m_mesh, m_depthMaterial, pieceMatrix( p ) );
    if( in.m_traffic )
        for( const TrafficCar &t : in.m_traffic->m_cars )
            if( std::abs( t.m_s - playerS ) < ShadowExtent )
            {
                Pose p = roadPose( track, t.m_s, t.m_x, t.m_yaw );
                p.m_pos.m_y += static_cast<double>( t.m_lift );
                DrawMesh( in.m_models->m_traffic[t.m_model].m_mesh, m_depthMaterial, poseMatrix( p, origin ) );
            }
    for( const CarDraw &c : in.m_cars )
        if( c.m_alpha >= 1.0f && std::abs( c.m_s - playerS ) < ShadowExtent )
            DrawMesh( in.m_models->m_playerCars[c.m_car][c.m_paint], m_depthMaterial, poseMatrix( roadPose( track, c.m_s, c.m_x, c.m_yaw ), origin ) );
    if( in.m_player )
        DrawMesh( in.m_models->m_livery.vaoId ? in.m_models->m_livery : in.m_models->m_playerCars[in.m_car][in.m_paint], m_depthMaterial,
                  poseMatrix( in.m_player->pose( track ), origin ) );
    rlEnableBackfaceCulling();
    EndMode3D();
    EndTextureMode();
}

void Renderer::drawSky( const SceneInput &in, const CameraView &cam )
{
    const Environment &e = in.m_env;
    const Shader      &s = m_sky.m_shader;
    const Vector3      fwd = Vector3Normalize( cam.m_forward );
    const Vector3      right = Vector3Normalize( Vector3CrossProduct( fwd, cam.m_up ) );
    const Vector3      up = Vector3CrossProduct( right, fwd );
    BeginShaderMode( s );
    setV2( s, "resolution", static_cast<float>( m_targetW ), static_cast<float>( m_targetH ) );
    setV3( s, "camRight", right );
    setV3( s, "camUp", up );
    setV3( s, "camForward", fwd );
    setF( s, "tanHalfFov", std::tan( cam.m_fovY * 0.5f * DEG2RAD ) );
    setF( s, "aspect", static_cast<float>( m_targetW ) / static_cast<float>( m_targetH ) );
    setV3( s, "zenith", linearRgb( e.m_skyZenith ) );
    setV3( s, "horizon", linearRgb( e.m_skyHorizon ) );
    setV3( s, "fogColor", linearRgb( e.m_fogColor ) );
    setV3( s, "sunDir", sunDirection( e ) );
    setV3( s, "sunColor", Vector3Scale( linearRgb( e.m_sunColor ), std::min( 1.0f, e.m_sunIntensity ) ) );
    setF( s, "sunRadius", e.m_sunSize * DEG2RAD );
    setF( s, "sunStripes", e.m_sunStripes );
    setV3( s, "mountNear", linearRgb( e.m_mountainNear ) );
    setV3( s, "mountFar", linearRgb( e.m_mountainFar ) );
    setF( s, "mountHeight", e.m_mountainHeight );
    setF( s, "mountSeed", in.m_track ? static_cast<float>( in.m_track->seed() % 97 ) * 0.37f : 1.0f );
    setF( s, "clouds", e.m_cloudAmount );
    setV3( s, "cloudColor", linearRgb( e.m_cloudColor ) );
    setF( s, "stars", e.m_stars );
    setF( s, "time", in.m_time );
    setF( s, "flash", in.m_flash );
    const float d = static_cast<float>( ViewDistance );
    setF( s, "horizonDrop", std::atan( ( Bend * d * d + 3.0f ) / d ) );
    setF( s, "aurora", e.m_aurora );
    setF( s, "rainbow", e.m_rainbow );
    setF( s, "eclipse", e.m_eclipse );
    const float pe = e.m_planetElevation * DEG2RAD, pa = e.m_planetAzimuth * DEG2RAD;
    setV3( s, "planetDir", Vector3{ -std::sin( pa ) * std::cos( pe ), std::sin( pe ), std::cos( pa ) * std::cos( pe ) } );
    setF( s, "planetRadius", e.m_planetSize * DEG2RAD );
    setV3( s, "planetColor", linearRgb( e.m_planetColor ) );
    DrawRectangle( 0, 0, m_targetW, m_targetH, WHITE );
    EndShaderMode();
}

void Renderer::drawWorld( const SceneInput &in, const CameraView &cam, const Vec3d &origin )
{
    const Environment &e = in.m_env;
    const Models      &models = *in.m_models;
    const Track       &track = *in.m_track;
    const Shader      &s = m_world.m_shader;

    Camera3D c3{};
    c3.position = Vector3{ 0, 0, 0 };
    c3.target = cam.m_forward;
    c3.up = cam.m_up;
    c3.fovy = cam.m_fovY;
    c3.projection = CAMERA_PERSPECTIVE;
    rlSetClipPlanes( NearPlane, FarPlane );
    BeginMode3D( c3 );
    rlDisableBackfaceCulling();

    setV3( s, "camPos", Vector3{ 0, 0, 0 } );
    setF( s, "bend", Bend );
    setV3( s, "sunDir", sunDirection( e ) );
    setV3( s, "sunColor", Vector3Scale( linearRgb( e.m_sunColor ), e.m_sunIntensity ) );
    setV3( s, "ambTop", linearRgb( e.m_ambientTop ) );
    setV3( s, "ambBottom", linearRgb( e.m_ambientBottom ) );
    setV3( s, "fogColor", linearRgb( e.m_fogColor ) );
    setF( s, "fogDensity", e.m_fogDensity );
    setF( s, "viewDist", static_cast<float>( ViewDistance ) - 40.0f );
    setV3( s, "skyColor", Vector3Lerp( linearRgb( e.m_skyHorizon ), linearRgb( e.m_skyZenith ), 0.3f ) );
    setF( s, "wetness", std::max( e.m_rain, e.m_snow * 0.4f ) );
    setF( s, "flash", in.m_flash * 0.8f );
    setF( s, "gridGlow", in.m_surface.m_gridGlow );
    setF( s, "time", std::fmod( in.m_time, 1000.0f ) );
    setF( s, "wind", 1.0f + in.m_env.m_rain * 1.5f + in.m_env.m_lightning * 0.8f + in.m_env.m_snow * 0.5f );
    setV2( s, "swayOffset", static_cast<float>( std::fmod( origin.m_x, 450.0 ) ), static_cast<float>( std::fmod( origin.m_z, 450.0 ) ) );
    setF( s, "objectShade", 1.0f );
    setF( s, "objectAlpha", 1.0f );
    setF( s, "shadowStrength", m_shadowStrength );
    setF( s, "shadowTexel", 1.0f / static_cast<float>( ShadowSize ) );
    SetShaderValueMatrix( s, GetShaderLocation( s, "lightVP" ), m_lightVP );
    {
        const int slot = ShadowSlot;
        rlActiveTextureSlot( ShadowSlot );
        rlEnableTexture( m_shadowMap.depth.id );
        rlActiveTextureSlot( 0 );
        SetShaderValue( s, GetShaderLocation( s, "shadowMap" ), &slot, SHADER_UNIFORM_INT );
    }
    setV3( s, "gridColor", Vector3{ 0.02f, 0.75f, 1.0f } );
    setV2( s, "gridOffset", static_cast<float>( std::fmod( origin.m_x, 12.0 ) ), static_cast<float>( std::fmod( origin.m_z, 12.0 ) ) );
    Pose playerPose{};
    if( in.m_player )
    {
        playerPose = in.m_player->pose( track );
        const Vector3 pos = relativeTo( playerPose.m_pos, origin );
        setV3( s, "headPos", Vector3Add( Vector3Add( pos, Vector3Scale( playerPose.m_forward, 2.3f ) ), Vector3Scale( playerPose.m_up, 0.7f ) ) );
        setV3( s, "headDir", Vector3Normalize( Vector3Subtract( playerPose.m_forward, Vector3Scale( playerPose.m_up, 0.07f ) ) ) );
        setF( s, "headStrength", std::max( e.m_headlights, inTunnel( track, in.m_player->m_s ) ) );
    }
    else
    {
        setF( s, "headStrength", 0.0f );
    }

    // The chunks.
    if( in.m_world )
        for( const Chunk &c : in.m_world->chunks() )
        {
            const Vector3 o = relativeTo( c.m_origin, origin );
            const Matrix  t = MatrixTranslate( o.x, o.y, o.z );
            if( c.m_ground.vertexCount ) DrawMesh( c.m_ground, m_material, t );
            if( c.m_props.vertexCount ) DrawMesh( c.m_props, m_material, t );
        }
    // Lone models (the landmark viewer, a crossing).
    for( const ScenePiece &p : in.m_pieces )
    {
        if( !p.m_mesh || !p.m_mesh->vertexCount ) continue;
        const Vector3 o = relativeTo( p.m_pos, origin );
        DrawMesh( *p.m_mesh, m_material, MatrixMultiply( MatrixRotateY( p.m_yaw ), MatrixTranslate( o.x, o.y, o.z ) ) );
    }

    // Vehicles and cones.
    const double playerS = in.m_player ? in.m_player->m_s : cam.m_pos.m_z;
    struct Shadow
    {
        Pose  m_pose;
        float m_w, m_l;
    };
    std::vector<Shadow> shadows;
    if( in.m_traffic )
    {
        for( const TrafficCar &c : in.m_traffic->m_cars )
        {
            if( c.m_s - playerS > ViewDistance ) continue;
            const float wobble = c.m_hitTimer > 0.0f ? std::sin( c.m_hitTimer * 25.0f ) * 0.08f * c.m_hitTimer : 0.0f;
            Pose        p = roadPose( track, c.m_s, c.m_x, c.m_yaw + wobble );
            p.m_pos.m_y += static_cast<double>( c.m_lift );
            setF( s, "objectShade", 1.0f - 0.8f * inTunnel( track, c.m_s ) );
            DrawMesh( models.m_traffic[c.m_model].m_mesh, m_material, poseMatrix( p, origin ) );
            if( !c.m_animal && c.m_s > playerS - 10.0 && c.m_s - playerS < 90.0 && c.m_plate < models.m_plates.size() )
            {
                // Its number plate behind, in the country's style.
                Pose plate = p;
                const float back = c.m_length * 0.5f + 0.02f;
                plate.m_pos.m_x -= static_cast<double>( p.m_forward.x * back - p.m_up.x * 0.5f );
                plate.m_pos.m_y -= static_cast<double>( p.m_forward.y * back - p.m_up.y * 0.5f );
                plate.m_pos.m_z -= static_cast<double>( p.m_forward.z * back - p.m_up.z * 0.5f );
                DrawMesh( models.m_plates[c.m_plate], m_material, poseMatrix( plate, origin ) );
            }
            if( c.m_lift < 0.5f && !c.m_swim ) shadows.push_back( { p, c.m_width, c.m_length } );
        }
        for( const Obstacle &o : in.m_traffic->m_obstacles )
        {
            const Mesh &mesh = o.m_kind == 0 ? models.m_cone : models.m_arrowBoard[o.m_kind == 1 ? 0 : 1];
            if( o.m_knocked )
            {
                const Vector3 rel = relativeTo( o.m_pos, origin );
                const Matrix  m = MatrixMultiply( MatrixRotate( o.m_axis, o.m_angle ), MatrixTranslate( rel.x, rel.y, rel.z ) );
                DrawMesh( mesh, m_material, m );
            }
            else
            {
                DrawMesh( mesh, m_material, poseMatrix( roadPose( track, o.m_s, o.m_x, 0.0f ), origin ) );
            }
        }
    }
    for( const CarDraw &c : in.m_cars )
    {
        if( c.m_alpha < 1.0f || c.m_s - playerS > ViewDistance ) continue;
        const Pose p = roadPose( track, c.m_s, c.m_x, c.m_yaw );
        setF( s, "objectShade", 1.0f - 0.8f * inTunnel( track, c.m_s ) );
        DrawMesh( models.m_playerCars[c.m_car][c.m_paint], m_material, poseMatrix( p, origin ) );
        shadows.push_back( { p, carSpec( c.m_car ).m_width, carSpec( c.m_car ).m_length } );
    }
    if( in.m_player && in.m_drawPlayer )
    {
        setF( s, "objectShade", 1.0f - 0.8f * inTunnel( track, in.m_player->m_s ) );
        DrawMesh( models.m_livery.vaoId ? models.m_livery : models.m_playerCars[in.m_car][in.m_paint], m_material, poseMatrix( playerPose, origin ) );
    }
    setF( s, "objectShade", 1.0f );
    if( in.m_player )
    {
        // The shadow stays on the road while the car flies, and shrinks.
        const PlayerCar &pc = *in.m_player;
        Pose             p = roadPose( track, pc.m_s, pc.m_x, pc.m_yawVisual + pc.m_spin );
        const float      h = static_cast<float>( pc.m_y - p.m_pos.m_y );
        const float      k = 1.0f / ( 1.0f + std::max( 0.0f, h ) * 0.2f );
        shadows.push_back( { p, pc.m_spec.m_width * k, pc.m_spec.m_length * k } );
    }

    rlDisableDepthMask();
    for( const Shadow &sh : shadows )
    {
        const Vector3 pos = Vector3Add( relativeTo( sh.m_pose.m_pos, origin ), Vector3Scale( sh.m_pose.m_up, 0.06f ) );
        const Matrix  m = basisMatrix( Vector3Scale( sh.m_pose.m_left, sh.m_w * 0.98f ), sh.m_pose.m_up,
                                       Vector3Scale( sh.m_pose.m_forward, sh.m_l * 0.92f ), pos );
        DrawMesh( models.m_shadow, m_material, m );
    }
    // Ghosts: see-through, after everything solid.
    for( const CarDraw &c : in.m_cars )
    {
        if( c.m_alpha >= 1.0f ) continue;
        setF( s, "objectAlpha", c.m_alpha );
        setF( s, "objectShade", 1.0f );
        DrawMesh( models.m_playerCars[c.m_car][c.m_paint], m_material, poseMatrix( roadPose( track, c.m_s, c.m_x, c.m_yaw ), origin ) );
    }
    // The glass of undersea tunnels: tinted panes, shiny, seen through.
    setF( s, "objectAlpha", 0.24f );
    setF( s, "objectShade", 1.0f );
    if( in.m_world )
        for( const Chunk &c : in.m_world->chunks() )
        {
            if( !c.m_glass.vertexCount ) continue;
            const Vector3 o = relativeTo( c.m_origin, origin );
            DrawMesh( c.m_glass, m_material, MatrixTranslate( o.x, o.y, o.z ) );
        }
    setF( s, "objectAlpha", 1.0f );
    rlEnableDepthMask();

    drawParticles( in, cam, origin );
    rlEnableBackfaceCulling();
    EndMode3D();
    rlActiveTextureSlot( ShadowSlot );
    rlDisableTexture();
    rlActiveTextureSlot( 0 );
}

void Renderer::drawParticles( const SceneInput &in, const CameraView &cam, const Vec3d &origin )
{
    const Shader     &s = m_particle.m_shader;
    const Environment &e = in.m_env;
    const Vector3     fwd = Vector3Normalize( cam.m_forward );
    const Vector3     right = Vector3Normalize( Vector3CrossProduct( fwd, cam.m_up ) );
    const Vector3     up = Vector3CrossProduct( right, fwd );

    auto billboard = [&]( Vector3 c, float size, Color col )
    {
        const Vector3 r = Vector3Scale( right, size ), u = Vector3Scale( up, size );
        quad( Vector3Subtract( Vector3Subtract( c, r ), u ), Vector3Add( Vector3Subtract( c, r ), u ),
              Vector3Add( Vector3Add( c, r ), u ), Vector3Subtract( Vector3Add( c, r ), u ), col );
    };
    auto ribbon = [&]( Vector3 a, Vector3 b, float width, Color col )
    {
        const Vector3 dir = Vector3Subtract( b, a );
        Vector3       side = Vector3CrossProduct( dir, a );
        const float   len = Vector3Length( side );
        if( len < 1e-6f ) return;
        side = Vector3Scale( side, width * 0.5f / len );
        quad( Vector3Add( a, side ), Vector3Subtract( a, side ), Vector3Subtract( b, side ), Vector3Add( b, side ), col );
    };

    rlDisableDepthMask();
    BeginShaderMode( s );
    setV3( s, "camPos", Vector3{ 0, 0, 0 } );
    setV3( s, "fogColor", linearRgb( e.m_fogColor ) );
    setF( s, "fogDensity", e.m_fogDensity );
    rlSetTexture( rlGetTextureIdDefault() );

    // Smoke and dust.
    setF( s, "intensity", 1.0f );
    setF( s, "roundSprite", 1.0f );
    rlBegin( RL_QUADS );
    if( in.m_particles )
    {
        for( const Particle &p : in.m_particles->items() )
        {
            if( p.m_glow > 1.0f ) continue;
            const float t = p.m_life / p.m_maxLife;
            Color       c = p.m_color;
            c.a = static_cast<unsigned char>( static_cast<float>( c.a ) * ( 1.0f - t ) * std::min( 1.0f, p.m_life * 8.0f ) );
            billboard( relativeTo( p.m_pos, origin ), p.m_size, c );
        }
    }
    rlEnd();
    rlDrawRenderBatchActive();

    // Sparks and flames: bright, additive.
    BeginBlendMode( BLEND_ADDITIVE );
    setF( s, "intensity", 6.0f );
    rlBegin( RL_QUADS );
    if( in.m_particles )
    {
        for( const Particle &p : in.m_particles->items() )
        {
            if( p.m_glow <= 1.0f ) continue;
            const float   t = p.m_life / p.m_maxLife;
            Color         c = p.m_color;
            c.a = static_cast<unsigned char>( static_cast<float>( c.a ) * ( 1.0f - t ) );
            const Vector3 pos = relativeTo( p.m_pos, origin );
            if( p.m_streak )
                ribbon( pos, Vector3Subtract( pos, Vector3Scale( p.m_vel, 0.03f ) ), p.m_size, c );
            else
                billboard( pos, p.m_size, c );
        }
    }
    rlEnd();
    rlDrawRenderBatchActive();
    EndBlendMode();

    // Rain, snow or blowing sand, and air streaks at speed; under the sea shafts of light, its snow and bubbles.
    const float rain = e.m_rain, snow = e.m_snow;
    if( e.m_underwater > 0.3f )
    {
        // Shafts from the surface, a little slanted, brightest above, flickering with the waves.
        BeginBlendMode( BLEND_ADDITIVE );
        setF( s, "intensity", 1.0f );
        setF( s, "roundSprite", 1.0f );
        rlBegin( RL_QUADS );
        const Vector3 slant = Vector3Normalize( Vector3{ 0.3f, -1.0f, 0.15f } );
        for( size_t i = 0; i < m_shafts.size(); ++i )
        {
            const float   fi = static_cast<float>( i );
            const float   flicker = 0.55f + 0.45f * std::sin( in.m_time * ( 0.5f + 0.07f * fi ) + fi * 1.7f );
            const float   a = 60.0f * e.m_underwater * flicker;
            const Vector3 foot = m_shafts[i], top = Vector3Subtract( foot, Vector3Scale( slant, 40.0f ) );
            const Vector3 dir = Vector3Subtract( foot, top );
            Vector3       side = Vector3CrossProduct( dir, top );
            const float   len = Vector3Length( side );
            if( len < 1e-6f ) continue;
            side = Vector3Scale( side, ( 2.0f + 1.3f * static_cast<float>( i % 4 ) ) / len );
            quad2( Vector3Add( top, side ), Vector3Subtract( top, side ), Vector3Subtract( foot, side ), Vector3Add( foot, side ),
                   Color{ 190, 236, 255, static_cast<unsigned char>( a ) }, Color{ 150, 220, 240, static_cast<unsigned char>( a * 0.35f ) } );
        }
        rlEnd();
        rlDrawRenderBatchActive();
        EndBlendMode();
        // Bubbles and the sea's snow, outside the tube only.
        setF( s, "intensity", 1.2f );
        rlBegin( RL_QUADS );
        const Vector3 across = Vector3Normalize( Vector3CrossProduct( fwd, Vector3{ 0, 1, 0 } ) );
        const int     n = static_cast<int>( static_cast<float>( m_drops.size() ) * 0.7f * e.m_underwater );
        for( int i = 0; i < n; ++i )
        {
            const Vector3 p = m_drops[static_cast<size_t>( i )];
            if( std::abs( Vector3DotProduct( p, across ) ) < 8.5f && p.y < 7.0f ) continue;
            if( i % 4 == 0 ) billboard( p, 0.05f + 0.03f * static_cast<float>( i % 3 ), Color{ 215, 245, 255, 160 } );
            else billboard( p, 0.03f, Color{ 190, 214, 220, 100 } );
        }
        rlEnd();
        rlDrawRenderBatchActive();
    }
    else if( std::max( { e.m_leaves, e.m_petals, e.m_confetti } ) > std::max( 0.02f, std::max( rain, snow ) ) )
    {
        // Autumn leaves in their colours, blossom petals, or a festival's confetti.
        const bool  leaves = e.m_leaves >= e.m_petals;
        Color       colors[4] = { leaves ? Color{ 214, 120, 40, 230 } : Color{ 250, 196, 214, 220 }, leaves ? Color{ 190, 62, 40, 230 } : Color{ 250, 244, 240, 220 },
                                  leaves ? Color{ 226, 182, 60, 230 } : Color{ 244, 176, 200, 220 }, leaves ? Color{ 150, 100, 52, 230 } : Color{ 252, 210, 224, 220 } };
        const bool  confetti = e.m_confetti > std::max( e.m_leaves, e.m_petals ) && e.m_festival >= 0 && e.m_festival < static_cast<int>( festivals().size() );
        if( confetti )
        {
            const auto &palette = festivals()[static_cast<size_t>( e.m_festival )].m_confetti;
            for( size_t k = 0; k < 4 && !palette.empty(); ++k ) colors[k] = palette[k % palette.size()];
        }
        setF( s, "intensity", 1.2f );
        setF( s, "roundSprite", leaves ? 0.0f : 1.0f );
        rlBegin( RL_QUADS );
        const int n = static_cast<int>( static_cast<float>( m_drops.size() ) * 0.35f * std::max( { e.m_leaves, e.m_petals, e.m_confetti } ) );
        for( int i = 0; i < n; ++i ) billboard( m_drops[static_cast<size_t>( i )], leaves ? 0.07f : 0.05f, colors[i & 3] );
        rlEnd();
        rlDrawRenderBatchActive();
    }
    else if( e.m_dust > std::max( 0.02f, std::max( rain, snow ) ) )
    {
        setF( s, "intensity", 1.1f );
        setF( s, "roundSprite", 0.0f );
        rlBegin( RL_QUADS );
        const int     n = static_cast<int>( static_cast<float>( m_drops.size() ) * e.m_dust );
        const Vector3 blow = Vector3Subtract( Vector3{ 14.0f, 0.0f, 0.0f }, m_camVel );
        for( int i = 0; i < n; ++i )
        {
            const Vector3 p = m_drops[static_cast<size_t>( i )];
            ribbon( p, Vector3Subtract( p, Vector3Scale( blow, 0.035f ) ), 0.04f, Color{ 222, 178, 120, 120 } );
        }
        rlEnd();
        rlDrawRenderBatchActive();
    }
    else if( rain > 0.02f || snow > 0.02f )
    {
        const bool snowing = snow > rain;
        setF( s, "intensity", snowing ? 1.4f : 1.2f );
        setF( s, "roundSprite", snowing ? 1.0f : 0.0f );
        rlBegin( RL_QUADS );
        const int     n = static_cast<int>( static_cast<float>( m_drops.size() ) * ( snowing ? snow * 0.7f : rain ) );
        const Vector3 fall = snowing ? Vector3{ 0, -1.6f, 0 } : Vector3{ 0, -19.0f, 0 };
        const Vector3 rel = Vector3Subtract( fall, m_camVel );
        for( int i = 0; i < n; ++i )
        {
            const Vector3 p = m_drops[static_cast<size_t>( i )];
            if( snowing )
                billboard( p, 0.07f, Color{ 245, 248, 255, 210 } );
            else
                ribbon( p, Vector3Subtract( p, Vector3Scale( rel, 0.03f ) ), 0.025f, Color{ 190, 200, 220, 110 } );
        }
        rlEnd();
        rlDrawRenderBatchActive();
    }
    if( in.m_windStreaks > 0.01f )
    {
        setF( s, "intensity", 1.6f );
        setF( s, "roundSprite", 0.0f );
        rlBegin( RL_QUADS );
        const unsigned char a = static_cast<unsigned char>( 80.0f * std::min( 1.0f, in.m_windStreaks ) );
        for( const Vector3 &p : m_streaks )
        {
            if( std::abs( p.x ) < 2.0f && std::abs( p.z ) < 2.0f ) continue;
            ribbon( p, Vector3Subtract( p, Vector3Scale( m_camVel, 0.05f ) ), 0.03f, Color{ 230, 240, 255, a } );
        }
        rlEnd();
        rlDrawRenderBatchActive();
    }
    rlSetTexture( 0 );
    EndShaderMode();
    rlEnableDepthMask();
}

void Renderer::post( const SceneInput &in )
{
    const Environment &e = in.m_env;
    const int          bw = m_bloomA.texture.width, bh = m_bloomA.texture.height;
    const int          cw = m_bloomC.texture.width, ch = m_bloomC.texture.height;

    BeginTextureMode( m_bloomA );
    BeginShaderMode( m_bright.m_shader );
    setV2( m_bright.m_shader, "texel", 1.0f / static_cast<float>( m_targetW ) * 2.0f, 1.0f / static_cast<float>( m_targetH ) * 2.0f );
    setF( m_bright.m_shader, "threshold", 1.0f );
    blit( m_scene.texture, bw, bh );
    EndShaderMode();
    EndTextureMode();

    auto blurPass = [&]( RenderTexture2D &src, RenderTexture2D &dst, float dx, float dy )
    {
        BeginTextureMode( dst );
        BeginShaderMode( m_blur.m_shader );
        setV2( m_blur.m_shader, "dir", dx / static_cast<float>( src.texture.width ), dy / static_cast<float>( src.texture.height ) );
        blit( src.texture, dst.texture.width, dst.texture.height );
        EndShaderMode();
        EndTextureMode();
    };
    for( float k : { 1.0f, 2.0f } )
    {
        blurPass( m_bloomA, m_bloomB, k, 0.0f );
        blurPass( m_bloomB, m_bloomA, 0.0f, k );
    }
    BeginTextureMode( m_bloomC );
    blit( m_bloomA.texture, cw, ch );
    EndTextureMode();
    for( float k : { 1.0f, 2.0f } )
    {
        blurPass( m_bloomC, m_bloomD, k, 0.0f );
        blurPass( m_bloomD, m_bloomC, 0.0f, k );
    }

    const Shader &c = m_composite.m_shader;
    BeginTextureMode( m_ldr );
    BeginShaderMode( c );
    SetShaderValueTexture( c, GetShaderLocation( c, "bloom1" ), m_bloomA.texture );
    SetShaderValueTexture( c, GetShaderLocation( c, "bloom2" ), m_bloomC.texture );
    setF( c, "bloomStrength", e.m_bloom * 0.45f );
    setF( c, "exposure", e.m_exposure );
    setF( c, "saturation", e.m_saturation );
    setF( c, "vignette", 0.32f );
    setF( c, "blurAmount", in.m_speedBlur );
    setF( c, "aberration", in.m_aberration );
    setV2( c, "blurCenter", 0.5f, 0.52f );
    setV2( c, "sunUv", m_sunUv.x, m_sunUv.y );
    setF( c, "flare", m_flare );
    setF( c, "aspect", static_cast<float>( m_targetW ) / static_cast<float>( m_targetH ) );
    setF( c, "lensRain", in.m_lensRain );
    setF( c, "speed", clampf( m_camVel.x * m_camVel.x + m_camVel.z * m_camVel.z, 0.0f, 6400.0f ) / 6400.0f );
    setF( c, "time", std::fmod( in.m_time, 100.0f ) );
    blit( m_scene.texture, m_targetW, m_targetH );
    EndShaderMode();
    EndTextureMode();

    BeginShaderMode( m_fxaa.m_shader );
    setV2( m_fxaa.m_shader, "texel", 1.0f / static_cast<float>( m_targetW ), 1.0f / static_cast<float>( m_targetH ) );
    blit( m_ldr.texture, m_screenW, m_screenH );
    EndShaderMode();
}

} // namespace sunstreak
