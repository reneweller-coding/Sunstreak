/**
 * @file Shaders.h
 * @brief The GLSL 330 sources: world, sky, particles and the post-processing chain.
 *
 * The world is lit in linear space into a half-float target; bloom, tone mapping, the radial speed blur, grading and
 * FXAA follow. The world shader bends the ground down with distance (a small planet) the way modern takes on the
 * pseudo-3D racers do, which shows more of the road ahead over crests.
 *
 * @author reneweller
 */
#pragma once

namespace sunstreak::shaders
{

/** @brief World vertex shader: model transform, the planet bend. */
inline const char *WorldVs = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec2 vertexTexCoord2;
in mat4 instanceTransform;
uniform mat4 matModel;
uniform mat4 matView;
uniform mat4 matProjection;
uniform float instanced;
uniform vec3 camPos;
uniform float bend;
uniform float time;
uniform float wind;
uniform vec2 swayOffset;
uniform float objectShade;
out vec3 vPos;
out vec3 vNormal;
out vec4 vColor;
out vec2 vMat;
out float vShade;
out vec3 vFlat;
void main()
{
    mat4 model = instanced > 0.5 ? instanceTransform : matModel;
    vec4 wp = model * vec4(vertexPosition, 1.0);
    float sway = vertexTexCoord2.y;
    if (sway > 0.0)
    {
        vec2 wpos = wp.xz + swayOffset;
        float ph = time * 1.7 + dot(wpos, vec2(0.069813, 0.055851));
        wp.x += sway * wind * (sin(ph) * 0.25 + sin(ph * 2.3 + 1.0) * 0.08);
        wp.z += sway * wind * cos(ph * 0.8) * 0.18;
    }
    vShade = vertexTexCoord2.x * objectShade;
    vFlat = wp.xyz;
    vec2 d = wp.xz - camPos.xz;
    wp.y -= bend * dot(d, d);
    vPos = wp.xyz;
    vNormal = mat3(model) * vertexNormal;
    vColor = vertexColor;
    vMat = vertexTexCoord;
    gl_Position = matProjection * matView * wp;
}
)";

/** @brief World fragment shader: sun, ambient, headlights, gloss, emissive, neon grid, fog. */
inline const char *WorldFs = R"(#version 330
in vec3 vPos;
in vec3 vNormal;
in vec4 vColor;
in vec2 vMat;
in float vShade;
in vec3 vFlat;
uniform mat4 lightVP;
uniform sampler2D shadowMap;
uniform float shadowStrength;
uniform float shadowTexel;
uniform vec3 camPos;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform vec3 ambTop;
uniform vec3 ambBottom;
uniform vec3 fogColor;
uniform float fogDensity;
uniform float viewDist;
uniform vec3 skyColor;
uniform vec3 headPos;
uniform vec3 headDir;
uniform float headStrength;
uniform float wetness;
uniform float slush;
uniform float groundFog;
uniform float writeMirror;
uniform float flash;
uniform float gridGlow;
uniform vec3 gridColor;
uniform vec2 gridOffset;
uniform float objectAlpha;
out vec4 finalColor;
float sunShadow(vec3 n)
{
    if (shadowStrength <= 0.0) return 1.0;
    vec4 lp = lightVP * vec4(vFlat, 1.0);
    vec3 p = lp.xyz / lp.w * 0.5 + 0.5;
    float edge = max(abs(p.x - 0.5), abs(p.y - 0.5)) * 2.0;
    if (edge > 1.0 || p.z > 1.0) return 1.0;
    float bias = 0.0008 + 0.002 * (1.0 - max(dot(n, sunDir), 0.0));
    float lit = 0.0;
    for (int x = -1; x <= 1; x++)
        for (int y = -1; y <= 1; y++)
            lit += (p.z - bias > texture(shadowMap, p.xy + vec2(x, y) * shadowTexel).r) ? 0.0 : 1.0;
    lit /= 9.0;
    return mix(1.0, lit, shadowStrength * (1.0 - smoothstep(0.8, 1.0, edge)));
}
void main()
{
    vec3 n = normalize(vNormal);
    if (!gl_FrontFacing) n = -n;
    vec3 base = pow(vColor.rgb, vec3(2.2));
    float cobbles = step(vMat.x, -1.5);
    float ground = step(vMat.x, -0.5) * (1.0 - cobbles);
    float emissive = max(vMat.x, 0.0);
    float gloss = vMat.y;
    vec3 toCam = camPos - vPos;
    float dist = length(toCam);
    vec3 V = toCam / max(dist, 0.001);

    float shade = vShade;
    float ndl = max(dot(n, sunDir), 0.0) * sunShadow(n);
    vec3 light = mix(ambBottom, ambTop, n.y * 0.5 + 0.5) * (0.25 + 0.75 * shade) + (sunColor * ndl + vec3(flash)) * shade;
    light += ambTop * 0.55 * max(dot(n, V), 0.0) * (0.3 + 0.7 * shade);

    vec3 toP = vPos - headPos;
    float hd = length(toP);
    vec3 hdir = toP / max(hd, 0.001);
    float cone = smoothstep(0.86, 0.975, dot(hdir, headDir));
    float hl = headStrength * cone * max(dot(n, -hdir), 0.0) * 70.0 / (25.0 + hd * hd * 0.07);
    light += vec3(1.0, 0.92, 0.8) * hl;

    vec3 col = base * light;
    float roadLike = step(0.1, gloss) * step(gloss, 0.35);
    float slushy = 0.0;
    if (slush > 0.0 && roadLike > 0.5)
    {
        // Slush: grey-white blotches, repeating every 12 m like the grid's offset.
        vec2 p = (vPos.xz + gridOffset) * 0.5236;
        float n = 0.5 + 0.25 * (sin(p.x * 2.0 + 2.0 * sin(p.y)) + sin(p.y * 3.0 + p.x));
        slushy = smoothstep(0.42, 0.72, n) * slush;
        col = mix(col, pow(vec3(0.74, 0.73, 0.7), vec3(2.2)) * light, slushy * 0.75);
    }
    float g = clamp(gloss + wetness * 0.55 * roadLike, 0.0, 1.0);
    vec3 H = normalize(sunDir + V);
    float spec = pow(max(dot(n, H), 0.0), mix(12.0, 180.0, g)) * g * 2.0;
    col += sunColor * spec * shade * sunShadow(n);
    float fres = pow(1.0 - max(dot(n, V), 0.0), 4.0) * g * (0.2 + 0.8 * shade);
    col = mix(col, skyColor, clamp(fres, 0.0, 0.85));
    col += vec3(1.0, 0.9, 0.75) * hl * g * 0.6;
    col += base * emissive * 3.5;

    if (cobbles > 0.5)
    {
        // Cobblestones in staggered rows, their joints dark; repeating every 12 m like the grid's offset.
        vec2 q = (vPos.xz + gridOffset) * vec2(2.5, 3.0);
        q.x += mod(floor(q.y), 2.0) * 0.5;
        vec2 f = fract(q) - 0.5;
        vec2 w = fwidth(q);
        float joint = smoothstep(0.34, 0.5, max(abs(f.x), abs(f.y)));
        vec2 cell = mod(floor(q), vec2(30.0, 36.0));
        float stone = fract(sin(dot(cell, vec2(12.9898, 78.233))) * 43758.5453);
        float fade = 1.0 - smoothstep(0.25, 0.6, max(w.x, w.y));
        col *= mix(1.0, (1.0 - 0.5 * joint) * (0.82 + 0.32 * stone), fade);
    }
    if (gridGlow > 0.0 && ground > 0.5)
    {
        vec2 gp = (vPos.xz + gridOffset) / 12.0;
        vec2 gw = fwidth(gp);
        vec2 f = abs(fract(gp - 0.5) - 0.5) / max(gw, vec2(0.0001));
        float line = 1.0 - clamp(min(f.x, f.y) - 0.6, 0.0, 1.0);
        line *= 1.0 - smoothstep(180.0, 650.0, dist);
        col += gridColor * line * gridGlow * 1.3;
        emissive = max(emissive, line * gridGlow);
    }

    float fog = 1.0 - exp(-pow(dist * fogDensity, 1.6));
    fog = max(fog, smoothstep(viewDist * 0.7, viewDist, dist));
    fog *= 1.0 - 0.55 * clamp(emissive, 0.0, 1.0) * (1.0 - smoothstep(viewDist * 0.5, viewDist, dist));
    col = mix(col, fogColor, clamp(fog, 0.0, 1.0));
    if (groundFog > 0.0)
    {
        // Fog lying low: thick below the road's height, thin above it, none close by.
        float low = 1.0 - smoothstep(camPos.y - 5.0, camPos.y + 10.0, vPos.y);
        float lf = groundFog * low * smoothstep(12.0, 160.0, dist);
        col = mix(col, fogColor * 1.06, clamp(lf, 0.0, 0.92));
        fog = max(fog, lf);
    }
    float alpha = vColor.a * objectAlpha;
    if (writeMirror > 0.5)
    {
        // The ground pass: alpha is 1 less how much the surface mirrors (the wet road, a puddle), for the mirror pass.
        float mirror = wetness * roadLike * 0.42 * (1.0 - slushy) + step(0.97, gloss) * 0.8;
        alpha = 1.0 - clamp(mirror * (1.0 - clamp(fog, 0.0, 1.0)), 0.0, 0.9);
    }
    finalColor = vec4(col, alpha);
}
)";

/** @brief Shadow pass vertex shader: depth from the sun. */
inline const char *DepthVs = R"(#version 330
in vec3 vertexPosition;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

/** @brief Shadow pass fragment shader: nothing but depth. */
inline const char *DepthFs = R"(#version 330
out vec4 finalColor;
void main()
{
    finalColor = vec4(1.0);
}
)";

/** @brief Full-screen pass vertex shader (raylib's 2D batch). */
inline const char *ScreenVs = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
out vec4 fragColor;
void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

/** @brief Sky: gradient, sun (striped in the neon biome), stars, clouds, two mountain ranges. */
inline const char *SkyFs = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
uniform vec2 resolution;
uniform vec3 camRight;
uniform vec3 camUp;
uniform vec3 camForward;
uniform float tanHalfFov;
uniform float aspect;
uniform vec3 zenith;
uniform vec3 horizon;
uniform vec3 fogColor;
uniform vec3 sunDir;
uniform vec3 sunColor;
uniform float sunRadius;
uniform float sunStripes;
uniform vec3 mountNear;
uniform vec3 mountFar;
uniform float mountHeight;
uniform float mountSeed;
uniform float clouds;
uniform vec3 cloudColor;
uniform float stars;
uniform float time;
uniform float flash;
uniform float horizonDrop;
uniform float aurora;
uniform float rainbow;
uniform float eclipse;
uniform vec3 planetDir;
uniform float planetRadius;
uniform vec3 planetColor;
out vec4 finalColor;

float hash(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
float noise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), f.x), mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), f.x), f.y);
}
float fbm(vec2 p)
{
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 5; i++) { v += a * noise(p); p *= 2.03; a *= 0.5; }
    return v;
}
float ridge(float az, float seed)
{
    float h = 0.50 * sin(az * 3.0 + seed) + 0.30 * sin(az * 7.0 + seed * 1.7) + 0.18 * sin(az * 13.0 + seed * 2.3)
            + 0.10 * abs(sin(az * 29.0 + seed * 3.1)) + 0.06 * abs(sin(az * 61.0 + seed * 4.7));
    return h;
}
void main()
{
    vec2 uv = (gl_FragCoord.xy / resolution) * 2.0 - 1.0;
    vec3 dir = normalize(camForward + camRight * uv.x * tanHalfFov * aspect + camUp * uv.y * tanHalfFov);
    float el = asin(clamp(dir.y, -1.0, 1.0));
    float az = atan(dir.x, dir.z);
    float up = max(dir.y, 0.0);
    vec3 col = mix(horizon, zenith, pow(up, 0.5));

    if (stars > 0.0)
    {
        vec2 sp = vec2(az * 160.0, el * 160.0);
        float h = hash(floor(sp));
        float tw = 0.6 + 0.4 * sin(time * 3.0 + h * 50.0);
        col += vec3(step(0.995, h) * stars * tw * smoothstep(0.03, 0.25, up)) * 2.0;
    }

    if (aurora > 0.0 && dir.y > 0.02)
    {
        vec2 p = dir.xz / (dir.y + 0.25);
        float band = fbm(vec2(p.x * 0.9 + time * 0.02, p.y * 0.35 - time * 0.01));
        float curtain = smoothstep(0.45, 0.7, band) * smoothstep(0.02, 0.15, dir.y) * (1.0 - smoothstep(0.55, 0.95, up));
        float rays = 0.65 + 0.35 * sin(p.x * 30.0 + band * 12.0 + time * 0.5);
        vec3 ac = mix(vec3(0.05, 1.0, 0.45), vec3(0.75, 0.2, 1.0), smoothstep(0.25, 0.6, up));
        col += ac * curtain * rays * aurora * 1.4;
    }

    if (planetRadius > 0.0)
    {
        float pa = acos(clamp(dot(dir, planetDir), -1.0, 1.0));
        if (pa < planetRadius)
        {
            vec3 t1 = normalize(cross(planetDir, vec3(0.0001, 1.0, 0.0)));
            vec3 t2 = cross(t1, planetDir);
            vec2 q = vec2(dot(dir - planetDir, t1), dot(dir - planetDir, t2)) / sin(planetRadius);
            float h = sqrt(max(0.0, 1.0 - dot(q, q)));
            vec3 n = normalize(t1 * q.x + t2 * q.y - planetDir * h);
            float lit = max(dot(n, sunDir), 0.0);
            float cl = smoothstep(0.55, 0.75, fbm(q * 3.0 + 7.0));
            vec3 pc = mix(planetColor, vec3(1.0), cl * 0.7) * (lit * 1.3 + 0.03);
            col = mix(col, pc, 1.0 - smoothstep(planetRadius * 0.97, planetRadius, pa));
        }
    }

    float sd = dot(dir, sunDir);
    float sunEl = asin(clamp(sunDir.y, -1.0, 1.0));
    float ang = acos(clamp(sd, -1.0, 1.0));
    float disk = 1.0 - smoothstep(sunRadius * 0.97, sunRadius * 1.03, ang);
    float v = (el - sunEl) / sunRadius;
    if (sunStripes > 0.0 && v < 0.15)
    {
        float gap = step(fract(v * 4.5 + 0.1), 0.22 + 0.45 * clamp(-v, 0.0, 1.0));
        disk *= 1.0 - gap * sunStripes;
    }
    vec3 sunCol = sunColor * mix(vec3(1.0), mix(vec3(1.4, 0.35, 0.8), vec3(1.1, 1.25, 0.35), clamp(v * 0.5 + 0.5, 0.0, 1.0)), sunStripes);
    float glow = pow(max(sd, 0.0), 900.0) * 1.5 + pow(max(sd, 0.0), 60.0) * 0.35 + pow(max(sd, 0.0), 6.0) * 0.12;

    if (clouds > 0.0 && dir.y > 0.0)
    {
        vec2 p = dir.xz / (dir.y + 0.08) * 0.7 + vec2(time * 0.006, time * 0.002);
        float c = fbm(p * 1.4);
        float cover = 1.0 - clouds * 0.8;
        float dens = smoothstep(cover - 0.15, cover + 0.2, c) * smoothstep(0.0, 0.12, up);
        vec3 cc = cloudColor * (0.75 + 0.5 * pow(max(sd, 0.0), 4.0)) + flash * vec3(0.8);
        col = mix(col, cc, dens * 0.92);
        disk *= 1.0 - dens * 0.85;
        glow *= 1.0 - dens * 0.6;
    }
    float corona = 0.0;
    if (eclipse > 0.0)
    {
        vec3 c1 = normalize(cross(sunDir, vec3(0.0001, 1.0, 0.0)));
        vec3 c2 = cross(c1, sunDir);
        float phi = atan(dot(dir, c2), dot(dir, c1));
        float rr = ang / sunRadius;
        float streamers = 0.65 + 0.35 * sin(phi * 7.0 + 1.3) * sin(phi * 3.0);
        corona = step(1.0, rr) * exp(-(rr - 1.0) * 2.2) * streamers + exp(-(rr - 1.0) * 14.0) * step(1.0, rr);
        glow *= 1.0 - 0.9 * eclipse;
    }
    col += sunColor * glow;
    col = mix(col, mix(sunCol * 14.0, vec3(0.0), eclipse), disk);
    col += vec3(1.0, 0.97, 0.9) * corona * 2.6 * eclipse;

    if (rainbow > 0.0)
    {
        float ra = degrees(acos(clamp(dot(dir, -sunDir), -1.0, 1.0)));
        float t = (ra - 40.0) / 3.0;
        if (t > 0.0 && t < 1.0)
        {
            float hue = (1.0 - t) * 0.78;
            vec3 rc = clamp(abs(fract(hue + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0) - 1.0, 0.0, 1.0);
            col += rc * sin(t * 3.14159) * rainbow * 0.45 * smoothstep(-0.02, 0.06, dir.y);
        }
    }

    float base = -horizonDrop;
    float farTop = base + radians(mountHeight * (0.75 + 0.45 * ridge(az, mountSeed)));
    float nearTop = base + radians(mountHeight * (0.42 + 0.32 * ridge(az * 1.0, mountSeed + 5.0)));
    if (el < farTop)
    {
        float k = clamp((farTop - el) / max(farTop - base, 0.001), 0.0, 1.0);
        col = mix(mountFar, fogColor, 0.25 + 0.5 * k);
    }
    if (el < nearTop)
    {
        float k = clamp((nearTop - el) / max(nearTop - base, 0.001), 0.0, 1.0);
        col = mix(mountNear, fogColor, 0.1 + 0.6 * k);
    }
    if (el < base) col = fogColor;
    col += flash * vec3(0.55, 0.6, 0.75) * (0.4 + 0.6 * up);
    finalColor = vec4(col, 1.0);
}
)";

/** @brief Particle vertex shader (raylib's batch, in world space). */
inline const char *ParticleVs = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
uniform mat4 mvp;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 wpos;
void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    wpos = vertexPosition;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
)";

/** @brief Particle fragment shader: round soft sprites or streaks, fogged, optionally above 1 for bloom. */
inline const char *ParticleFs = R"(#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 wpos;
uniform float intensity;
uniform float roundSprite;
uniform vec3 camPos;
uniform vec3 fogColor;
uniform float fogDensity;
out vec4 finalColor;
void main()
{
    vec2 d = fragTexCoord * 2.0 - 1.0;
    float a = roundSprite > 0.5 ? clamp(1.0 - dot(d, d), 0.0, 1.0) : 1.0 - abs(d.x);
    vec3 c = pow(fragColor.rgb, vec3(2.2)) * intensity;
    float dist = length(wpos - camPos);
    float fog = 1.0 - exp(-pow(dist * fogDensity, 1.6));
    c = mix(c, fogColor, fog * 0.8);
    finalColor = vec4(c, fragColor.a * a);
}
)";

/** @brief Bright pass with a 4-tap downsample. */
inline const char *BrightFs = R"(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 texel;
uniform float threshold;
out vec4 finalColor;
void main()
{
    vec3 c = texture(texture0, fragTexCoord + texel * vec2(-0.5, -0.5)).rgb;
    c += texture(texture0, fragTexCoord + texel * vec2(0.5, -0.5)).rgb;
    c += texture(texture0, fragTexCoord + texel * vec2(-0.5, 0.5)).rgb;
    c += texture(texture0, fragTexCoord + texel * vec2(0.5, 0.5)).rgb;
    c *= 0.25;
    float l = max(c.r, max(c.g, c.b));
    float k = smoothstep(threshold, threshold * 2.5, l);
    finalColor = vec4(min(c * k, vec3(40.0)), 1.0);
}
)";

/**
 * @brief Reflections: where the ground mirrors (alpha below 1), up the picture to the foot of what stands on it, then as
 *        far again, a little blurred and rippled; mixed in as strongly as the surface mirrors.
 */
inline const char *MirrorFs = R"(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform float time;
uniform float aspect;
out vec4 finalColor;
void main()
{
    vec2 uv = fragTexCoord;
    vec4 here = texture(texture0, uv);
    float mirror = 1.0 - here.a;
    if (mirror < 0.01)
    {
        finalColor = vec4(here.rgb, 1.0);
        return;
    }
    float foot = -1.0;
    float stepY = 0.009;
    for (int i = 1; i <= 44; i++)
    {
        float y = uv.y + float(i) * stepY;
        if (y >= 1.0) break;
        if (texture(texture0, vec2(uv.x, y)).a > 0.99)
        {
            foot = y;
            break;
        }
    }
    vec3 col = here.rgb;
    if (foot > 0.0)
    {
        // Close in on the foot.
        float lo = foot - stepY, hi = foot;
        for (int k = 0; k < 4; k++)
        {
            float m = 0.5 * (lo + hi);
            if (texture(texture0, vec2(uv.x, m)).a > 0.99) hi = m; else lo = m;
        }
        float ry = hi + (hi - uv.y);
        if (ry < 0.995)
        {
            float wob = sin(uv.y * 500.0 + time * 5.0) * 0.0012 + sin(uv.x * 300.0 * aspect - time * 3.0) * 0.0008;
            vec2 r = vec2(uv.x + wob, ry);
            vec3 refl = texture(texture0, r).rgb * 0.5 + texture(texture0, r + vec2(0.0, 0.004)).rgb * 0.25 +
                        texture(texture0, r - vec2(0.0, 0.004)).rgb * 0.25;
            float fade = smoothstep(0.995, 0.9, ry) * smoothstep(0.0, 0.03, hi - uv.y);
            col = mix(here.rgb, refl * 0.8, mirror * fade);
        }
    }
    finalColor = vec4(col, 1.0);
}
)";

/** @brief Separable Gaussian blur (9 taps through 5 bilinear reads). */
inline const char *BlurFs = R"(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 dir;
out vec4 finalColor;
void main()
{
    vec3 c = texture(texture0, fragTexCoord).rgb * 0.227027;
    c += (texture(texture0, fragTexCoord + dir * 1.3846).rgb + texture(texture0, fragTexCoord - dir * 1.3846).rgb) * 0.316216;
    c += (texture(texture0, fragTexCoord + dir * 3.2308).rgb + texture(texture0, fragTexCoord - dir * 3.2308).rgb) * 0.070270;
    finalColor = vec4(c, 1.0);
}
)";

/** @brief Composite: radial speed blur, aberration, bloom, exposure, ACES, grading, vignette, grain. */
inline const char *CompositeFs = R"(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform sampler2D bloom1;
uniform sampler2D bloom2;
uniform float bloomStrength;
uniform float exposure;
uniform float saturation;
uniform float vignette;
uniform float blurAmount;
uniform float aberration;
uniform vec2 blurCenter;
uniform float time;
uniform vec2 sunUv;
uniform float flare;
uniform float aspect;
uniform float lensRain;
uniform float speed;
uniform float heat;
out vec4 finalColor;
float hash12(vec2 p) { return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
vec2 rainOffset(vec2 uv)
{
    vec2 grid = vec2(14.0, 8.0);
    vec2 cell = floor(uv * grid);
    vec2 f = fract(uv * grid);
    float h = hash12(cell);
    float h2 = hash12(cell + 7.13);
    float life = fract(time * (0.25 + h * 0.3) + h2);
    vec2 c = vec2(0.2 + 0.6 * h, 0.25 + 0.5 * h2 + life * 0.2 * speed);
    vec2 d = (f - c) * vec2(1.0, 0.75);
    float r = 0.1 + 0.12 * h;
    float m = smoothstep(r, r * 0.6, length(d)) * smoothstep(1.0, 0.6, life) * step(0.45, hash12(cell + 3.3));
    return d * m * 0.7 / grid;
}
vec3 aces(vec3 x)
{
    const float a = 2.51; const float b = 0.03; const float c = 2.43; const float d = 0.59; const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}
void main()
{
    vec2 uv = fragTexCoord;
    if (lensRain > 0.0) uv += rainOffset(uv) * lensRain;
    if (heat > 0.0)
    {
        // The air shimmering over the hot road, strongest in a band at the horizon.
        float band = smoothstep(0.36, 0.48, uv.y) * (1.0 - smoothstep(0.52, 0.64, uv.y));
        uv.x += sin(uv.y * 520.0 + time * 9.0) * 0.0013 * heat * band;
        uv.y += sin(uv.x * 310.0 + time * 6.0) * 0.0007 * heat * band;
    }
    vec2 d = uv - blurCenter;
    float edge = smoothstep(0.12, 0.65, length(d * vec2(1.6, 1.0)));
    vec3 col;
    float strength = blurAmount * edge * 0.07;
    if (strength > 0.0005)
    {
        vec3 acc = vec3(0.0);
        float wsum = 0.0;
        for (int i = 0; i < 10; i++)
        {
            float t = float(i) / 9.0;
            float w = 1.0 - t * 0.6;
            acc += texture(texture0, uv - d * t * strength).rgb * w;
            wsum += w;
        }
        col = acc / wsum;
    }
    else
    {
        col = texture(texture0, uv).rgb;
    }
    if (aberration > 0.0)
    {
        vec2 off = d * aberration * 0.0045 * edge;
        col.r = texture(texture0, uv + off).r;
        col.b = texture(texture0, uv - off).b;
    }
    col += (texture(bloom1, uv).rgb * 0.55 + texture(bloom2, uv).rgb * 0.8) * bloomStrength;
    if (flare > 0.0)
    {
        // Ghosts along the line through the centre, as bright as the sun is visible.
        vec3 sc = texture(texture0, sunUv).rgb + texture(texture0, sunUv + vec2(0.004, 0.0)).rgb + texture(texture0, sunUv - vec2(0.004, 0.0)).rgb;
        float vis = smoothstep(3.0, 18.0, max(sc.r, max(sc.g, sc.b)));
        vec2 axis = vec2(0.5) - sunUv;
        const float k[5] = float[](0.45, 0.75, 1.15, 1.5, 1.95);
        const float r[5] = float[](0.025, 0.06, 0.035, 0.09, 0.05);
        const vec3 tint[5] = vec3[](vec3(1.0, 0.7, 0.4), vec3(0.4, 0.8, 1.0), vec3(1.0, 0.5, 0.8), vec3(0.5, 1.0, 0.6), vec3(0.8, 0.6, 1.0));
        for (int i = 0; i < 5; i++)
        {
            vec2 p = sunUv + axis * k[i];
            float dd = length((uv - p) * vec2(aspect, 1.0));
            col += tint[i] * smoothstep(r[i], r[i] * 0.6, dd) * vis * flare * 0.22;
        }
        float dh = length((uv - sunUv) * vec2(aspect, 1.0));
        col += vec3(1.0, 0.85, 0.65) * smoothstep(0.012, 0.0, abs(dh - 0.22)) * vis * flare * 0.12;
    }
    col *= exposure;
    col = aces(col);
    float l = dot(col, vec3(0.2126, 0.7152, 0.0722));
    col = mix(vec3(l), col, saturation);
    col = pow(col, vec3(1.0 / 2.2));
    float v = smoothstep(1.0, 0.35, length(d * vec2(1.0, 0.9)));
    col *= mix(1.0 - vignette, 1.0, v);
    col += (fract(sin(dot(uv * 1000.0 + time, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * 0.014;
    finalColor = vec4(col, 1.0);
}
)";

/** @brief FXAA (the compact console variant). */
inline const char *FxaaFs = R"(#version 330
in vec2 fragTexCoord;
uniform sampler2D texture0;
uniform vec2 texel;
out vec4 finalColor;
float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }
void main()
{
    vec2 uv = fragTexCoord;
    vec3 rgbNW = texture(texture0, uv + vec2(-1.0, -1.0) * texel).rgb;
    vec3 rgbNE = texture(texture0, uv + vec2(1.0, -1.0) * texel).rgb;
    vec3 rgbSW = texture(texture0, uv + vec2(-1.0, 1.0) * texel).rgb;
    vec3 rgbSE = texture(texture0, uv + vec2(1.0, 1.0) * texel).rgb;
    vec3 rgbM = texture(texture0, uv).rgb;
    float lNW = luma(rgbNW); float lNE = luma(rgbNE); float lSW = luma(rgbSW); float lSE = luma(rgbSE); float lM = luma(rgbM);
    float lMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));
    vec2 dir = vec2(-((lNW + lNE) - (lSW + lSE)), ((lNW + lSW) - (lNE + lSE)));
    float reduce = max((lNW + lNE + lSW + lSE) * (0.25 / 8.0), 1.0 / 128.0);
    float rcpMin = 1.0 / (min(abs(dir.x), abs(dir.y)) + reduce);
    dir = clamp(dir * rcpMin, vec2(-8.0), vec2(8.0)) * texel;
    vec3 rgbA = 0.5 * (texture(texture0, uv + dir * (1.0 / 3.0 - 0.5)).rgb + texture(texture0, uv + dir * (2.0 / 3.0 - 0.5)).rgb);
    vec3 rgbB = rgbA * 0.5 + 0.25 * (texture(texture0, uv - dir * 0.5).rgb + texture(texture0, uv + dir * 0.5).rgb);
    float lB = luma(rgbB);
    finalColor = vec4((lB < lMin || lB > lMax) ? rgbA : rgbB, 1.0);
}
)";

} // namespace sunstreak::shaders
