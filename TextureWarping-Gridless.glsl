
-- Quad.VS

layout(location = 0) in vec3 Position;
out vec2 vTexCoord;

void main()
{
    vTexCoord = Position.xy;
    gl_Position = vec4(Position, 1);
}

-- Quad.FS

in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D Sampler;

const bool BlackBackground = true;
const bool BlackBorder = false;

void main()
{
    vec2 p = vTexCoord;
    float theta  = atan(p.y,p.x);
    float radius = length(p);
    radius = radius * radius;
    p.x = radius * cos(theta);
    p.y = radius * sin(theta);
    vec2 tc = 0.5 * (p + 1.0);

    vec2 q = 1-abs(p);
    float u = fwidth(q.x);
    float v = fwidth(q.y);
    float L = 1.0;

    if (BlackBackground) {
        if (q.x < -u || q.y < -v) {
            FragColor = vec4(0);
            return;
        }
        if (q.x < u) L *= (q.x/u);
        if (q.y < v) L *= (q.y/v);
    }

    if (BlackBorder) {
        if (q.x < -u || q.y < -v) {
            FragColor = vec4(1);
            return;
        }
        if (q.x < u) L *= abs(q.x/u);
        if (q.y < v) L *= abs(q.y/v);
    }

    FragColor = L * texture(Sampler, tc);
}

-- Simple.VS

layout(location = 0) in vec4 Position;
uniform mat4 ModelviewProjection[7];

void main()
{
    gl_Position = ModelviewProjection[gl_InstanceID] * Position;
}


-- Simple.FS

out vec4 FragColor;
uniform vec4 Color;
void main()
{
    FragColor = Color;
}

-- Lit.VS

layout(location = 0) in vec4 Position;
out vec3 vPosition;
out int vInstanceID;
uniform mat4 ModelviewProjection[7];

void main()
{
    vInstanceID = gl_InstanceID;
    vPosition = Position.xyz;
    gl_Position = ModelviewProjection[gl_InstanceID] * Position;
}


-- Lit.GS

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 vPosition[3];
out vec3 gNormal;
in int vInstanceID[3];
out float gDistance[4];
flat out int gInstanceID;

void main()
{
    gInstanceID = vInstanceID[0];
    vec3 A = vPosition[2] - vPosition[0];
    vec3 B = vPosition[1] - vPosition[0];
    gNormal = normalize(cross(A, B));

    for (int j = 0; j < 3; j++) {
        gl_Position = gl_in[j].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

-- Lit.FS

in vec3 gNormal;
flat in int gInstanceID;
out vec4 FragColor;
uniform vec3 AmbientMaterial = vec3(0.2, 0.2, 0.2);
uniform vec3 SpecularMaterial = vec3(0.5, 0.5, 0.5);
uniform vec4 FrontMaterial = vec4(0.75, 0.75, 0.5, 0.5);
uniform vec4 BackMaterial = vec4(0.75, 0.75, 0.5, 0.5);
uniform float Shininess = 7;
uniform vec3 Hhat[7];
uniform vec3 Lhat[7];

void main()
{
    vec3 N = -normalize(gNormal);
    if (!gl_FrontFacing)
       N = -N;

    float df = max(0.0, dot(N, Lhat[gInstanceID]));
    float sf = max(0.0, dot(N, Hhat[gInstanceID]));
    sf = pow(sf, Shininess);

    vec3 diffuse = gl_FrontFacing ? FrontMaterial.rgb : BackMaterial.rgb;
    vec3 lighting = AmbientMaterial + df * diffuse;
    if (gl_FrontFacing)
        lighting += sf * SpecularMaterial;

    FragColor = vec4(lighting, FrontMaterial.a);
}
