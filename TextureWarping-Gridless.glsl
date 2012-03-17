
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

void main()
{
    vec2 tc = vTexCoord;

    float theta  = atan(tc.y,tc.x);
    float radius = length(tc);
    radius = radius * radius;
    tc.x = radius * cos(theta);
    tc.y = radius * sin(theta);
    tc = 0.5 * (tc + 1.0);

    if (tc.s < 0 || tc.s > 1 ||
        tc.t < 0 || tc.t > 1) {
        FragColor = vec4(0, 0, 0, 1);
        return;
    }

    FragColor = texture(Sampler, tc);
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
