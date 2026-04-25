#version 440

layout(location = 0) in vec2 vTexCoord;
layout(location = 0) out vec4 fragColor;

layout(binding = 1) uniform sampler2D equiTex;

layout(std140, binding = 0) uniform buf {
    mat4 qt_Matrix;
    float qt_Opacity;
    float yaw;
    float pitch;
    float fov;           // vertical FOV in radians (default: π/2 = 90°)
    float aspectRatio;   // viewport width / height
} ubuf;

const float PI = 3.14159265358979323846;
const float HALF_PI = PI / 2.0;

void main()
{
    // Map the item's [0,1]×[0,1] UV to normalized device coords [-1,1]×[-1,1]
    vec2 ndc = vTexCoord * 2.0 - 1.0;

    // Reconstruct the view-space ray direction for this fragment.
    // Qt Quick's vTexCoord has y=0 at the TOP of the item, but in view space
    // positive Y points UP, so we negate ndc.y so the top of the screen
    // corresponds to looking upward.
    float tanHalf = tan(ubuf.fov * 0.5);
    vec3 ray = normalize(vec3(
        ndc.x * ubuf.aspectRatio * tanHalf,
        -ndc.y * tanHalf,
        -1.0   // camera looks down -Z in view space
    ));

    // Apply yaw (rotation around the Y/up axis)
    float cy = cos(ubuf.yaw), sy = sin(ubuf.yaw);
    float rx = ray.x * cy + ray.z * sy;
    float rz = -ray.x * sy + ray.z * cy;
    ray.x = rx;
    ray.z = rz;

    // Apply pitch (rotation around the X/right axis)
    float cp = cos(-ubuf.pitch), sp = sin(-ubuf.pitch);
    float ry  =  ray.y * cp - ray.z * sp;
    float rz2 =  ray.y * sp + ray.z * cp;
    ray.y = ry;
    ray.z = rz2;

    // Convert direction to spherical coordinates.
    // azimuth: angle in the horizontal plane from -Z forward axis; [-π, +π]
    // elevation: angle above/below the horizontal plane; [-π/2, +π/2]
    float azimuth   = atan(ray.x, -ray.z);
    float elevation = asin(clamp(ray.y, -1.0, 1.0));

    // VR180 equirectangular: texture covers azimuth ∈ [-π/2, +π/2] and elevation ∈ [-π/2, +π/2]
    float u = (azimuth   + HALF_PI) / PI;  // [0, 1]
    float v = 0.5 - elevation / PI;        // [0, 1], Y-flipped so v=0 is top

    // Left-eye image occupies the left half of the 8192×4096 SBS texture
    vec2 texCoord = vec2(u * 0.5, v);

    // Sample with clamped coords to avoid bleeding from the right-eye half
    vec4 color = texture(equiTex, clamp(texCoord, vec2(0.001, 0.001), vec2(0.499, 0.999)));

    // Smooth fade to black in the ~15° zone approaching the ±90° azimuth boundary
    // so there is no hard edge when the user approaches the back hemisphere.
    float fadeEdge = smoothstep(1.06, 0.92, u) * smoothstep(-0.06, 0.08, u);

    fragColor = color * fadeEdge * ubuf.qt_Opacity;
}
