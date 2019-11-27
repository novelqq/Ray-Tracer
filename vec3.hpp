#include <GL/glut.h>
#include <cmath>
#include <iostream>

class vec3{
    public: 
        GLfloat x, y, z; 
        vec3() : x(GLfloat(0)), y(GLfloat(0)), z(GLfloat(0)) {} 
        vec3(GLfloat s) : x(s), y(s), z(s) {} 
        vec3(GLfloat x, GLfloat y, GLfloat z) : x(x), y(y), z(z) {} 
        vec3( const vec3& v ) { x = v.x;  y = v.y;  z = v.z; }
        vec3 operator * (const GLfloat &f) const { 
            return vec3(x * f, y * f, z * f); 
        } 
        vec3 operator * (const vec3 &v) const { 
            return vec3(x * v.x, y * v.y, z * v.z); 
        } 
        GLfloat dot(const vec3 &v) const { 
            return x * v.x + y * v.y + z * v.z; 
        } 
        vec3 operator - (const vec3 &v) const {
             return vec3(x - v.x, y - v.y, z - v.z);
        } 
        vec3 operator + (const vec3 &v) const {
            return vec3(x + v.x, y + v.y, z + v.z); 
        } 
        vec3& operator += (const vec3 &v) { 
            this->x += v.x, this->y += v.y, this->z += v.z; 
            return *this; 
        } 
        vec3& operator *= (const vec3 &v) { 
            x *= v.x, y *= v.y, z *= v.z; 
            return *this; 
        } 
        vec3 operator - () const { 
            return vec3(-x, -y, -z); 
        } 
        GLfloat length() const { 
            return sqrt(square()); 
        } 
        vec3& normalize(){ 
            GLfloat sq = square(); 
            if (sq > 0) { 
                GLfloat inv = 1 / sqrt(sq); 
                x *= inv, y *= inv, z *= inv; 
            } 
            return *this; 
        } 
    private:
        GLfloat square() const { 
            return x * x + y * y + z * z; 
        } 
}; 
