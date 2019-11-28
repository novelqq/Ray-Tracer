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

vec3 background_color = vec3(0.0);
vec3 ambient = vec3(0.2);
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
        Plane(const GLfloat &a, const GLfloat &b, const GLfloat &c, const GLfloat &d, const Material &material):
            a(a), b(b), c(c), d(d), material(material){}
        GLfloat a, b, c, d;
        Material material;

        GLfloat intersect(const Ray& ray){
            GLfloat t = (vec3(a, b, c).dot(ray.origin) + d) / (vec3(a, b, c).dot(ray.direction));
            std::cout << t << std::endl;
            return t;  
        }

};

//simple local point light sources with no falloff, consists of a vec3 center and a vec3 color intensity
class Light{
    public:
        Light(const vec3& center, const vec3& color): center(center), color(color){}
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
                center(c), radius(r), radius2(r * r), material(mat){}
        
        GLfloat intersect(const Ray &ray) const{ 
            GLfloat t0, t1;
            vec3 l = center - ray.origin; 
            GLfloat tca = l.dot(ray.direction); 
            if (tca < 0){
                return -1;
            }
            GLfloat d2 = l.dot(l) - tca * tca; 
            if (d2 > radius2){
                return -1;
            } 
            GLfloat thc = sqrt(radius2 - d2); 
            t0 = tca - thc; 
            t1 = tca + thc; 
            if (t0 < 0){
                t0 = t1;
            }
            return t0;
        } 
}; 

std::vector<Sphere> spheres;
std::vector<Light> lights;
std::vector<Plane> planes;
 
GLfloat mix(const GLfloat &a, const GLfloat &b, const GLfloat &mix){ 
    return b * mix + a * (1 - mix); 
} 
vec3 compute_lighting(vec3 intersection, vec3 normal, vec3 v, Material material){
    vec3 i = vec3(0);
    vec3 j = vec3(0);
    i += ambient;
    j += ambient;
    for(int k = 0; k < lights.size(); k++){
        vec3 l = lights[k].center - intersection;
        
        GLfloat ndot = normal.dot(l);
        if(ndot > 0){
            i = i + (lights[k].color * ndot) / (normal.length()*l.length());
        }

        vec3 r = (normal * 2)*ndot - l;
        GLfloat rdot = r.dot(v);
        if(rdot > 0){
            j = j + (lights[k].color * std::pow(rdot / r.length() * v.length(), material.q));
        }


    }
    return i*material.kd + j*material.ki;

}
//trace takes a Ray ray and a int depth and returns a vec3 color intensity. If depth == 0, will return bg
vec3 trace(const Ray& ray, const int &depth){
    GLfloat min_distance = INFINITY;
    const Sphere* nearest_sphere = NULL; 
    const Plane* nearest_plane = NULL;
    for(int i = 0; i < spheres.size(); ++i){
        GLfloat t = spheres[i].intersect(ray);
        if(t > 0){
            //std::cout << t << std::endl;
        }
        if(t > 0 && t < min_distance){
            min_distance = t;
            
            nearest_sphere = &spheres[i];
        }
    }
    for(int i = 0; i < planes.size(); ++i){
        GLfloat t = planes[i].intersect(ray);
        if(t > 0 && t < min_distance){
            min_distance = t;
            nearest_plane = &planes[i];
            nearest_sphere = NULL;
        }
    }
    if(nearest_sphere == NULL && nearest_plane == NULL){
        return background_color;
    }
    if(nearest_sphere != NULL){
        vec3 intersection = ray.origin + ray.direction*min_distance;
        vec3 normal = intersection - nearest_sphere->center;
        normal.normalize();
        //vec3 ret = nearest_sphere->material.kd;
        //std::cout << ret.x << " " << ret.y << " "<< ret.z << std::endl;
        vec3 ret = compute_lighting(intersection, normal, -(ray.direction), nearest_sphere->material);
        return ret;
    }
    if(nearest_plane != NULL){
        vec3 intersection = ray.origin + ray.direction*min_distance;
        vec3 normal = intersection - nearest_sphere->center;
        normal.normalize();
        // vec3 ret = compute_lighting(intersection, normal, -(ray.direction), nearest_plane->material);
        // return ret;
    }
    
}


//render computes the initial rays and calls trace to generate the color intensity for each pixel in vec3 image
void render(){ 
    // Trace rays
    int k = WIDTH * HEIGHT;
    for (unsigned y = 0; y < HEIGHT; ++y) { 
        for (unsigned x = 0; x < WIDTH; ++x, --k) { 
            GLfloat vx = (2 * ((x + 0.5) * WIDTH_INVERSE) - 1) * angle; 
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
void construct_scene_1(){ //                kd                                      ks                 q    r     t    i
    Material aqua = Material(vec3(GLfloat(0.1), GLfloat(0.8), GLfloat(0.8)), vec3(0.9, 0.9, 0.9), 32, 0.5, 0.0, 1.5);
    //                         position                radius material
    spheres.push_back(Sphere(vec3(0.0,    0.0, -10.0), 1.5, aqua));
    lights.push_back(Light(vec3(0.0, 30.0, -10.0), vec3(0.5, 0.5, 0.5)));
    planes.push_back(Plane(0.0, 1.0, 0.0, 1.0, aqua));
    vec3 ret = aqua.kd;
    std::cout << ret.x << " " << ret.y << " "<< ret.z << std::endl;
    vec3 ret1 = spheres[0].material.kd;
    std::cout << ret1.x << " " << ret1.y << " "<< ret1.z << std::endl;
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