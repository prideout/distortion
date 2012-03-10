
-- Lit.VS

in vec4 Position;

out vec3 vPosition;

uniform mat4 Projection;
uniform mat4 Modelview;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

void main()
{
    vPosition = Position.xyz;
    gl_Position = Projection * Modelview * Position;
}


-- Lit.GS

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

in vec3 vPosition[3];

out vec3 gNormal;
out float gDistance[4];

uniform mat3 NormalMatrix;

void main()
{
    vec3 A = vPosition[2] - vPosition[0];
    vec3 B = vPosition[1] - vPosition[0];
    gNormal = NormalMatrix * normalize(cross(A, B));

    for (int j = 0; j < 3; j++) {
        gl_Position = gl_in[j].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}

-- Lit.FS

in vec3 gNormal;

out vec4 FragColor;

uniform vec3 LightPosition = vec3(0.5, 0.25, 1.0);
uniform vec3 AmbientMaterial = vec3(0.2, 0.2, 0.2);
uniform vec3 SpecularMaterial = vec3(0.5, 0.5, 0.5);
uniform vec4 DiffuseMaterial = vec4(0.75, 0.75, 0.5, 0.5);
uniform float Shininess = 7;

void main()
{
    vec3 N = -normalize(gNormal);
    if (!gl_FrontFacing)
       N = -N;

    vec3 L = normalize(LightPosition);
    vec3 Eye = vec3(0, 0, 1);
    vec3 H = normalize(L + Eye);
    
    float df = max(0.0, dot(N, L));

    float sf = max(0.0, dot(N, H));
    sf = pow(sf, Shininess);

    vec3 lighting = AmbientMaterial + df * DiffuseMaterial.rgb;
    if (gl_FrontFacing)
        lighting += sf * SpecularMaterial;

    FragColor = vec4(lighting, DiffuseMaterial.a);
}
