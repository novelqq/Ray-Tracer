#include <cstdlib> 
#include <cstdio> 
#include <cmath> 
#include <vector> 
#include <iostream> 
#include <GL/glut.h>
#include "prim.hpp"

//pixel resolution
const int WIDTH = 400, HEIGHT = 400; 

//stores rendered pixels
vec3 *image = new vec3[WIDTH * HEIGHT];

//ray recursion depth
const int MAX_RAY_DEPTH = 4;

//some calculations needed for determining initial rays for each pixel
GLfloat WIDTH_INVERSE = 1 / GLfloat(WIDTH);
GLfloat HEIGHT_INVERSE = 1 / GLfloat(HEIGHT); 
GLfloat fov = 30, aspectratio = WIDTH / GLfloat(HEIGHT); 
GLfloat angle = tan(M_PI * 0.5 * fov / 180.); 

//background color
vec3 background_color = vec3(0.1, 0.3, 0.7);

//ambient lighting color
vec3 ambient = vec3(0.1);

//global lists holding scene objects
std::vector<Sphere> spheres;
std::vector<Light> lights;
std::vector<Plane> planes;

//finds the nearest colliding primitive, updates nearest_sphere and nearest_plane, and returns the distance to the nearest colliding primitive
GLfloat get_intersect(const Ray& ray, const Sphere **nearest_sphere, const Plane **nearest_plane){
    GLfloat min_distance = INFINITY;
    for(int i = 0; i < spheres.size(); ++i){
        GLfloat t = spheres[i].intersect(ray);
        if(t >0){
            //std::cout << t << std::endl;
        }
        if(t > 0 && t < min_distance){
            min_distance = t;
            *nearest_sphere = &spheres[i];
        }
    }
    for(int i = 0; i < planes.size(); ++i){
        GLfloat t = planes[i].intersect(ray);
        if(t > 0 && t < min_distance){
            min_distance = t;
            *nearest_plane = &planes[i];
            *nearest_sphere = NULL;
        }
    }
    return min_distance;
}

//does lighting calculations and returns the color intensity
vec3 compute_lighting(vec3 intersection, vec3 normal, vec3 v, Material material){
    vec3 i = vec3(0);
    vec3 j = vec3(0);
    GLfloat bias = 1e-4;
    i += ambient;
    //j += ambient;
    for(int k = 0; k < lights.size(); k++){
        vec3 l = lights[k].center - intersection;
        const Sphere *nearest_sphere = NULL; 
        const Plane *nearest_plane = NULL;
        GLfloat min_distance = get_intersect(Ray(intersection + normal * bias, l.normalize()), &nearest_sphere, &nearest_plane);
        if((nearest_sphere != NULL || nearest_plane != NULL)){
            
            continue;
        }
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
    return i*material.kd + j*material.ks;

}
vec3 reflect_ray(const vec3 &direction, const vec3 &normal){
    return (normal*2)*normal.dot(direction) - direction;
}
//trace takes a Ray ray and a int depth and returns a vec3 color intensity. If depth == 0, will return bg
vec3 trace(const Ray& ray, const int &depth){
    const Sphere *nearest_sphere = NULL; 
    const Plane *nearest_plane = NULL;
    GLfloat min_distance = get_intersect(ray, &nearest_sphere, &nearest_plane);
    
    if(nearest_sphere == NULL && nearest_plane == NULL){
        return background_color;
    }
    if(nearest_sphere != NULL){
        vec3 intersection = ray.origin + ray.direction*min_distance;
        vec3 normal = intersection - nearest_sphere->center;
        normal.normalize();
        bool inside = false;
        if(ray.direction.dot(normal) > 0){
            normal = -normal;
            inside = true;
        }
        vec3 ret = compute_lighting(intersection, normal, -(ray.direction), nearest_sphere->material);

        //return ret;
        if(depth> 0 && nearest_sphere->material.kr > 0){
            vec3 refl = reflect_ray(-(ray.direction), normal);
            vec3 reflc = trace(Ray(intersection, refl), depth - 1); 

            ret = ret*(1-nearest_sphere->material.kr) + reflc*(nearest_sphere->material.kr);
        }
        //modified refraction referenced from: https://www.scratchapixel.com/code.php?id=3&origin=/lessons/3d-basic-rendering/introduction-to-ray-tracing
        if(depth > 0 && nearest_sphere->material.kt > 0){
            GLfloat ior = nearest_sphere->material.ki; 
            GLfloat ratio = 1 / ior; 
            if(inside){ratio = ior;}
            GLfloat cosi = -normal.dot(ray.direction); 
            GLfloat k = 1 - ratio * ratio * (1 - cosi * cosi); 
            vec3 refr = ray.direction * ratio + normal * (ratio *  cosi - sqrt(k)); 
            refr.normalize(); 
            vec3 refrc = trace(Ray(intersection - normal, refr), depth - 1); 
            ret = ret*(1-nearest_sphere->material.kt) + refrc*(nearest_sphere->material.kt);
        }
        else{
            return ret;
        }
        return ret;
    }
    if(nearest_plane != NULL){
        vec3 intersection = ray.origin + ray.direction*min_distance;
        vec3 normal = vec3(nearest_plane->a, nearest_plane->b, nearest_plane->c);
        normal.normalize();
        vec3 ret = compute_lighting(intersection, normal, -(ray.direction), nearest_plane->material);
        //std::cout << nearest_plane->material.kr << std::endl;
        if(depth > 0 && nearest_plane->material.kr > 0){
            vec3 refl = reflect_ray(-(ray.direction), normal);
            vec3 reflc = trace(Ray(intersection, refl), depth - 1); 
            return ret*(1-nearest_plane->material.kr) + reflc*(nearest_plane->material.kr);
        }
        if(depth > 0 && nearest_plane->material.kt > 0){

        }
        else{
            return ret;
        }
    }
    
}


//render computes the initial rays and calls trace to generate the color intensity for each pixel in vec3 image
void render(){ 
    int k = WIDTH * HEIGHT;
    for (unsigned y = 0; y < HEIGHT; ++y) { 
        for (unsigned x = 0; x < WIDTH; ++x, --k) { 
            //caculate initial rays
            GLfloat vx = (2 * ((x + 0.5) * WIDTH_INVERSE) - 1) * angle; 
            GLfloat vy = (1 - 2 * ((y + 0.5) * HEIGHT_INVERSE)) * angle; 
            vec3 v(vx, vy, -1); 
            v.normalize(); 

            image[k] = trace(Ray(vec3(0), v), MAX_RAY_DEPTH); 
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

//creates primitives needed for scene 2
void construct_scene_2(){ //        kd              ks                 q    r     t    i
    Material aqua = Material(vec3(0.1, 0.3, 0.8), vec3(0.9, 0.9, 0.9), 32, 0.5, 0.0, 1.5);
    Material shiny_black = Material(vec3(0.3, 0.3 , 0.3), vec3(0.9, 0.9, 0.9), 1.5 , 0.4, 0.0, 0.0);
    Material emerald_green = Material(vec3(0.3, 0.8, 0.3), (0.9, 0.9, 0.9), 32, 0.5, 0.0, 0.0);
    Material mirror = Material(vec3(0.9, 0.9, 0.9), (0.9, 0.9, 0.9), 32, 0.5, 0.0, 0.0);
    //                         position                radius material
    
    spheres.push_back(Sphere(vec3(0.0,    -5001.5, -10.0), 5000, shiny_black));
    spheres.push_back(Sphere(vec3(0.0,    0.0, -10.0), 1.0, mirror));
    spheres.push_back(Sphere(vec3(4.0,    3.5, -20.0), 3.67, emerald_green));
    spheres.push_back(Sphere(vec3(-1.0,    -1, -8.0), 0.3, aqua));
    //planes.push_back(Plane(0.0, 1, 0.0, -5.0, aqua));
    
    lights.push_back(Light(vec3(0.0, 10.0, -10.0), vec3(0.5, 0.5, 0.5)));
    //lights.push_back(Light(vec3(4.0, 4.0, -10.0), vec3(0.5, 0.5, 0.5)));
    // vec3 ret = aqua.kd;
    // std::cout << ret.x << " " << ret.y << " "<< ret.z << std::endl;
    // vec3 ret1 = spheres[0].material.kd;
    // std::cout << ret1.x << " " << ret1.y << " "<< ret1.z << std::endl;
    render(); 
}

//creates primitives needed for scene 1
void construct_scene_3(){
    Material black = Material(vec3(0.8, 0.8, 0.8), vec3(0.9, 0.9, 0.9), 4, 0.6, 0.0, 0.0);
    Material red = Material(vec3(1, 0, 0), vec3(0.9,0.9,0.9), 16, 0.0, 0.4, 1.1); 
    Material green = Material(vec3(0, 1, 0), vec3(0.9,0.9,0.9), 16, 0.0, 0.4, 1.1); 
    Material clear = Material(vec3(0.8, 0.8, 0.8), vec3(0.9,0.9,0.9), 500, 0.5, 0.9, 1.35); 
    lights.push_back(Light(vec3(0.0, 4.0, -5.0), vec3(1, 1,1)));
    spheres.push_back(Sphere(vec3(2.0,    1.5, -15.0), 2.0, red));
    spheres.push_back(Sphere(vec3(-2.0,    1.5, -15.0), 2.0, green));
    spheres.push_back(Sphere(vec3(0.0,    -5001.5, -10.0), 5000, black));
    //spheres.push_back(Sphere(vec3(-3.0,    0.0, -20.0), 1.5, green));
    spheres.push_back(Sphere(vec3(0.5,    0.0, -8.0), 0.5, clear));
    spheres.push_back(Sphere(vec3(-0.5,    0.0, -8.0), 0.5, clear));
    planes.push_back(Plane(0.0, 1, 0.0, -5.0, black)); 
    render();
}

void construct_scene_4(){
    Material black = Material(vec3(0.8, 0.8, 0.8), vec3(0.9, 0.9, 0.9), 4, 0.6, 0.0, 0.0);
    Material red = Material(vec3(1, 0, 0), vec3(0.9,0.9,0.9), 16, 0.5, 0.4, 1.1); 
    Material green = Material(vec3(0, 1, 0), vec3(0.9,0.9,0.9), 16, 0.5, 0.4, 1.1); 
    Material clear = Material(vec3(0.8, 0.8, 0.8), vec3(0.9,0.9,0.9), 500, 0.5, 0.9, 1); 
    Material clear_water = Material(vec3(0.4, 0.4, 0.8), vec3(0.9,0.9,0.9), 500, 0.5, 0.9, 1.33); 
    lights.push_back(Light(vec3(4.0, 7.0, -6.0), vec3(1, 1,1)));
    //lights.push_back(Light(vec3(-5.0, 5.0, -10.0), vec3(1, 1,1)));
    spheres.push_back(Sphere(vec3(2.0,    1.5, -15.0), 2.0, red));
    spheres.push_back(Sphere(vec3(-2.0,    1.5, -15.0), 2.0, green));
    spheres.push_back(Sphere(vec3(0.0,    -5001.5, -10.0), 5000, black));
    //spheres.push_back(Sphere(vec3(-3.0,    0.0, -20.0), 1.5, green));
    spheres.push_back(Sphere(vec3(-0.1,    0.1, -1.0), 0.1, clear_water));
    spheres.push_back(Sphere(vec3(0.0,    0.0, -14.0), 1, black));
    planes.push_back(Plane(0.0, 1, 0.0, -5.0, black)); 
    render();
}
void construct_scene_1(){
    Material black = Material(vec3(0.1, 0.1, 0.1), vec3(0.9, 0.9, 0.9), 4, 0, 0.0, 0.0);
    Material red = Material(vec3(1, 0, 0), vec3(0.9,0.9,0.9), 16, 0.0, 0.0, 0.0); 
    Material green = Material(vec3(0, 1, 0), vec3(0.9,0.9,0.9), 64, 0.0, 0.0, 0.0); 
    Material blue = Material(vec3(0.1, 0.8, 0.9), vec3(0.9,0.9,0.9), 500, 0.0, 0.0, 0.0); 
    lights.push_back(Light(vec3(1.0, 2.0, -5.0), vec3(1, 1,1)));
    spheres.push_back(Sphere(vec3(0.0,    -0.5, -10.0), 1.0, red));
    spheres.push_back(Sphere(vec3(0.0,    -5001.5, -10.0), 5000, black));
    spheres.push_back(Sphere(vec3(-3.0,    0.0, -20.0), 1.5, green));
    spheres.push_back(Sphere(vec3(0.5,    -0.3, -8.0), 0.3, blue));
    planes.push_back(Plane(0.0, 1, 0.0, -5.0, black)); 
    render();
}
//main function
int main(int argc, char **argv){ 
    if(argc != 2){
        std::cout << "wrong arg input"  << std::endl;
    }
    #include <stdexcept>
    #include <string>
    int x;
    std::string arg = argv[1];
    try {
    std::size_t pos;
    x = std::stoi(arg, &pos);
    if (pos < arg.size()) {
        std::cerr << "Trailing characters after number: " << arg << '\n';
    }
    } catch (std::invalid_argument const &ex) {
    std::cerr << "Invalid number: " << arg << '\n';
    } catch (std::out_of_range const &ex) {
    std::cerr << "Number out of range: " << arg << '\n';
    } 
    switch(x){
        case 1:
            construct_scene_1();
            break;
        case 2:
            construct_scene_2();
            break;
        case 3:
            construct_scene_3();
            break;
        case 4:
            construct_scene_4();
            break;
        
    }
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