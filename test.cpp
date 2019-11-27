#include <cstdlib> 
#include <cstdio> 
#include <cmath> 
#include <fstream> 
#include <vector> 
#include <iostream> 
#include <cassert> 
#include <GL/glut.h>
#include "vec3.hpp"
#if defined __linux__ || defined __APPLE__ 
// "Compiled for Linux
#else 
// Windows doesn't define these values by default, Linux does
#define M_PI 3.141592653589793 
#define INFINITY 1e8 
#endif 
unsigned width = 400, height = 400; 
GLfloat invWidth = 1 / GLfloat(width), invHeight = 1 / GLfloat(height); 
GLfloat fov = 30, aspectratio = width / GLfloat(height); 
GLfloat angle = tan(M_PI * 0.5 * fov / 180.); 
vec3 *image = new vec3[width * height];
const int MAX_RAY_DEPTH = 5;

class Sphere{ 
public: 
    vec3 center;                           /// position of the sphere 
    GLfloat radius, radius2;                  /// sphere radius and radius^2 
    vec3 surfaceColor, emissionColor;      /// surface color and emission (light) 
    GLfloat transparency, reflection;         /// surface transparency and reflectivity 
    Sphere( 
        const vec3 &c, 
        const GLfloat &r, 
        const vec3 &sc, 
        const GLfloat &refl = 0, 
        const GLfloat &transp = 0, 
        const vec3 &ec = 0) : 
        center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec), 
        transparency(transp), reflection(refl) 
    { /* empty */ } 
    bool intersect(const vec3 &rayorig, const vec3 &raydir, GLfloat &t0, GLfloat &t1) const{ 
        vec3 l = center - rayorig; 
        GLfloat tca = l.dot(raydir); 
        if (tca < 0) return false; 
        GLfloat d2 = l.dot(l) - tca * tca; 
        if (d2 > radius2) return false; 
        GLfloat thc = sqrt(radius2 - d2); 
        t0 = tca - thc; 
        t1 = tca + thc; 
 
        return true; 
    } 
}; 

 
GLfloat mix(const GLfloat &a, const GLfloat &b, const GLfloat &mix){ 
    return b * mix + a * (1 - mix); 
} 
vec3 trace( 
    const vec3 &rayorig, 
    const vec3 &raydir, 
    const std::vector<Sphere> &spheres, 
    const int &depth) 
{ 
    //if (raydir.length() != 1) std::cerr << "Error " << raydir << std::endl;
    GLfloat tnear = INFINITY; 
    const Sphere* sphere = NULL; 
    // find intersection of this ray with the sphere in the scene
    for (unsigned i = 0; i < spheres.size(); ++i) { 
        GLfloat t0 = INFINITY, t1 = INFINITY; 
        if (spheres[i].intersect(rayorig, raydir, t0, t1)) { 
            if (t0 < 0) t0 = t1; 
            if (t0 < tnear) { 
                tnear = t0; 
                sphere = &spheres[i]; 
            } 
        } 
    } 
    // if there's no intersection return black or background color
    if (!sphere) return vec3(2); 
    vec3 surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray 
    vec3 phit = rayorig + raydir * tnear; // point of intersection 
    vec3 nhit = phit - sphere->center; // normal at the intersection point 
    nhit.normalize(); // normalize normal direction 
    // If the normal and the view direction are not opposite to each other
    // reverse the normal direction. That also means we are inside the sphere so set
    // the inside bool to true. Finally reverse the sign of IdotN which we want
    // positive.
    GLfloat bias = 1e-4; // add some bias to the point from which we will be tracing 
    bool inside = false; 
    if (raydir.dot(nhit) > 0) nhit = -nhit, inside = true; 
    if ((sphere->transparency > 0 || sphere->reflection > 0) && depth < MAX_RAY_DEPTH) { 
        GLfloat facingratio = -raydir.dot(nhit); 
        // change the mix value to tweak the effect
        GLfloat fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1); 
        // compute reflection direction (not need to normalize because all vectors
        // are already normalized)
        vec3 refldir = raydir - nhit * 2 * raydir.dot(nhit); 
        refldir.normalize(); 
        vec3 reflection = trace(phit + nhit * bias, refldir, spheres, depth + 1); 
        vec3 refraction = 0; 
        // if the sphere is also transparent compute refraction ray (transmission)
        if (sphere->transparency) { 
            GLfloat ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface? 
            GLfloat cosi = -nhit.dot(raydir); 
            GLfloat k = 1 - eta * eta * (1 - cosi * cosi); 
            vec3 refrdir = raydir * eta + nhit * (eta *  cosi - sqrt(k)); 
            refrdir.normalize(); 
            refraction = trace(phit - nhit * bias, refrdir, spheres, depth + 1); 
        } 
        // the result is a mix of reflection and refraction (if the sphere is transparent)
        surfaceColor = ( 
            reflection * fresneleffect + 
            refraction * (1 - fresneleffect) * sphere->transparency) * sphere->surfaceColor; 
    } 
    else { 
        // it's a diffuse object, no need to raytrace any further
        for (unsigned i = 0; i < spheres.size(); ++i) { 
            if (spheres[i].emissionColor.x > 0) { 
                // this is a light
                vec3 transmission = 1; 
                vec3 lightDirection = spheres[i].center - phit; 
                lightDirection.normalize(); 
                for (unsigned j = 0; j < spheres.size(); ++j) { 
                    if (i != j) { 
                        GLfloat t0, t1; 
                        if (spheres[j].intersect(phit + nhit * bias, lightDirection, t0, t1)) { 
                            transmission = 0; 
                            break; 
                        } 
                    } 
                } 
                surfaceColor = surfaceColor + sphere->surfaceColor * transmission * 
                std::max(GLfloat(0), nhit.dot(lightDirection)) * spheres[i].emissionColor; 
            } 
        } 
    } 
 
    return surfaceColor + sphere->emissionColor; 
} 
void render(const std::vector<Sphere> &spheres){ 
    // Trace rays
    int k = width * height;
    for (unsigned y = 0; y < height; ++y) { 
        for (unsigned x = 0; x < width; ++x, --k) { 
            GLfloat xx = (2 * ((x + 0.5) * invWidth) - 1) * angle * aspectratio; 
            GLfloat yy = (1 - 2 * ((y + 0.5) * invHeight)) * angle; 
            vec3 raydir(xx, yy, -1); 
            raydir.normalize(); 
            image[k] = trace(vec3(0), raydir, spheres, 0); 
        } 
    } 
} 

void reshape(int w, int h){
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
}
void render_scene(void){
    glMatrixMode(GL_MODELVIEW);
    glRasterPos2f(0.0, 0.0);
    glDrawPixels(width, height, GL_RGB, GL_FLOAT, image);
    glutSwapBuffers();
}
void construct_scene_1(){
    std::vector<Sphere> spheres; 
    // position, radius, surface color, reflectivity, transparency, emission color
    spheres.push_back(Sphere(vec3( 0.0, -10004, -20), 10000, vec3(0.20, 0.20, 0.20), 0, 0.0)); 
    spheres.push_back(Sphere(vec3( 0.0,      0, -20),     4, vec3(1.00, 0.32, 0.36), 1, 0.0)); 
    spheres.push_back(Sphere(vec3( 5.0,     -1, -15),     2, vec3(0.90, 0.76, 0.46), 1, 0.0)); 
    spheres.push_back(Sphere(vec3( 5.0,      0, -25),     3, vec3(0.65, 0.77, 0.97), 1, 0.0)); 
    spheres.push_back(Sphere(vec3(-5.5,      0, -15),     3, vec3(0.90, 0.90, 0.90), 1, 0.0)); 
    // light
    //spheres.push_back(Sphere(vec3( 0.0,     20, -30),     3, vec3(0.00, 0.00, 0.00), 0, 0.0, vec3(3))); 
    render(spheres); 
}
int main(int argc, char **argv){ 
    construct_scene_1();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100, 500);
    glutInitWindowSize(width, height);
    glutCreateWindow("Raytracing program");
    glutDisplayFunc(render_scene);
    glutReshapeFunc(reshape);
    glutMainLoop();

    return 0; 
} 