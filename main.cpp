/**
 * @file    main.cpp
 * @brief   Main file responsible for implementing and running the 3D engine.
 *
 * The engine uses the OneLoneCoder Pixel Game Engine (olcPGE),
 * specifically optimized for Apple Silicon (M-series) Macs.
 *
 * @author  Loic Martins
 * @date    2026-11-07
 * @version 1.0
 *
 * @par Credits
 *          - Pixel Game Engine by javidx9 (OneLoneCoder), OLC-3 License:
 *          https://github.com/OneLoneCoder/olcPixelGameEngine.
 *          - 3D math/rasterization approach based on javidx9's
 *          "Code-It-Yourself! 3D Graphics Engine" video series.
 *          - Inspired by / based on: Dunn, F., & Parberry, I. (2011). 3D math primer for graphics and game development (2nd ed.).
 * 
 * @copyright Copyright (c) 2026 Loic Martins
 *           Licensed under the MIT License.
 */

#include <cassert>

#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"
#include <vector>
#include <iostream>
#include <sstream>
#include <strstream>
#include <algorithm>

// ============================================================================
// Section: Structures
// ============================================================================

// 2D Vector.
struct vec2d
{ 
    float u = 0; 
    float v = 0;
    float w = 1;
};

// 3D Vector.
struct vec3d
{
    float x = 0; 
    float y = 0;
    float z = 0;
    float w = 1;
};

// Triangle containing 3 vectors (position).
struct triangle {
    vec3d p[3]; // Vertex position coordinates
    vec2d t[3]; // Texture coordinates
    olc::Pixel color;
};

// Mesh 
struct mesh
{
    std::vector<triangle> tris;

    /**
     * @brief   Load the object file.
     *
     * @param[in]  sFilename   Path of the file: .obj.
     * @param[in]  bHasTexture   Boolean if texture is present.
     * @return     True in case of success. Create the mesh.
     *             Create triangles in the mesh.
     *
     */
    bool LoadFromObjectFile(std::string sFilename, bool bHasTexture = false)
    {
        std::ifstream f(sFilename);
        if (!f.is_open())
            return false;

        // Local cache of vertices.
        std::vector<vec3d> vertices;
        std::vector<vec2d> textures;
        std::string line;

        // Loop through each line of the object file.
        while (getline(f, line))
        {
            std::stringstream s(line);
            char junk;

            // Handle the lines storing vertices coordinates.
            if (line[0] == 'v')
            {
                if (line[1] == 't')
                {
                    vec2d v2;
                    s >> junk >> junk >> v2.u >> v2.v >> v2.w;
                    textures.push_back(v2);
                }
                else
                { 
                    vec3d v3;
                    s >> junk >> v3.x >> v3.y >> v3.z;
                    vertices.push_back(v3);
                }
            }

            // Handle the lines storing the indeces for each vertex.
            if (!bHasTexture)
            {
                if (line[0] == 'f')
                {
                    int idxf[3];
                    s >> junk >> idxf[0] >> idxf[1] >> idxf[2];

                    // Create directly the triangle.
                    // Based on the indeces and corresponding vertex coordinates.
                    tris.push_back({ vertices[idxf[0] - 1],
                                    vertices[idxf[1] - 1],
                                    vertices[idxf[2] - 1] });
                }
            }
			else
			{
				if (line[0] == 'f')
				{
					s >> junk;

					std::string tokens[6];
					int nTokenCount = -1;


					while (!s.eof())
					{
						char c = s.get();
						if (c == ' ' || c == '/')
							nTokenCount++;
						else
							tokens[nTokenCount].append(1, c);
					}

					tokens[nTokenCount].pop_back();

					tris.push_back({ vertices[stoi(tokens[0]) - 1], vertices[stoi(tokens[2]) - 1], vertices[stoi(tokens[4]) - 1],
						textures[stoi(tokens[1]) - 1], textures[stoi(tokens[3]) - 1], textures[stoi(tokens[5]) - 1] });

				}

			}
        }
        return true;
    }
};

// Matrix 4x4.
struct mat4x4
{
    float m[4][4] = { 0 };
};


// ============================================================================
// Section: Main Class (olcEngine3D) 
// ============================================================================


class olcEngine3D : public olc::PixelGameEngine
{

    public:
        olcEngine3D()
        {
            sAppName = "3D Demo";
        };

    private:

        // --- Private Data Members ------------------------------------------------------
        mesh meshCube;
        mat4x4 matProj;
        vec3d vCamera;      // Camera position
        vec3d vLookDir;     // Direction the camera is facing
        float fYaw = 0.0f;  // Camera rotation around the Y axis
        float fTheta = 0.0f;

        olc::Sprite *sprTex1;

        // --- Utility Functions ---------------------------------------------------------
        /// @brief Utility Function (Vector Operation): Add two vectors. 
        vec3d Vector_Add(vec3d &v1, vec3d &v2)
        { 
            return { v1.x + v2.x, v1.y + v2.y, v1.z + v2.z };
        }

        /// @brief Utility Function (Vector Operation): Substract two vectors.
        vec3d Vector_Substract(vec3d &v1, vec3d &v2)
        { 
            return { v1.x - v2.x, v1.y - v2.y, v1.z - v2.z };
        }

        /// @brief Utility Function (Vector Operation): Multiply a vector by a scalar.
        vec3d Vector_Multiply(vec3d &v1, float k)
        {
            return { v1.x * k, v1.y * k, v1.z * k };
        }

        /// @brief Utility Function (Vector Operation): Divide vector by a scalar.
        vec3d Vector_Divide(vec3d &v1, float k)
        { 
            return { v1.x / k, v1.y / k, v1.z / k };
        }

        /// @brief Utility Function (Vector Operation): Dot product of two vectors.
        float Vector_DotProduct(vec3d &v1, vec3d &v2)
        { 
            return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
        }

        /// @brief Utility Function (Vector Operation): Get vector length.
        float Vector_Length(vec3d &v)
        { 
            return sqrtf(Vector_DotProduct(v, v));
        }

        /// @brief Utility Function (Vector Operation): Normalize a vector.
        vec3d Vector_Normalise(vec3d &v)
        { 
            float l = Vector_Length(v);
            return { (v.x / l), (v.y / l), (v.z / l) };
        }

        /// @brief Utility Function (Vector Operation): Cross Product of two vectors.
        vec3d Vector_CrossProduct(vec3d &v1, vec3d &v2)
        { 
            vec3d v; 
            v.x = (v1.y * v2.z) - (v1.z * v2.y);
            v.y = (v1.z * v2.x) - (v1.x * v2.z);
            v.z = (v1.x * v2.y) - (v1.y * v2.x);
            return v;
        }

        /// @brief Utility Function (Matrix Operation): Multiply Vector Matrix.
        vec3d Matrix_MultiplyVector(mat4x4 &m, vec3d &i)
        {
            vec3d v; 
            v.x = (i.x * m.m[0][0]) + (i.y * m.m[1][0]) + (i.z * m.m[2][0]) + (i.w * m.m[3][0]);
            v.y = (i.x * m.m[0][1]) + (i.y * m.m[1][1]) + (i.z * m.m[2][1]) + (i.w * m.m[3][1]);
            v.z = (i.x * m.m[0][2]) + (i.y * m.m[1][2]) + (i.z * m.m[2][2]) + (i.w * m.m[3][2]);
            v.w = (i.x * m.m[0][3]) + (i.y * m.m[1][3]) + (i.z * m.m[2][3]) + (i.w * m.m[3][3]);
            return v;
        }

        /// @brief Utility Function (Matrix Operation): Matrix Mutltiplication.
        mat4x4 Matrix_MultiplyMatrix(mat4x4 &m1, mat4x4 &m2)
        {
            mat4x4 matrix;

            for (size_t r = 0; r < 4; r++)
            {
                for (size_t c = 0; c < 4; c++)
                {
                    float sum = 0.0f;
                    for (size_t i = 0; i < 4; i++)
                        sum += m1.m[r][i] * m2.m[i][c];
                    matrix.m[r][c] = sum;
                }
            }

            return matrix;
        }

        /**
         * @brief   Utility Function (Matrix Operation): Matrix Mutltiplication (with specific matrix).
         *
         * Transformation that places an object at position pos and orients it so it faces target.
         * Compute the 3 vectors, right, up, and forward, from that position.
         * 
         */ 
        mat4x4 Matrix_PointAt(vec3d &pos, vec3d &target, vec3d &up)
        {   
            // Forward Vector.
            vec3d newForward = Vector_Substract(target, pos); 
            newForward = Vector_Normalise(newForward); // Normalise to keep only the direction

            // Up vector.
            float dot_upNewforward = Vector_DotProduct(up, newForward); // How much of up points along forward
            vec3d a = Vector_Multiply(newForward, dot_upNewforward); // The part of up that lies along the forward direction
            vec3d newUp = Vector_Substract(up, a); 
            newUp = Vector_Normalise(newUp); 

            // Right Direction.
            vec3d newRight = Vector_CrossProduct(newUp, newForward);

            // Build the matrix based on these vectors.
            mat4x4 matPointAt; 
            matPointAt.m[0][0] = newRight.x;    matPointAt.m[0][1] = newRight.y;    matPointAt.m[0][2] = newRight.z;    matPointAt.m[0][3] = 0;
            matPointAt.m[1][0] = newUp.x;       matPointAt.m[1][1] = newUp.y;       matPointAt.m[1][2] = newUp.z;       matPointAt.m[1][3] = 0;
            matPointAt.m[2][0] = newForward.x;  matPointAt.m[2][1] = newForward.y;  matPointAt.m[2][2] = newForward.z;  matPointAt.m[2][3] = 0;
            matPointAt.m[3][0] = pos.x;         matPointAt.m[3][1] = pos.y;         matPointAt.m[3][2] = pos.z;         matPointAt.m[3][3] = 1;

            return matPointAt;
        }

        /**
         * @brief   Utility Function (Matrix Operation): Matrix Inversion.
         *
         * Only for Rotation/Translation Matrices.
         * 
         */ 
        mat4x4 Matrix_LookAt(mat4x4 &m)
        {   
            mat4x4 matLookAt; 

            matLookAt.m[0][0] = m.m[0][0];    matLookAt.m[0][1] = m.m[1][0];    matLookAt.m[0][2] = m.m[2][0];    matLookAt.m[0][3] = 0.0f;
            matLookAt.m[1][0] = m.m[0][1];    matLookAt.m[1][1] = m.m[1][1];    matLookAt.m[1][2] = m.m[2][1];    matLookAt.m[1][3] = 0.0f;
            matLookAt.m[2][0] = m.m[0][2];    matLookAt.m[2][1] = m.m[1][2];    matLookAt.m[2][2] = m.m[2][2];    matLookAt.m[2][3] = 0.0f;
            
            matLookAt.m[3][0] = -(m.m[3][0] * matLookAt.m[0][0] + m.m[3][1] * matLookAt.m[1][0] + m.m[3][2] * matLookAt.m[2][0]);
            matLookAt.m[3][1] = -(m.m[3][0] * matLookAt.m[0][1] + m.m[3][1] * matLookAt.m[1][1] + m.m[3][2] * matLookAt.m[2][1]);
            matLookAt.m[3][2] = -(m.m[3][0] * matLookAt.m[0][2] + m.m[3][1] * matLookAt.m[1][2] + m.m[3][2] * matLookAt.m[2][2]);
            matLookAt.m[3][3] = 1.0f;

            return matLookAt;
        }        
        
        /// @brief Utility Function (Matrix Generation): Identity matrix.
        mat4x4 Matrix_MakeIdentity()
        { 
            mat4x4 matrix; 
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
            return matrix;
        }

        /// @brief Utility Function (Matrix Generation): Rotation Z.
        mat4x4 Matrix_MakeRotationZ(float fAngleRad)
        { 
            mat4x4 matrix; 
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][1] = sinf(fAngleRad);
            matrix.m[1][0] = -sinf(fAngleRad);
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
            return matrix;
        }

        /// @brief Utility Function (Matrix Generation): Rotation X.
        mat4x4 Matrix_MakeRotationX(float fAngleRad)
        { 
            mat4x4 matrix; 
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = cosf(fAngleRad);
            matrix.m[1][2] = sinf(fAngleRad);
            matrix.m[2][1] = -sinf(fAngleRad);
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;
            return matrix;
        }

        /// @brief Utility Function (Matrix Generation): Rotation Y.
        mat4x4 Matrix_MakeRotationY(float fAngleRad)
        { 
            mat4x4 matrix; 
            matrix.m[0][0] = cosf(fAngleRad);
            matrix.m[0][2] = sinf(fAngleRad);
            matrix.m[2][0] = -sinf(fAngleRad);
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = cosf(fAngleRad);
            matrix.m[3][3] = 1.0f;
            return matrix;
        }

        /// @brief Utility Function (Matrix Generation): Translation.
        mat4x4 Matrix_MakeTranslation(float x, float y, float z)
        { 
            mat4x4 matrix; 
            matrix.m[0][0] = 1.0f;
            matrix.m[1][1] = 1.0f;
            matrix.m[2][2] = 1.0f;
            matrix.m[3][3] = 1.0f;
            matrix.m[3][0] = x;
            matrix.m[3][1] = y;
            matrix.m[3][2] = z;
            return matrix;
        }

        /// @brief Utility Function (Matrix Generation): Projection.
        mat4x4 Matrix_MakeProjection(float fFovDegrees, float fAspectRatio, float fNear, float fFar)
        { 
            float fFovRad = 1.0f / tanf(fFovDegrees * 0.5f / 180.0f * 3.14159f);

            mat4x4 matrix; 
            matrix.m[0][0] = fAspectRatio * fFovRad;
            matrix.m[1][1] = fFovRad;
            matrix.m[2][2] = fFar / (fFar - fNear);
            matrix.m[3][2] = (-fFar * fNear) / (fFar - fNear);
            matrix.m[2][3] = 1.0f;
            matrix.m[3][3] = 0.0f;
            return matrix;
        }

        /**
         * @brief   Utility Function (Vector Operation): Intersection between vector and plane.
         *
         * This function finds the exact point where a line segment crosses a plane.
         *
         * @param[in]  plane_p     Position of the plane.
         * @param[in]  plane_n     Vector normal.
         * @param[in]  lineStart   Start of the line.
         * @param[in]  lineEnd     End of the line.
         * @param[in]  t           Interpolation parameter along the line segment.
         * @return                 Vector of the positions of the intersection point.
         *
         */
        vec3d Vector_IntersectPlane(vec3d &plane_p, vec3d &plane_n, vec3d &lineStart, vec3d &lineEnd, float &t)
        { 
            plane_n = Vector_Normalise(plane_n); 
            float plane_d = - Vector_DotProduct(plane_n, plane_p); // Encodes how far the plane sits from the origin along its normal: d = −n⋅p
            float a_d = Vector_DotProduct(lineStart, plane_n); // The distance measured along the normal (lineStart --> plane)
            float b_d = Vector_DotProduct(lineEnd, plane_n); // The distance measured along the normal (lineEnd --> plane)
            t = ( -plane_d - a_d ) / ( b_d - a_d ); // t represents the fraction of the distance from the start point to the end point at which the line intersects the plane
            vec3d lineStartToEnd = Vector_Substract(lineEnd, lineStart); 
            vec3d lineToIntersect = Vector_Multiply(lineStartToEnd, t); // The whole line from start to end shrinked by the fraction t that reaches the plane
            vec3d intersect_p = Vector_Add(lineStart, lineToIntersect); // Get the actual location of the intersection point
            return intersect_p;
        }

        /**
         * @brief   Utility Function (Vector Operation): Intersection between vector and plane.
         *
         * Return the number of triangles that will be clipped against the plane.
         * 
         */ 
        int Triangle_ClipAgainstPlane(vec3d plane_p, vec3d plane_n, triangle &in_tri, triangle &out_tri1, triangle &out_tri2)
        { 
            plane_n = Vector_Normalise(plane_n);

            /**
             * @brief   Computes the signed distance from a point to a plane.
             *
             * Positive: the point is on the side the normal points toward (the “inside”/keep side for clipping).
             * Zero: the point lies exactly on the plane.
             * Negative: the point is on the opposite side.
             */ 
            auto dist = [&](vec3d &p)
            {
                auto result = plane_n.x * p.x + plane_n.y * p.y + plane_n.z * p.z - Vector_DotProduct(plane_n, plane_p);
                return result;
            };

            // Temporary storage arrays to classify points either side of plane.
            vec3d* inside_points[3]; // Creates an array of 3 pointer variables
            vec3d* outside_points[3]; // Each capable of storing the memory address of a vec3d structure or object
            int nInsidePointCount = 0;
            int nOutsidePointCount = 0;

            vec2d* inside_texture[3]; // Same for the texture.
            vec2d* outside_texture[3]; 
            int nInsideTextureCount = 0;
            int nOutsideTextureCount = 0;

            // Get signed distance of each point in triangle to plan.
            float d0 = dist(in_tri.p[0]);
            float d1 = dist(in_tri.p[1]);
            float d2 = dist(in_tri.p[2]);

            // Check the sign of the distance.
            // If distance side is positive, point lies on inside of the plane.
            if (d0 >= 0) 
            { 
                inside_points[nInsidePointCount++] = &in_tri.p[0]; 
                inside_texture[nInsideTextureCount++] = &in_tri.t[0]; 
            }
            else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[0]; 
                outside_texture[nOutsideTextureCount++] = &in_tri.t[0];
            }

            if (d1 >= 0) 
            { 
                inside_points[nInsidePointCount++] = &in_tri.p[1]; 
                inside_texture[nInsideTextureCount++] = &in_tri.t[1]; 
            }
            else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[1]; 
                outside_texture[nOutsideTextureCount++] = &in_tri.t[1];
            }

            if (d2 >= 0) 
            { 
                inside_points[nInsidePointCount++] = &in_tri.p[2]; 
                inside_texture[nInsideTextureCount++] = &in_tri.t[2]; 
            }
            else { 
                outside_points[nOutsidePointCount++] = &in_tri.p[2]; 
                outside_texture[nOutsideTextureCount++] = &in_tri.t[2];
            }

            // Classify the triangle points.
            // Break the input triangle into smaller output triangles if required.
            // 4 if so, 4 possible outcomes.
            if ( nInsidePointCount == 0 )
            {
                // All points lie on the outside of plane.
                // No valid triangles - Exit the function.
                return 0;
            }
            
            if ( nInsidePointCount == 3 )
            {
                // All points lie inside the plane.
                // Do nothing.
                out_tri1 = in_tri; 
                return 1; 
            }

            if ( nInsidePointCount == 1 && nOutsidePointCount == 2 )
            {   
                // Clip in a smaller triangle.
                // Generate new texture coordinate based on the new verteces.

                // Copy appearance.
                out_tri1.color = in_tri.color; 

                // 1 point inside is valid.
                out_tri1.p[0] = *inside_points[0];
                out_tri1.t[0] = *inside_texture[0];

                // 2 points are outside.
                // The 2 new points are at the intersection between the lines and the plane .
                float t = 0;
                out_tri1.p[1] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);
                out_tri1.t[1].u = t * ( outside_texture[0]->u - inside_texture[0]->u ) + inside_texture[0]->u;
                out_tri1.t[1].v = t * ( outside_texture[0]->v - inside_texture[0]->v ) + inside_texture[0]->v;
                out_tri1.t[1].w = t * ( outside_texture[0]->w - inside_texture[0]->w ) + inside_texture[0]->w;

                out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[1], t);
                out_tri1.t[2].u = t * ( outside_texture[1]->u - inside_texture[0]->u ) + inside_texture[0]->u;
                out_tri1.t[2].v = t * ( outside_texture[1]->v - inside_texture[0]->v ) + inside_texture[0]->v;
                out_tri1.t[2].w = t * ( outside_texture[1]->w - inside_texture[0]->w ) + inside_texture[0]->w;

                return 1;
            }
            
            if ( nInsidePointCount == 2 && nOutsidePointCount == 1 )
            {
                // Clip in a quad, so 2 triangles.

                float t = 0;

                // Copy appearance.
                out_tri1.color = in_tri.color; 
                out_tri2.color = in_tri.color;
                
                // First Triangle: two inside points + 
                // new point determined by the location where one side of the triangle intersects with the plane.
                out_tri1.p[0] = *inside_points[0];
                out_tri1.p[1] = *inside_points[1];
                out_tri1.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[0], *outside_points[0], t);

                out_tri1.t[0] = *inside_texture[0];
                out_tri1.t[1] = *inside_texture[1];
                out_tri1.t[2].u = t * ( outside_texture[0]->u - inside_texture[0]->u ) + inside_texture[0]->u;
                out_tri1.t[2].v = t * ( outside_texture[0]->v - inside_texture[0]->v ) + inside_texture[0]->v;
                out_tri1.t[2].w = t * ( outside_texture[0]->w - inside_texture[0]->w ) + inside_texture[0]->w; 

                // The second triangle is composed of one of the inside point +
                // the last intersection point + 
                // a new intersection point from the other side of the triangle.
                out_tri2.p[0] = *inside_points[1];
                out_tri2.p[1] = out_tri1.p[2];
                out_tri2.p[2] = Vector_IntersectPlane(plane_p, plane_n, *inside_points[1], *outside_points[0], t);

                out_tri2.t[0] = *inside_texture[1];
                out_tri2.t[1] = out_tri1.t[2]; 
                out_tri2.t[2].u = t * ( outside_texture[0]->u - inside_texture[1]->u ) + inside_texture[1]->u;
                out_tri2.t[2].v = t * ( outside_texture[0]->v - inside_texture[1]->v ) + inside_texture[1]->v;
                out_tri2.t[2].w = t * ( outside_texture[0]->w - inside_texture[1]->w ) + inside_texture[1]->w;

                return 2;
            }            
        }
        
        float *pDepthBuffer = nullptr; // Pointer to an array

    public:

        /**
         * @brief   Initialization hook in the olcConsoleGameEngine.
         *
         * Loads or builds meshes.
         * Sets up the projection matrix.
         * Initializes state like the camera position or rotation values.
         */ 
        bool OnUserCreate() override
        {
            pDepthBuffer = new float[ScreenWidth() * ScreenHeight()](); // Every pixel on the screen will have a fp depth number associated

            // Create the mesh from the object file.
            meshCube.LoadFromObjectFile("./assets/world.obj", true);

            // Load the PNG picture.
            sprTex1 = new olc::Sprite();

            // Load the texture for the mesh.
            if (sprTex1->LoadFromFile("./assets/world_texture.png") != olc::rcode::OK)
            {
                // Load failed, the path is wrong.
                std::cout << "texture load failed\n";
                return false;
            }

            // Set the projection Matrix.
            float aspectRatio = (float)ScreenHeight() / (float)ScreenWidth();
            matProj = Matrix_MakeProjection(90.0f, aspectRatio, 0.1f, 1000.0f);

            return true;
        }

        /**
         * @brief   Render a 3D scene on a 2D screen.
         *
         * Handles input (keyboard/mouse to move or rotate the camera).
         * Clears the screen.
         * Updates transforms, building rotation/translation matrices, scaled by fElapsedTime.
         * Runs the render pipeline for each triangle.
         * 
         * Pipeline: Model Space --World Matrix--> World Space --View Matrix--> View Space --Projection Matrix--> Screen Space (2D)
         */ 
        bool OnUserUpdate(float fElapsedTime) override
        {

            // Clear Screen
            Clear(olc::BLACK);

            // Clear depth buffer
            for (int i = 0; i < ScreenWidth() * ScreenHeight(); i++)
                pDepthBuffer[i] = 0.0f;

            // --- User input ---------------------------------------------------------
            // User input to move the camera around the scene.
            // Multiply the movement speed by the elapsed time to calculate the distance the object should move since the last frame.
            // Part1: Moving the camera --slides up, down, left, or right.
            if (GetKey(olc::Key::UP).bHeld)
                vCamera.y += 8.0f * fElapsedTime;

            if (GetKey(olc::Key::DOWN).bHeld)
                vCamera.y -= 8.0f * fElapsedTime;

            if (GetKey(olc::Key::LEFT).bHeld)
                vCamera.x -= 8.0f * fElapsedTime;

            if (GetKey(olc::Key::RIGHT).bHeld)
                vCamera.x += 8.0f * fElapsedTime;

            // Part 2: Turning the camera.
            // W and S: move along the direction the user is looking.
            // A and D: rotate the camera (yaw).
            vec3d vForward = Vector_Multiply(vLookDir, 8.0f * fElapsedTime); // velocity vector

            if (GetKey(olc::Key::W).bHeld)
                vCamera = Vector_Add(vCamera, vForward);

            if (GetKey(olc::Key::S).bHeld)
                vCamera = Vector_Substract(vCamera, vForward);

            if (GetKey(olc::Key::A).bHeld)
                fYaw -= 2.0f * fElapsedTime;
            
            if (GetKey(olc::Key::D).bHeld)
                fYaw += 2.0f * fElapsedTime;

            mat4x4 matRotZ, matRotX, matTrans, matWorld;
            // fTheta += 1.0f * fElapsedTime;

            // --- Transformation Matrix ---------------------------------------------------------
            // Set up the tranformation matrix
            matRotZ = Matrix_MakeRotationZ(fTheta * 0.5f); // Generate Rotatio (z) matrix transform
            matRotX = Matrix_MakeRotationX(fTheta); // Generate Rotation (X) matrix transform
            matTrans = Matrix_MakeTranslation(0.0f, 0.0f, 8.0f); // Generate Translation matrix transform
            matWorld = Matrix_MakeIdentity(); // Start with identity matrix
            matWorld = Matrix_MultiplyMatrix(matRotZ, matRotX); // Rotating around the origin 
            matWorld = Matrix_MultiplyMatrix(matWorld, matTrans); // Rotation + Translation to a different location
            
            // --- Camera Matrix ---------------------------------------------------------
            // Set up Camera Matrix 
            vec3d vUp = { 0, 1, 0 };
            vec3d vTarget = { 0, 0, 1 }; // Target vector fixed to the z axis
            mat4x4 matCameraRot = Matrix_MakeRotationY(fYaw); // Create rotation matrix using Yaw radians (turning the player, left to right)
            vLookDir = Matrix_MultiplyVector(matCameraRot, vTarget); // vLookDir is some unit vector rotated around the origin --forward facing vector
            vTarget = Vector_Add(vCamera, vLookDir); // Offset the "look direction" to the current camera location --target for the camera to look at
            mat4x4 matCamera = Matrix_PointAt(vCamera, vTarget, vUp); // Point At matrix for the camera position and orientation
            mat4x4 matView = Matrix_LookAt(matCamera); // Make view natrix from camera

            // To store triangles for rastering.
            std::vector<triangle> vectTrianglesToRaster;

            // 2 Loops:
            // 1: Select the triangles that we'll send to the rasterizer (aligned with the camera view).
            // 2: Draw the triangles in order, from farthest to nearest.

            // ============================================================================
            // Loop 1: Select the triangles
            // ============================================================================

            for (auto tri : meshCube.tris)
            {
                triangle triProjected, triTransformed, triViewed;

                // --- Move from model space to world space ----------------------------------------------
                // Multiply each point of the triangle by the world (transformation) matrix.
                // Rotation around Z, rotation around X, Translation.
                triTransformed.p[0] = Matrix_MultiplyVector(matWorld, tri.p[0]);
                triTransformed.p[1] = Matrix_MultiplyVector(matWorld, tri.p[1]);
                triTransformed.p[2] = Matrix_MultiplyVector(matWorld, tri.p[2]);
                
                triTransformed.t[0] = tri.t[0]; // Copy the texture coodinates
                triTransformed.t[1] = tri.t[1];
                triTransformed.t[2] = tri.t[2];

                // --- Calculate the normals to the triangles ---------------------------------------------
                // Allow us to work out various properties of the triangles: visibility and colors.
                vec3d normal, line1, line2;
                // Calculate 2 (line) vectors for the triangle.
                line1 = Vector_Substract(triTransformed.p[1], triTransformed.p[0]);
                line2 = Vector_Substract(triTransformed.p[2], triTransformed.p[0]);
                // Cross product to find the normal vector.
                normal = Vector_CrossProduct(line1, line2);
                normal = Vector_Normalise(normal); // Normalise

                // --- Translate, scale and draw ----------------------------------------------------------
                // If Condition: Translate, scale and draw the triangle only if
                // Dot Product between the triangle surface normal and 
                // the line between the camera and the triangle is less than 0. 
                // It means that the 2 vectors point in the same (but opposite) direction.

                vec3d vCameraRay = Vector_Substract(triTransformed.p[0], vCamera); // Compute the ray from the camera to a point on the triangle
                float dotP_normalCam = Vector_DotProduct(normal, vCameraRay);

                if ( dotP_normalCam < 0.0f )
                {
                    // --- Calculate the illumination ---------------------------------------------------------
                    vec3d light_direction = { 0.0f, 0.0f, -1.0f };
                    light_direction = Vector_Normalise(light_direction); // Normalise the light direction vector
                    float dotP_normalLight = Vector_DotProduct(normal, light_direction); // How "aligned" are the light direction and triangle surface normal?
                    
                    if ( dotP_normalLight < 0.1f ) dotP_normalLight = 0.1f; // No negative value of the dot product

                    uint8_t shade = (uint8_t)( dotP_normalLight * 255.0f ); // Turn the luminance into a grey olc::Pixel and store it
                    triTransformed.color = olc::Pixel( shade, shade, shade );

                    // --- From world space --> View space ----------------------------------------------------
                    triViewed.p[0] = Matrix_MultiplyVector(matView, triTransformed.p[0]);
                    triViewed.p[1] = Matrix_MultiplyVector(matView, triTransformed.p[1]);
                    triViewed.p[2] = Matrix_MultiplyVector(matView, triTransformed.p[2]);
                    triViewed.color = triTransformed.color;

                    triViewed.t[0] = tri.t[0]; // Copy the texture coodinates
                    triViewed.t[1] = tri.t[1];
                    triViewed.t[2] = tri.t[2];

                    // --- Clipping ----------------------------------------------------------------------------
                    // Clipping against a single plane in a Z-axis.
                    int nClippedTriangles = 0; 
                    triangle clipped[2];
                    nClippedTriangles = Triangle_ClipAgainstPlane(
                        { 0.0f, 0.0f, 0.1f },
                        { 0.0f, 0.0f, 1.0f },
                        triViewed,
                        clipped[0],
                        clipped[1]
                    );

                    // --- Projection - From 3D to 2D ----------------------------------------------------------
                    // Loop through the possible 1 to 3 triangles (created during the clipping).
                    for (int n = 0; n < nClippedTriangles; n++)
                    {
                        // Project to the screen.
                        clipped[n].color = triViewed.color;
                        triProjected.p[0] = Matrix_MultiplyVector(matProj, clipped[n].p[0]);
                        triProjected.p[1] = Matrix_MultiplyVector(matProj, clipped[n].p[1]);
                        triProjected.p[2] = Matrix_MultiplyVector(matProj, clipped[n].p[2]);
                        triProjected.color = clipped[n].color; 

                        triProjected.t[0] = clipped[n].t[0]; // Copy the texture coodinates
                        triProjected.t[1] = clipped[n].t[1];
                        triProjected.t[2] = clipped[n].t[2];

                        // Perspective-correction of our texture coordinates u and v.
                        // Use w, as depth, to modify the texture coordinates.
                        triProjected.t[0].u = triProjected.t[0].u / triProjected.p[0].w;
                        triProjected.t[0].v = triProjected.t[0].v / triProjected.p[0].w;
                        triProjected.t[1].u = triProjected.t[1].u / triProjected.p[1].w;
                        triProjected.t[1].v = triProjected.t[1].v / triProjected.p[1].w;
                        triProjected.t[2].u = triProjected.t[2].u / triProjected.p[2].w;
                        triProjected.t[2].v = triProjected.t[2].v / triProjected.p[2].w;
                        triProjected.t[0].w = 1.0f / triProjected.p[0].w; // Will be use to recover the true u and v
                        triProjected.t[1].w = 1.0f / triProjected.p[1].w;
                        triProjected.t[2].w = 1.0f / triProjected.p[2].w;

                        // Normalise the vectors.
                        triProjected.p[0] = Vector_Divide(triProjected.p[0], triProjected.p[0].w);
                        triProjected.p[1] = Vector_Divide(triProjected.p[1], triProjected.p[1].w);
                        triProjected.p[2] = Vector_Divide(triProjected.p[2], triProjected.p[2].w);

                        // X/Y are inverted so put them back.
                        triProjected.p[0].x *= -1.0f;
                        triProjected.p[1].x *= -1.0f;
                        triProjected.p[2].x *= -1.0f;
                        triProjected.p[0].y *= -1.0f;
                        triProjected.p[1].y *= -1.0f;
                        triProjected.p[2].y *= -1.0f;

                        // Scaling: Scale into view.
                        triProjected.p[0].x += 1.0f;    triProjected.p[0].y += 1.0f;
                        triProjected.p[1].x += 1.0f;    triProjected.p[1].y += 1.0f;
                        triProjected.p[2].x += 1.0f;    triProjected.p[2].y += 1.0f;
                        triProjected.p[0].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[0].y *= 0.5f * (float)ScreenHeight();
                        triProjected.p[1].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[1].y *= 0.5f * (float)ScreenHeight();
                        triProjected.p[2].x *= 0.5f * (float)ScreenWidth();
                        triProjected.p[2].y *= 0.5f * (float)ScreenHeight();

                        // Store the triangle for Painter's Algorithm.
                        vectTrianglesToRaster.push_back(triProjected);
                    }
                }
            }   

            // ============================================================================
            // Loop 2: Draw the triangles
            // ============================================================================

            // Clipping against the 4 edges of the screen.
            // Max 3 triangles for 1 triangle.
            for (auto &tritoRaster : vectTrianglesToRaster)
            {
                triangle clipped[2]; 
                std::list<triangle> listTriangles;
                listTriangles.push_back(tritoRaster);
                int nNewTriangles = 1; 

                // Test for each plane, each triangle in the queue.
                for ( int p = 0; p < 4; p++ )
                {
                    int nTrisToAdd = 0; 
                    
                    while ( nNewTriangles > 0 )
                    {
                        triangle test = listTriangles.front();
                        listTriangles.pop_front();
                        nNewTriangles--;

                        // Each call describes a plane.
                        switch (p)
                        {   
                            // Top edge (y = 0).
                            case 0:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
                            // Bottom edge (y = ScreenHeight - 1).
                            case 1:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, (float)ScreenHeight() - 1, 0.0f }, { 0.0f, -1.0f, 0.0f }, test, clipped[0], clipped[1]); break;
                            // Left edge (x = 0).
                            case 2:	nTrisToAdd = Triangle_ClipAgainstPlane({ 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
                            // Right edge (x = ScreenWidth - 1).
                            case 3:	nTrisToAdd = Triangle_ClipAgainstPlane({ (float)ScreenWidth() - 1, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, test, clipped[0], clipped[1]); break;
                        }
                    
                        for (int w = 0; w < nTrisToAdd; w++)
                            listTriangles.push_back(clipped[w]);
                    }
                    nNewTriangles = listTriangles.size();
                }

                // Draw the transformed, viewed, clipped, projected, sorted, clipped triangles.
                for (auto &t : listTriangles)
                {
                    // DrawTriangle(t.p[0].x, t.p[0].y,
                    //             t.p[1].x, t.p[1].y,
                    //             t.p[2].x, t.p[2].y,
                    //             olc::WHITE);

                    TexturedTriangle( t.p[0].x, t.p[0].y, t.t[0].u, t.t[0].v, t.t[0].w,
                        t.p[1].x, t.p[1].y, t.t[1].u, t.t[1].v, t.t[1].w,
                        t.p[2].x, t.p[2].y, t.t[2].u, t.t[2].v, t.t[2].w,
                        sprTex1
                    );
                }
            } 
            return true;
        }   
    
    /**
     * @brief   Draws triangles filled with a texture.
     *
     * @param[in]  v1_   Vertices and textures for vertex 1.
     * @param[in]  v2_   Vertices and textures for vertex 2.
     * @param[in]  v3_   Vertices and textures for vertex 3.
     * @param[in]  tex.  Pointer to the texture image.
     * @return           None, draw the triangles.
     *
     */
    void TexturedTriangle( int v1_x, int v1_y, float v1_u, float v1_v, float v1_w,
        int v2_x, int v2_y, float v2_u, float v2_v, float v2_w,
        int v3_x, int v3_y, float v3_u, float v3_v, float v3_w,
        olc::Sprite *tex )
    {
        // The triangle is drawn in two halves:
        // 1: Top half (flat-bottom shape): loop from v1_y to v2_y.
        // 2: Bottom half (flat-top shape): a second loop from v2_y to v3_y.

        // Sort y coordinates.
        if ( v2_y < v1_y )
        {
            std::swap(v1_y, v2_y);
            std::swap(v1_x, v2_x);
            std::swap(v1_u, v2_u);
            std::swap(v1_v, v2_v);
            std::swap(v1_w, v2_w);
        }
        if ( v3_y < v1_y )
        {
            std::swap(v1_y, v3_y);
            std::swap(v1_x, v3_x);
            std::swap(v1_u, v3_u);
            std::swap(v1_v, v3_v);
            std::swap(v1_w, v3_w);
        }
        if ( v3_y < v2_y )
        {
            std::swap(v2_y, v3_y);
            std::swap(v2_x, v3_x);
            std::swap(v2_u, v3_u);
            std::swap(v2_v, v3_v);
            std::swap(v2_w, v3_w);
        }
        
        // Calculate the sizes of the lines of the triangle.
        // Edge A = Vertex 1 <-> Vertex 2 // Edge B = Vertex 1 <-> Vertex 3.
        // Edge A
        int dist_edgeA_x = v2_x - v1_x;
        int dist_edgeA_y = v2_y - v1_y; // Example:  the edge is 10 pixels tall
        float dist_edgeA_u = v2_u - v1_u;
        float dist_edgeA_v = v2_v - v1_v; 
        float dist_edgeA_w = v2_w - v1_w; 
        // Edge B 
        int dist_edgeB_x = v3_x - v1_x;
        int dist_edgeB_y = v3_y - v1_y;
        float dist_edgeB_u = v3_u - v1_u;
        float dist_edgeB_v = v3_v - v1_v;
        float dist_edgeB_w = v3_w - v1_w;

        float tex_u, tex_v, tex_w; // Final point of the texture of which we are going to sample

        // Step values (slope with y on the horizontal axis).
        // Edge A
        float step_edgeA_x = 0;
        float step_edgeA_u = 0;
        float step_edgeA_v = 0;
        float step_edgeA_w = 0;
        // Edge B
        float step_edgeB_x = 0;
        float step_edgeB_u = 0;
        float step_edgeB_v = 0;
        float step_edgeB_w = 0;

        // Test if the line exists or is flat.
        // dist_edgeA_y == 0 means both ends of the edge are on the same row, so the edge is flat (horizontal).
        // If the edge if flat, the step variables are not computed. They keep their initial value.
        // Edge A
        if (dist_edgeA_y) step_edgeA_x = dist_edgeA_x / (float)abs(dist_edgeA_y); // How much x changes when y increases by 1 (edge A)
        if (dist_edgeA_y) step_edgeA_u = dist_edgeA_u / (float)abs(dist_edgeA_y);
        if (dist_edgeA_y) step_edgeA_v = dist_edgeA_v / (float)abs(dist_edgeA_y);
        if (dist_edgeA_y) step_edgeA_w = dist_edgeA_w / (float)abs(dist_edgeA_y);
        // Edge B
        if (dist_edgeB_y) step_edgeB_x = dist_edgeB_x / (float)abs(dist_edgeB_y); // How much x changes when y increases by 1 (edge B)
        if (dist_edgeB_y) step_edgeB_u = dist_edgeB_u / (float)abs(dist_edgeB_y); 
        if (dist_edgeB_y) step_edgeB_v = dist_edgeB_v / (float)abs(dist_edgeB_y);
        if (dist_edgeB_y) step_edgeB_w = dist_edgeB_w / (float)abs(dist_edgeB_y);

        // --- First part: Top half ---------------------------------------------------------
        // Top half (flat-bottom shape): loop from v1_y to v2_y.

        if (dist_edgeA_y)
        { 
            // Walk down the triangle one scanline at a time.
            for (int iy = v1_y; iy <= v2_y; iy++)
            {  
                // Edge 1 (A)
                int pos_edgeA_x = v1_x + (float)(iy - v1_y) * step_edgeA_x; // Walk along edge A
                float pos_edgeA_u = v1_u + (float)(iy - v1_y) * step_edgeA_u;
                float pos_edgeA_v = v1_v + (float)(iy - v1_y) * step_edgeA_v;
                float pos_edgeA_w = v1_w + (float)(iy - v1_y) * step_edgeA_w;

                // Edge 2 (B) 
                int pos_edgeB_x = v1_x + (float)(iy - v1_y) * step_edgeB_x; // Walk along edge B
                float pos_edgeB_u = v1_u + (float)(iy - v1_y) * step_edgeB_u;
                float pos_edgeB_v = v1_v + (float)(iy - v1_y) * step_edgeB_v;
                float pos_edgeB_w = v1_w + (float)(iy - v1_y) * step_edgeB_w;

                // Ensure that we draw from start to end.
                if (pos_edgeA_x > pos_edgeB_x)
                { 
                    std::swap(pos_edgeA_x, pos_edgeB_x); 
                    std::swap(pos_edgeA_u, pos_edgeB_u);
                    std::swap(pos_edgeA_v, pos_edgeB_v);
                    std::swap(pos_edgeA_w, pos_edgeB_w);
                }
                
                // Interpolate from the left coordinates to the right ones.
                tex_u = pos_edgeA_u; // Starting values
                tex_v = pos_edgeA_v;
                tex_w = pos_edgeA_w;

                float tstep = 1.0f / ((float)(pos_edgeB_x - pos_edgeA_x)); // How much t grows per pixel
                float t = 0.0f; // Current position along the scanline, as a fraction from 0.0 to 1.0

                // Draw the triangle.
                // Step by step - One step corresponds to one pixel.
                for (int ix = pos_edgeA_x; ix < pos_edgeB_x; ix++)
                {
                    // Linear interpolation between A's and B's texture coordinates.
                    tex_u = pos_edgeA_u + t * (pos_edgeB_u - pos_edgeA_u);
                    tex_v = pos_edgeA_v + t * (pos_edgeB_v - pos_edgeA_v);
                    tex_w = pos_edgeA_w + t * (pos_edgeB_w - pos_edgeA_w);
                    //                    ^                ^
                    //                 fraction    total distance from A to B

                    // Draw only if the w value is greater than the current value at that point.
                    // The nearer pixel has the bigger w because w = 1/z.
                    if ( tex_w > pDepthBuffer[iy * ScreenWidth() + ix] )
                    {
                        Draw(ix, iy, tex->Sample(tex_u / tex_w, tex_v / tex_w)); // Sample returns the color (olc::Pixel)
                        pDepthBuffer[iy * ScreenWidth() + ix] = tex_w;
                    }

                    t += tstep;
                }
            }
        } 

        // --- Second part: Bottom half ---------------------------------------------------------
        // Bottom half (flat-top shape): a second loop from v2_y to v3_y
        // Edge C
        int dist_edgeC_x = v3_x - v2_x;
        int dist_edgeC_y = v3_y - v2_y;
        float dist_edgeC_u = v3_u - v2_u;
        float dist_edgeC_v = v3_v - v2_v;
        float dist_edgeC_w = v3_w - v2_w;

        float step_edgeC_x = 0;
        float step_edgeC_u = 0;
        float step_edgeC_v = 0;       
        float step_edgeC_w = 0;

        if (dist_edgeC_y) step_edgeC_x = dist_edgeC_x / (float)abs(dist_edgeC_y);
        if (dist_edgeC_y) step_edgeC_u = dist_edgeC_u / (float)abs(dist_edgeC_y);
        if (dist_edgeC_y) step_edgeC_v = dist_edgeC_v / (float)abs(dist_edgeC_y);
        if (dist_edgeC_y) step_edgeC_w = dist_edgeC_w / (float)abs(dist_edgeC_y);         

        if (dist_edgeC_y)
        {
            for (int iy = v2_y; iy <= v3_y; iy++)
            { 
                // Edge B (y1 → y2)
                int pos_edgeB_x = v1_x + (float)(iy - v1_y) * step_edgeB_x; // Walk along edge A (second half)
                float pos_edgeB_u = v1_u + (float)(iy - v1_y) * step_edgeB_u;
                float pos_edgeB_v = v1_v + (float)(iy - v1_y) * step_edgeB_v;
                float pos_edgeB_w = v1_w + (float)(iy - v1_y) * step_edgeB_w;

                // Edge C (y2 → y3)
                int pos_edgeC_x = v2_x + (float)(iy - v2_y) * step_edgeC_x; // Walk along edge C
                float pos_edgeC_u = v2_u + (float)(iy - v2_y) * step_edgeC_u;
                float pos_edgeC_v = v2_v + (float)(iy - v2_y) * step_edgeC_v;
                float pos_edgeC_w = v2_w + (float)(iy - v2_y) * step_edgeC_w;

                // Ensure that we draw from start to end.
                if (pos_edgeC_x > pos_edgeB_x)
                { 
                    std::swap(pos_edgeC_x, pos_edgeB_x); 
                    std::swap(pos_edgeC_u, pos_edgeB_u);
                    std::swap(pos_edgeC_v, pos_edgeB_v);
                    std::swap(pos_edgeC_w, pos_edgeB_w);
                }
                    
                // Interpolate from the left coordinates to the right ones.
                tex_u = pos_edgeC_u; // Starting values
                tex_v = pos_edgeC_v;
                tex_w = pos_edgeC_w;

                float tstep = 1.0f / ((float)(pos_edgeB_x - pos_edgeC_x)); // How much t grows per pixel
                float t = 0.0f; // Current position along the scanline, as a fraction from 0.0 to 1.0

                // Draw the triangle.
                // Step by step - One step corresponds to one pixel.
                for (int ix = pos_edgeC_x; ix < pos_edgeB_x; ix++)
                {
                    // Linear interpolation between A's and B's texture coordinates.
                    tex_u = pos_edgeC_u + t * (pos_edgeB_u - pos_edgeC_u);
                    tex_v = pos_edgeC_v + t * (pos_edgeB_v - pos_edgeC_v);
                    tex_w = pos_edgeC_w + t * (pos_edgeB_w - pos_edgeC_w);
                    //                    ^                ^
                    //                 fraction    total distance from A to B

                    if ( tex_w > pDepthBuffer[iy * ScreenWidth() + ix] )
                    {
                        Draw(ix, iy, tex->Sample(tex_u / tex_w, tex_v / tex_w)); // Sample returns the color (olc::Pixel).
                        pDepthBuffer[iy * ScreenWidth() + ix] = tex_w;
                    }

                    t += tstep;
                }
            }
        }
    }

};

// ============================================================================
// Section: Main Function
// ============================================================================

int main()
{
    olcEngine3D demo;
    if (demo.Construct(512, 480, 2, 2))
        demo.Start();

    return 0;
}