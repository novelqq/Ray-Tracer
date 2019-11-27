#include <GL/glut.h>
#include <vector>
#include "vec3.hpp"
#include <iostream>

// Globals
const int HEIGHT = 400;
const GLfloat INV_HEIGHT = 1/GLfloat(HEIGHT);
const int WIDTH  = 400;
const GLfloat INV_WIDTH = 1/GLfloat(WIDTH);
const int MAX_RAY_DEPTH = 5; 

GLfloat fov = 60;
GLfloat aspectratio = GLfloat(WIDTH) / GLfloat(HEIGHT); 
GLfloat angle = tan(M_PI * 0.5 * fov / 180.); 
vec3 bg(3); 
vec3 pixels[WIDTH][HEIGHT][3];


void change_fov(float value){
    fov = value;
}

class Ray{
    public:
        Ray(const vec3& direction, const vec3& origin): direction(direction), origin(origin){}
        vec3 direction;
        vec3 origin;
};

class Color{
    public:
        Color(GLfloat s = GLfloat(0.0)): red(s), blue(s), green(s){}
        GLfloat red;
        GLfloat green;
        GLfloat blue;
};
class Material{
    public:
        const Color kd;
        const Color ks;
        const GLfloat q;
        const GLfloat kr;
        const GLfloat kt;
};
class Plane{
    public:
        GLfloat a, b, c, d;

};
class Sphere{
    public:
        vec3 center;
        GLfloat radius, radius2;
        vec3 surfaceColor, emissionColor;
        GLfloat trans, refl;
        Sphere( 
            const vec3 &c, 
            const float &r, 
            const vec3 &sc, 
            const float &refl = 0, 
            const float &transp = 0, 
            const vec3 &ec = 0) : 
            center(c), radius(r), radius2(r * r), surfaceColor(sc), emissionColor(ec), 
            trans(transp), refl(refl) 
        { /* empty */ } 
        

        bool intersect(const Ray &ray, GLfloat &t0, GLfloat &t1) const{
            vec3 diff = center - ray.origin;
            GLfloat tca = diff.dot(ray.direction);
            if(tca < 0){
                return false;
            }
            GLfloat d2 = diff.length() - tca * tca; 
            if(d2 > radius2){
                return false;
            }
            GLfloat thc = std::sqrt(radius2 - d2); 
            t0 = tca - thc;
            t1 = tca + thc;
            return true;
        }
};

std::vector<Sphere> spheres;


GLfloat mix(const float &a, const float &b, const float &mix){ 
    return b * mix + a * (1 - mix); 
} 
vec3 ray_trace(const Ray &ray, const std::vector<Sphere> &spheres, const int &depth){
    //shoot primary ray in scene and get intersection
    float minDistance = INFINITY;
    const Sphere *sphere = NULL;
    for(int k = 0; k < spheres.size(); ++k){
        GLfloat t0 = INFINITY, t1 = INFINITY;
        if(spheres[k].intersect(ray, t0, t1)){
            if(t0 < 0){
                t0 =  t1;
            }
            if(t0 < minDistance){
                minDistance = t0;
                sphere = &spheres[k]; 
            }
        }
    }
    if(!sphere){
        return bg;
    }
   else{
        vec3 surfaceColor = 0; // color of the ray/surfaceof the object intersected by the ray 
        vec3 phit = ray.origin + ray.direction * minDistance; // point of intersection 
        vec3 nhit = phit - sphere->center; // normal at the intersection point 
        nhit.normalize(); // normalize normal direction 
        // If the normal and the view direction are not opposite to each other
        // reverse the normal direction. That also means we are inside the sphere so set
        // the inside bool to true. Finally reverse the sign of IdotN which we want
        // positive.
        float bias = 1e-4; // add some bias to the point from which we will be tracing 
        bool inside = false; 
        if (ray.direction.dot(nhit) > 0) nhit = -nhit, inside = true; 
        if ((sphere->trans > 0 || sphere->refl > 0) && depth < MAX_RAY_DEPTH) { 
            float facingratio = -ray.direction.dot(nhit); 
            // change the mix value to tweak the effect
            float fresneleffect = mix(pow(1 - facingratio, 3), 1, 0.1); 
            // compute reflection direction (not need to normalize because all vectors
            // are already normalized)
            vec3 refldir = ray.direction - nhit * 2 * ray.direction.dot(nhit); 
            refldir.normalize(); 
            vec3 reflection = ray_trace(Ray(phit + nhit * bias, refldir), spheres, depth + 1); 
            vec3 refraction = 0; 
            // if the sphere is also transparent compute refraction ray (transmission)
            if (sphere->trans) { 
                float ior = 1.1, eta = (inside) ? ior : 1 / ior; // are we inside or outside the surface? 
                float cosi = -nhit.dot(ray.direction); 
                float k = 1 - eta * eta * (1 - cosi * cosi); 
                vec3 refrdir = ray.direction * eta + nhit * (eta *  cosi - sqrt(k)); 
                refrdir.normalize(); 
                refraction = ray_trace(Ray(phit - nhit * bias, refrdir), spheres, depth + 1); 
            } 
            // the result is a mix of reflection and refraction (if the sphere is transparent)
            surfaceColor = ( 
                reflection * fresneleffect + 
                refraction * (1 - fresneleffect) * sphere->trans) * sphere->surfaceColor; 
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
                            float t0, t1; 
                            if (spheres[j].intersect(Ray(phit + nhit * bias, lightDirection), t0, t1)) { 
                                transmission = 0; 
                                break; 
                            } 
                        } 
                    } 
                    surfaceColor = surfaceColor + sphere->surfaceColor * transmission * 
                    std::max(float(0), nhit.dot(lightDirection)) * spheres[i].emissionColor; 
                } 
            } 
        } 
        return surfaceColor + sphere->emissionColor;
    }
}
vec3 *image = new vec3[WIDTH * HEIGHT], *pixel = image; 
void render(){
    for(int y = 0; y < HEIGHT; y++){
        for(int x = 0; x < WIDTH; x++, ++pixel){
            float xx = (2 * ((x + 0.5) * WIDTH) - 1) * angle * aspectratio; 
            float yy = (1 - 2 * ((y + 0.5) * HEIGHT)) * angle;
            vec3 dir(xx, yy, -1);
            dir = dir.normalize(); 
            Ray ray(dir, vec3(0));
            *pixel = ray_trace(ray, spheres, 0);
        }
    }
}
void init(){
    spheres.push_back(Sphere(vec3( 0.0, -10004, -20), 10000, vec3(0.20, 0.20, 0.20), 0, 0.0)); 
    spheres.push_back(Sphere(vec3( 0.0,      0, -20),     4, vec3(1.00, 0.32, 0.36), 1, 0.5)); 
    spheres.push_back(Sphere(vec3( 5.0,     -1, -15),     2, vec3(0.90, 0.76, 0.46), 1, 0.0)); 
    spheres.push_back(Sphere(vec3( 5.0,      0, -25),     3, vec3(0.65, 0.77, 0.97), 1, 0.0)); 
    spheres.push_back(Sphere(vec3(-5.5,      0, -15),     3, vec3(0.90, 0.90, 0.90), 1, 0.0)); 
    // light
    spheres.push_back(Sphere(vec3( 0.0,     20, -30),     3, vec3(0.00, 0.00, 0.00), 0, 0.0, vec3(3))); 
    render(); 
}

void reshape(int w, int h){
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
}
void render_scene1(void){
    glMatrixMode(GL_MODELVIEW);
    glRasterPos2f(0.0, 0.0);
    glDrawPixels(WIDTH, HEIGHT, GL_RGB, GL_FLOAT, image);
    glutSwapBuffers();
}
int main(int argc, char **argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE| GLUT_RGB);
    glutInitWindowPosition(100, 500);
    glutInitWindowSize(WIDTH, HEIGHT);
    glutCreateWindow("Raytracing program");
    init();
    glutDisplayFunc(render_scene1);
    glutReshapeFunc(reshape);
    glutMainLoop();

}