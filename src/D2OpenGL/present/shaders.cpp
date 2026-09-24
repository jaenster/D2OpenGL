// The present shaders, GLSL 1.10 (compatibility profile). The quad arrives in clip space with the game
// image's texture coordinates; uGameSize is the game size, uImageSize the image's size in real pixels.

#include "internal.h"

const char g_szPresentVertexShader[] = R"(#version 110
varying vec2 vTexCoord;
void main()
{
    vTexCoord = gl_MultiTexCoord0.xy;
    gl_Position = gl_Vertex;
}
)";

// Sharp bilinear: the image scaled up by the largest integer factor with nearest neighbour, then to its
// final size with linear filtering. Only the pixel edges are blended; at integer scales it is nearest.
const char g_szPresentSharpShader[] = R"(#version 110
uniform sampler2D uTexture;
uniform vec2 uGameSize;
uniform vec2 uImageSize;
varying vec2 vTexCoord;
void main()
{
    vec2 vScale = max(floor(uImageSize / uGameSize), 1.0);
    vec2 vTexel = vTexCoord * uGameSize;
    vec2 vOffset = fract(vTexel) - 0.5;
    vec2 vRange = 0.5 - 0.5 / vScale;
    vec2 vEdge = (vOffset - clamp(vOffset, -vRange, vRange)) * vScale;
    gl_FragColor = texture2D(uTexture, (floor(vTexel) + 0.5 + vEdge) / uGameSize);
}
)";

// A late-1990s VGA computer monitor, which is subtle next to a television: flat glass, a slightly soft
// horizontal edge, fine scanlines whose beam grows wider with brightness (dark lines show a faint gap,
// bright ones fill it), a faint aperture grille of vertical phosphor stripes and a little halation around
// bright areas. The grille follows the scale: magenta and green stripes one real pixel wide when each game
// pixel is an even number of real ones (a three-colour triad would not repeat per game pixel), red, green
// and blue otherwise. Everything is mixed in linear light and encoded again at the end; the brightness
// the grille and the scanlines take away is given back.
const char g_szPresentCrtShader[] = R"(#version 110
uniform sampler2D uTexture;
uniform vec2 uGameSize;
uniform vec2 uImageSize;
uniform float uScanlines;
uniform float uMask;
uniform float uGlow;
varying vec2 vTexCoord;

vec3 ToLinear(vec3 vColor)
{
    return pow(vColor, vec3(2.2));
}

// Row fRow of the game image at fX: nearest neighbour with a linear edge fScaleX real pixels wide.
vec3 Row(float fX, float fRow, float fScaleX)
{
    float fTexel = fX * uGameSize.x;
    float fOffset = fract(fTexel) - 0.5;
    float fRange = 0.5 - 0.5 / fScaleX;
    float fEdge = (fOffset - clamp(fOffset, -fRange, fRange)) * fScaleX;
    vec2 vUV = vec2((floor(fTexel) + 0.5 + fEdge) / uGameSize.x, (fRow + 0.5) / uGameSize.y);
    return ToLinear(texture2D(uTexture, vUV).rgb);
}

// The intensity of a row's beam at fDist rows from its centre; its width follows the row's brightness.
float Beam(float fDist, vec3 vColor)
{
    float fLuma = dot(vColor, vec3(0.2126, 0.7152, 0.0722));
    float fWidth = mix(0.30, 0.50, sqrt(fLuma));
    return exp(-0.5 * fDist * fDist / (fWidth * fWidth));
}

void main()
{
    vec2 vScale = uImageSize / uGameSize;
    float fScaleX = max(vScale.x / 1.5, 1.0);

    // The two rows around this pixel. With an even number of pixels per row the beam centre is put on a
    // pixel, so that the rows' pixels do not all come out alike.
    float fPerRow = max(floor(vScale.y + 0.01), 1.0);
    float fShift = mod(fPerRow, 2.0) == 0.0 ? 0.5 / fPerRow : 0.0;
    float fY = vTexCoord.y * uGameSize.y - 0.5 - fShift;
    float fRow = floor(fY);
    float fDist = fY - fRow;
    vec3 vRow0 = Row(vTexCoord.x, fRow, fScaleX);
    vec3 vRow1 = Row(vTexCoord.x, fRow + 1.0, fScaleX);
    vec3 vNearest = floor(vTexCoord.y * uGameSize.y) <= fRow ? vRow0 : vRow1;
    vec3 vBeams = vRow0 * Beam(fDist, vRow0) + vRow1 * Beam(1.0 - fDist, vRow1);
    // Scanlines need at least two pixels per row.
    float fScanlines = uScanlines * clamp(vScale.y - 1.0, 0.0, 1.0);
    vec3 vColor = mix(vNearest, vBeams, fScanlines);

    // Aperture grille, in real pixels.
    vec3 vMask = vec3(1.0 - uMask);
    float fMaskAverage;
    if (mod(floor(vScale.x + 0.01), 2.0) == 0.0) {
        if (mod(floor(gl_FragCoord.x), 2.0) < 0.5)
            vMask.rb = vec2(1.0);
        else
            vMask.g = 1.0;
        fMaskAverage = 1.0 - uMask / 2.0;
    } else {
        float fStripe = mod(floor(gl_FragCoord.x), 3.0);
        if (fStripe < 0.5)
            vMask.r = 1.0;
        else if (fStripe < 1.5)
            vMask.g = 1.0;
        else
            vMask.b = 1.0;
        fMaskAverage = 1.0 - uMask * 2.0 / 3.0;
    }
    float fGain = 1.0 / (fMaskAverage * (1.0 - 0.2 * fScanlines));
    vColor *= vMask * fGain;

    // Halation: light spread a couple of pixels around.
    vec2 vTexel = 1.0 / uGameSize;
    vec3 vGlow = ToLinear(texture2D(uTexture, vTexCoord + vec2(1.5, 1.5) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(-1.5, 1.5) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(1.5, -1.5) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(-1.5, -1.5) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(2.5, 0.0) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(-2.5, 0.0) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(0.0, 2.5) * vTexel).rgb);
    vGlow += ToLinear(texture2D(uTexture, vTexCoord + vec2(0.0, -2.5) * vTexel).rgb);
    vColor += uGlow * vGlow / 8.0;

    gl_FragColor = vec4(pow(clamp(vColor, 0.0, 1.0), vec3(1.0 / 2.2)), 1.0);
}
)";
