/* common.h helps for code factorization, and 
 * is part of distortion project 
 * Author : Eric Bachard   2019, 24th february
 * License : MIT or Creative Commons by (choose one)
 */

#ifndef __common_h_
#define __common_h_


typedef struct {
    int VertexCount;
    int LineIndexCount;
    int FillIndexCount;
    GLuint LineVao;
    GLuint FillVao;
} MeshPod;


static struct {
    float Theta;
    GLuint LitProgram;
    GLuint SimpleProgram;
    GLuint QuadProgram;
    GLuint GridProgram;
    MeshPod Cylinder;
    Matrix4 Projection;
    Matrix4 View;
    float Power;
    GLuint FboTexture;
    GLuint FboHandle;
    GLuint QuadVao;
    MeshPod Grid;
    float BarrelPower;
} Globals;

#endif  /* __common_h_ */
