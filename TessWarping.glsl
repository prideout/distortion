-- Simple.VS

in vec3 Position;
out vec3 vPosition;
out int vInstanceID;

void main()
{
    vInstanceID = gl_InstanceID;
    vPosition = Position;
}

-- Simple.TCS

layout(vertices = 2) out;

in vec3 vPosition[];
out vec3 tcPosition[];

in int vInstanceID[];
out int tcInstanceID[];

uniform float TessLevel;

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcInstanceID[ID] = vInstanceID[ID];
    gl_TessLevelOuter[0] = 1;
    gl_TessLevelOuter[1] = TessLevel;
}

-- Simple.TES

layout(isolines) in;

in vec3 tcPosition[];
in int tcInstanceID[];
uniform mat4 ModelviewProjection[7];

vec4 Distort(vec4 p)
{
    vec2 v = p.xy / p.w;

    // Convert to polar coords:
    float theta  = atan(v.y,v.x);
    float radius = length(v);

    // Distort:
    radius = sqrt(radius);

    // Convert back to Cartesian:
    v.x = radius * cos(theta);
    v.y = radius * sin(theta);
    p.xy = v.xy * p.w;
    return p;
}

void main()
{
    vec3 p0 =        gl_TessCoord.x  * tcPosition[0];
    vec3 p1 = (1.0 - gl_TessCoord.x) * tcPosition[1];
    vec4 p = ModelviewProjection[tcInstanceID[0]] * vec4(p0 + p1, 1);
    gl_Position = Distort(p);
}

-- Simple.FS

out vec4 FragColor;
uniform vec4 Color;
void main()
{
    FragColor = Color;
}

-- Lit.VS

in vec4 Position;
out vec3 vPosition;
out int vInstanceID;

void main()
{
    vInstanceID = gl_InstanceID;
    vPosition = Position.xyz;
}


-- Lit.TCS

layout(vertices = 3) out;

in vec3 vPosition[];
out vec3 tcPosition[];

in int vInstanceID[];
out int tcInstanceID[];

out float tcRadius[];

uniform float TessLevel;

uniform mat4 ModelviewProjection[7];

#define ID gl_InvocationID

void main()
{
    tcPosition[ID] = vPosition[ID];
    tcInstanceID[ID] = vInstanceID[ID];

    vec4 p = ModelviewProjection[vInstanceID[ID]] * vec4(vPosition[ID], 1);
    float r = length(p.xy / p.w);
    tcRadius[ID] = r;

    gl_TessLevelInner[0] = gl_TessLevelOuter[0] =
    gl_TessLevelOuter[1] = gl_TessLevelOuter[2] = TessLevel;
}

-- Lit.TES

layout(triangles, equal_spacing, ccw) in;

in vec3 tcPosition[];
out vec3 tePosition;
in int tcInstanceID[];
out int teInstanceID;
in float tcRadius[];
out float teRadius;
uniform mat4 ModelviewProjection[7];

vec4 Distort(vec4 p)
{
    vec2 v = p.xy / p.w;

    // Convert to polar coords:
    float theta  = atan(v.y,v.x);
    float radius = length(v);

    // Distort:
    radius = sqrt(radius);

    // Convert back to Cartesian:
    v.x = radius * cos(theta);
    v.y = radius * sin(theta);
    p.xy = v.xy * p.w;
    return p;
}

void main()
{
    vec3 p0 = gl_TessCoord.x * tcPosition[0];
    vec3 p1 = gl_TessCoord.y * tcPosition[1];
    vec3 p2 = gl_TessCoord.z * tcPosition[2];
    tePosition = (p0 + p1 + p2);
    teInstanceID = tcInstanceID[0];
    teRadius = (tcRadius[0] + tcRadius[1] * tcRadius[2]) / 3.0;
    vec4 p = ModelviewProjection[teInstanceID] * vec4(tePosition, 1);
    gl_Position = Distort(p);
}

-- Lit.GS

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 tePosition[3];
out vec3 gNormal;
in int teInstanceID[3];
out float gDistance[4];
flat out int gInstanceID;

void main()
{
    gInstanceID = teInstanceID[0];
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
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
