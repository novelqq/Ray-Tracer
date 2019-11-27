#include <cstdlib> 
#include <cstdio> 
#include <cmath> 
#include <vector> 
#include <iostream> 
#include <GL/glut.h>
#include "vec3.hpp"

//# of pixels
const int WIDTH = 400, HEIGHT = 400; 

//stores rendered pixels
vec3 *image = new vec3[WIDTH * HEIGHT];

//ray recursion depth
const int MAX_RAY_DEPTH = 5;

//some calculations needed for determining initial rays for each pixel
GLfloat WIDTH_INVERSE = 1 / GLfloat(WIDTH);
GLfloat HEIGHT_INVERSE = 1 / GLfloat(HEIGHT); 
GLfloat fov = 30, aspectratio = WIDTH / GLfloat(HEIGHT); 
GLfloat angle = tan(M_PI * 0.5 * fov / 180.); 

//Ray is an object that consists of a start point origin and a vec3 direction
class Ray{
    public:
        Ray(const vec3& origin, const vec3& direction):
            direction(direction), origin(origin){}
        vec3 direction;
        vec3 origin;
};

//Material is an object that contains material properties needed by primitives
class Material{
    public:
        Material(
            const vec3& kd, 
            const vec3& ks, 
            const GLfloat q, 
            const GLfloat kr = 0, 
            const GLfloat kt = 0,
            const GLfloat ki = 0):
                kd(kd), ks(ks), q(q), kr(kr), kt(kt), ki(ki){}
        const vec3 kd;
        const vec3 ks;
        const GLfloat q;
        const GLfloat kr;
        const GLfloat kt;
        const GLfloat ki;
};

//Plane is an object representing a flat plane primitive ax + by + cz + d = 0 with a material
class Plane{
    public:
        Plane(GLfloat &a, GLfloat &b, GLfloat &c, GLfloat &d, Material &material):
            a(a), b(b), c(c), d(d), material(material){}
        GLfloat a, b, c, d;
        Material material;

        GLfloat intersect(const Ray& ray){
            GLfloat t = (vec3(a, b, c).dot(ray.origin) + d) / (vec3(a, b, c).dot(ray.direction));
            return t;  
        }

};

//simple local point light sources with no falloff, consists of a vec3 center and a vec3 color intensity
class Light{
    public:
        vec3 center;
        vec3 color;
};

//Sphere is an object representing a sphere primitive with vec3 center, radius, and material
class Sphere{ 
    public: 
        vec3 center;
        GLfloat radius, radius2;
        Material material;
        Sphere(
            const vec3 &c, 
            const GLfloat &r, 
            const Material &mat): 
                center(c), radius(r), radius2(r * r), material(material){}
        
        //calculate nearest ray sphere intersection
        GLfloat intersect(const Ray& ray) const{
            vec3 oc = center - ray.origin;
            GLfloat k1 = ray.direction.dot(ray.direction);
            GLfloat k2 = 2 * (oc.dot(ray.direction));
            GLfloat k3 = oc.dot(oc) - radius2;
            GLfloat discriminant = (k2*k2) - (4*k1*k3);
            GLfloat t0 = (-k2 + sqrt(discriminant)) / (2*k1);
            GLfloat t1 = (-k2 - sqrt(discriminant)) / (2*k1);
            return std::min(t0, t1); 
        } 
}; 

 
GLfloat mix(const GLfloat &a, const GLfloat &b, const GLfloat &mix){ 
    return b * mix + a * (1 - mix); 
} 

//trace takes a Ray ray and a int depth and returns a vec3 color intensity. If depth == 0, will return bg
vec3 trace(const Ray& ray, const int &depth){
    GLfloat min_distance = INFINITY;
    
}
// vec3 trace(const Ray& ray, const std::vector<Sphere> &spheres, const int &depth){ 
//     //if (ray.direction.length() != 1) std::cerr << "Error " << ray.direction << std::endl;
//     GLfloat tnear = INFINITY; 
//     const Sphere* sphere = NULL; 
//     // find intersection of this ray with the sphere in the scene
//     for (unsigned i = 0; i < spheres.size(); ++i) { 
//         GLfloat t0 = INFINITY, t1 = INFINITY; 
//         if (spheres[i].intersect(ray, t0, t1)) { 
//             if (t0 < 0) t0 = t1; 
//             if (t0 < tnear) { 
//                 tnear = t0; 
//                 sphere = &spheres[i]; 
//             } 
//         } 
//     } 
//     // if there's no intersection return black or background color
//     if (!sphere) return vec3(2); 
//     vec3 surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray 
//     vec3 phit = ray.origin + ray.direction * tnear; // point of intersection 
//     vec3 nhit = phit - sphere->center; // normal at the intersection point 
//     nhit.normalize(); // normalize normal direction 
//     // If the normal and the view direction are not opposite to each other
//     // reverse the normal direction. That also means we are inside the sphere so set
//     // the inside bool to true. Finally reverse the sign of IdotN which we want
//     // positive.
//     GLfloat bias = 1e-4; // add some bias to the point from which we will be tracing 
//     bool inside = false; 
//     if (ray.direction.dot(nhit) > 0) nhit = -nhit, inside = true; 
//     if ((sphere->material.kt > 0 || sphere->material.kr > 0) && depth < MAX_RAY_DEPTH) { 
//         GLfloat facingratio = -ray.direction.dot(nhit); 
//         // change the mix value to tweak the effect
//         GLfloat fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1); 
//         // compute reflection direction (not need to normalize because all vectors
//         // are already normalized)
//         vec3 refldir = ray.direction - nhit * 2 * ray.direction.dot(nhit); 
//         refldir.normalize(); 
//         vec3 reflection = trace(Ray(phit + nhit * bias, refldir), spheres, depth + 1); 
//         vec3 refraction = 0; 
//         // if the sphere is also transparent compute refraction ray (transmission)
//         if (sphere->material.kt) { 
//             GLfloat ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface? 
//             GLfloat cosi = -nhit.dot(ray.direction); 
//             GLfloat k = 1 - eta * eta * (1 - cosi * cosi); 
//             vec3 refrdir = ray.direction * eta + nhit * (eta *  cosi - sqrt(k)); 
//             refrdir.normalize(); 
//             refraction = trace(Ray(phit - nhit * bias, refrdir), spheres, depth + 1); 
//         } 
//         // the result is a mix of reflection and refraction (if the sphere is transparent)
//         surfaceColor = ( 
//             reflection * fresneleffect + 
//             refraction * (1 - fresneleffect) * sphere->material.kt) * sphere->material.kd; 
//     } 
//     else { 
//         // it's a diffuse object, no need to raytrace any further
//         for (unsigned i = 0; i < spheres.size(); ++i) { 
//             if (spheres[i].material.ks.x > 0) { 
//                 // this is a light
//                 vec3 transmission = 1; 
//                 vec3 lightDirection = spheres[i].center - phit; 
//                 lightDirection.normalize(); 
//                 for (unsigned j = 0; j < spheres.size(); ++j) { 
//                     if (i != j) { 
//                         GLfloat t0, t1; 
//                         if (spheres[j].intersect(Ray(phit + nhit * bias, lightDirection), t0, t1)) { 
//                             transmission = 0; 
//                             break; 
//                         } 
//                     } 
//                 } 
//                 surfaceColor = surfaceColor + sphere->material.kd * transmission * 
//                 std::max(GLfloat(0), nhit.dot(lightDirection)) * spheres[i].material.ks; 
//             } 
//         } 
//     } 
 
//     return surfaceColor + sphere->material.ks; 
// } 
//render computes the initial rays and calls trace to generate the color intensity for each pixel in vec3 image
void render(){ 
    // Trace rays
    int k = WIDTH * HEIGHT;
    for (unsigned y = 0; y < HEIGHT; ++y) { 
        for (unsigned x = 0; x < WIDTH; ++x, --k) { 
            GLfloat vx = (2 * ((x + 0.5) * WIDTH_INVERSE) - 1) * angle * aspectratio; 
            GLfloat vy = (1 - 2 * ((y + 0.5) * HEIGHT_INVERSE)) * angle; 
            vec3 v(vx, vy, -1); 
            v.normalize(); 

            image[k] = trace(Ray(vec3(0), v), 0); 
        } 
    } 
} 

//reshape function to prevent distorting when resizing window
void reshape(int w, int h){
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
}

//reads vec3 image and draws pixels to screen using glDrawPixels
void render_scene(void){
    glMatrixMode(GL_MODELVIEW);
    glRasterPos2f(0.0, 0.0);
    glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_FLOAT, image);
    glutSwapBuffers();
}

//creates primitives needed for scene1
void construct_scene_1(){
    std::vector<Sphere> spheres; 
    // position, radius, surface color, reflectivity, transparency, emission color
    spheres.push_back(Sphere(vec3( 0.0, -10004, -20), 10000, Material(vec3(0.20, 0.20, 0.20), 0, 0.0))); 
    spheres.push_back(Sphere(vec3( 0.0,      0, -20),     4, Material(vec3(1.00, 0.32, 0.36), 1, 0.5))); 
    spheres.push_back(Sphere(vec3( 5.0,     -1, -15),     2, Material(vec3(0.90, 0.76, 0.46), 1, 0.0))); 
    spheres.push_back(Sphere(vec3( 5.0,      0, -25),     3, Material(vec3(0.65, 0.77, 0.97), 1, 0.0))); 
    spheres.push_back(Sphere(vec3(-5.5,      0, -15),     3, Material(vec3(0.90, 0.90, 0.90), 1, 0.0))); 
    // light
    //spheres.push_back(Sphere(vec3( 0.0,     20, -30),     3, Material(vec3(0.00, 0.00, 0.00), 0, 0.0, vec3(3)))); 
    render(); 
}

//main function
int main(int argc, char **argv){ 
    construct_scene_1();
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowPosition(100, 500);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Raytracing program");
    glutDisplayFunc(render_scene);
    glutReshapeFunc(reshape);
    glutMainLoop();

    return 0; 
} 