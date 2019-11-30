#include "vec3.hpp"

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
            // vec3 center = vec3(0, 0, d/c); 
            // vec3 norm = vec3(a, b, c);
            // GLfloat denom = norm.dot(ray.direction);
            // if(std::abs(denom) > 0.00001f){
            //     GLfloat t = (center - ray.origin).dot(norm) / denom;
            //     return t;
            // }
            GLfloat t = (vec3(a, b, c).dot(ray.origin) + d) / (vec3(a, b, c).dot(ray.direction));
            // return -1;
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
        
        //calulates ray-sphere intersection, returns closest valid intersection point
        GLfloat intersect(const Ray &ray) const{ 
            GLfloat t0, t1;
            vec3 oc = center - ray.origin; 
            GLfloat k1 = oc.dot(ray.direction); 
            GLfloat k2 = oc.dot(oc) - k1 * k1; 
            if (k1 < 0){
                return -1;
            }
            
            if (k2 > radius2){
                return -1;
            } 
            GLfloat k3 = sqrt(radius2 - k2); 
            t0 = k1 - k3; 
            t1 = k1 + k3; 
            if (t0 < 0){
                t0 = t1;
            }
            return t0;
        } 
}; 